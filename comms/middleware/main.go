package main

import (
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"sync"
	"time"

	"github.com/ParthGandhiNUS/CG4002/config"
	"github.com/gorilla/websocket"
)

type TLSConfig struct {
	CACertFile     string
	ServerCertFile string
	ServerKeyFile  string
}

type Hub struct {
	sessions     map[string]map[*WSClient]bool
	userSessions map[string][]*WSClient
	broadcast    chan BroadcastMessage
	register     chan *WSClient
	unregister   chan *WSClient
	mutex        sync.RWMutex
}

type WSClient struct {
	ID        string // connection ID
	UserID    string // ID of connected User
	SessionID string
	Conn      *websocket.Conn
	Send      chan []byte
	Hub       *Hub
	ClientCN  string
}

type BroadcastMessage struct {
	SessionID string
	Data      []byte
	Sender    *WSClient
	EventType string
}

type AREvent struct {
	EventType string `json:"eventType"`
	UserID    string `json:"userId"`
	SessionID string `json:"sessionId"`
	Timestamp int64  `json:"timestamp"`
	Data      interface{}
}

// Helper function to register clients to server
func (h *Hub) registerClient(client *WSClient) {
	h.mutex.Lock()
	defer h.mutex.Unlock()

	// Make session if session does not exist
	if h.sessions[client.SessionID] == nil {
		h.sessions[client.SessionID] = make(map[*WSClient]bool)
	}
	h.sessions[client.SessionID][client] = true

	h.userSessions[client.UserID] = append(h.userSessions[client.UserID], client)
	log.Printf("Client %s (User: %s, CN: %s) joined room %s. Session size: %v",
		client.ID, client.UserID, client.ClientCN, client.SessionID, len(h.sessions[client.SessionID]),
	)
}

// Helper function to remove client from server
func (h *Hub) unregisterClient(client *WSClient) {
	h.mutex.Lock()
	defer h.mutex.Unlock()

	if clients, ok := h.sessions[client.SessionID]; ok {
		if _, ok := clients[client]; ok {
			delete(clients, client) // remove client from list of clients
			close(client.Send)      // Close send channel of client
		}

		userClients := h.userSessions[client.UserID]
		for i, c := range userClients {
			if c == client {
				h.userSessions[client.UserID] = append(userClients[:i], userClients[i+1:]...)
				break
			}
		}

		// Clean up empty sessions
		if len(clients) == 0 {
			delete(h.sessions, client.SessionID)
		}

		if len(h.userSessions[client.UserID]) == 0 {
			delete(h.userSessions, client.UserID)
		}

		log.Printf("Client %s (User: %s) left session %s", client.ID, client.UserID, client.SessionID)

	}
}

// Helper function to send message to all clients connected to the server
func (h *Hub) broadcastMessage(message BroadcastMessage) {
	h.mutex.RLock()
	defer h.mutex.RUnlock()

	if clients, ok := h.sessions[message.SessionID]; ok {
		for client := range clients {
			if client == message.Sender {
				continue // dont need to send itself
			}

			select {
			case client.Send <- message.Data:
			default:
				close(client.Send)
				delete(clients, client)
			}
		}
	}
}

// Reads incoming messages from channel for Websocket server
func (c *WSClient) readPump() {
	defer func() {
		c.Hub.unregister <- c
		c.Conn.Close()
	}()

	// set connection
	c.Conn.SetReadLimit(512 * 1024) // 512kb max message size
	c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
	c.Conn.SetPongHandler(func(string) error {
		c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
		return nil
	})

	for {
		_, message, err := c.Conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("WebSocket error for client %s: %v", c.ID, err)
			}
			break
		}

		log.Printf("Raw message from client %s: %s", c.ID, string(message))

		var arEvent AREvent
		if err := json.Unmarshal(message, &arEvent); err != nil {
			log.Printf("Invalid message format from client %s: %v", c.ID, err)
			continue
		}

		// Rewriting with c to prevent spoofing
		arEvent.UserID = c.UserID
		arEvent.SessionID = c.SessionID
		arEvent.Timestamp = time.Now().UnixMilli()

		processedMessage, _ := json.Marshal(arEvent)
		c.Hub.broadcast <- BroadcastMessage{
			SessionID: c.SessionID,
			Data:      processedMessage,
			Sender:    c,
			EventType: arEvent.EventType,
		}

		// Send acknowledgment for debug.ping messages
		if arEvent.EventType == "debug.ping" {
			ackEvent := AREvent{
				EventType: "debug.pong",
				UserID:    c.UserID,
				SessionID: c.SessionID,
				Timestamp: time.Now().UnixMilli(),
				Data: map[string]interface{}{
					"originalTimestamp": arEvent.Timestamp,
					"message":           "ping received",
				},
			}
			ackMessage, _ := json.Marshal(ackEvent)
			select {
			case c.Send <- ackMessage:
			default:
				// If the send channel is full, we skip the acknowledgment
			}
		}
	}
}

func (c *WSClient) writePump() {
	ticker := time.NewTicker(54 * time.Second)
	defer func() {
		ticker.Stop()
		c.Conn.Close()
	}()

	for {
		select {
		case message, ok := <-c.Send:
			c.Conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
			if !ok {
				c.Conn.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}
			if err := c.Conn.WriteMessage(websocket.TextMessage, message); err != nil {
				log.Printf("Write error for client %s: %v", c.ID, err)
				return
			}
		case <-ticker.C:
			c.Conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
			if err := c.Conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				return
			}
		}
	}
}

// Imports required TLS certs
func setupTLSConfig(config TLSConfig) (*tls.Config, error) {
	// load CA cert
	caCert, err := os.ReadFile(config.CACertFile)
	if err != nil {
		return nil, fmt.Errorf("failed to read CA certificate: %v", err)
	}

	caCertPool := x509.NewCertPool()
	if !caCertPool.AppendCertsFromPEM(caCert) {
		return nil, fmt.Errorf("failed to parse CA certificate")
	}

	serverCert, err := tls.LoadX509KeyPair(config.ServerCertFile, config.ServerKeyFile)
	if err != nil {
		return nil, fmt.Errorf("failed to load server certificates: %v", err)
	}

	tlsConfig := &tls.Config{
		Certificates: []tls.Certificate{serverCert},
		ClientCAs:    caCertPool,
		ClientAuth:   tls.RequireAndVerifyClientCert,
	}

	return tlsConfig, nil
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true // Allow connections from any origin
	},
}

// Setup Websocket with TLS client certificate verification
func handleWebsocket(hub *Hub, w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Websocket upgrade error: %v", err)
		return
	}

	var clientCN string
	var verified bool = false
	if r.TLS != nil && len(r.TLS.PeerCertificates) > 0 {
		clientCert := r.TLS.PeerCertificates[0]
		clientCN = clientCert.Subject.CommonName
		verified = r.TLS.HandshakeComplete

		log.Printf("Client certificate - CN: %s, Verified: %v", clientCN, verified)

		if verified {
			now := time.Now()
			if now.Before(clientCert.NotBefore) || now.After(clientCert.NotAfter) {
				log.Printf("Client certificate for CN %s is not within validity period", clientCN)
				verified = false
			}
		}
	} else {
		log.Printf("No client certificate provided")
	}

	userID := r.URL.Query().Get("userId")
	sessionID := r.URL.Query().Get("sessionId")
	deviceID := r.URL.Query().Get("deviceId")

	if userID == "" {
		log.Printf("Missing userId parameter")
		conn.Close()
		return
	}

	// use UserID as SessionID if not provided
	if sessionID == "" {
		sessionID = userID
	}

	// Generate deviceID based on time if not provided
	if deviceID == "" {
		deviceID = fmt.Sprintf("device_%d", time.Now().UnixNano())
	}

	if !verified {
		log.Printf("Rejecting unverified client %s", deviceID)
	}

	client := &WSClient{
		ID:        deviceID,
		UserID:    userID,
		SessionID: sessionID,
		Conn:      conn,
		Send:      make(chan []byte, 256),
		Hub:       hub,
		ClientCN:  clientCN,
	}

	hub.register <- client

	go client.writePump()
	go client.readPump()

}

// Creates a hub of channels for websocket server event
func NewHub() *Hub {
	return &Hub{
		sessions:     make(map[string]map[*WSClient]bool),
		userSessions: make(map[string][]*WSClient),
		broadcast:    make(chan BroadcastMessage),
		register:     make(chan *WSClient),
		unregister:   make(chan *WSClient),
	}
}

// Manages Websocket Connection Events
func (h *Hub) Run() {
	for {
		select {
		case client := <-h.register:
			h.registerClient(client)
		case client := <-h.unregister:
			h.unregisterClient(client)
		case client := <-h.broadcast:
			h.broadcastMessage(client)
		}
	}
}

func main() {
	envConfig := config.LoadConfig()

	tlsConfig := TLSConfig{
		CACertFile:     envConfig.CACertLocation,
		ServerCertFile: envConfig.ServerCertLocation,
		ServerKeyFile:  envConfig.ServerKeyLocation,
	}

	tlsConf, err := setupTLSConfig(tlsConfig)
	if err != nil {
		log.Fatal("Failed to setup TLS:", err)
	}

	hub := NewHub()
	go hub.Run()

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		handleWebsocket(hub, w, r)
	})
	// http.HandleFunc("/health", handleHealth)

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == "OPTIONS" {
			return
		}

		w.WriteHeader(http.StatusNotFound)
	})

	server := &http.Server{
		Addr:      ":" + envConfig.WebSocketPort,
		TLSConfig: tlsConf,
	}

	log.Printf("Starting secure WebSocket server on port :8443")
	log.Printf("Secure WebSocket endpoint: wss://localhost:8443/ws?userId=<USER_ID>&sessionId=<SESSION_ID>")
	log.Printf("CA Certificate: %s", tlsConfig.CACertFile)
	log.Printf("Server Certificate: %s", tlsConfig.ServerCertFile)

	if err := server.ListenAndServeTLS(tlsConfig.ServerCertFile, tlsConfig.ServerKeyFile); err != nil {
		log.Fatal("Server failed to start:", err)
	}

}
