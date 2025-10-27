package websocket

import (
	"crypto/tls"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/auth"
	"github.com/gorilla/websocket"
)

// WSTLSConfig holds TLS cert paths for WebSocket server
type WSTLSConfig struct {
	CACertFile     string
	ServerCertFile string
	ServerKeyFile  string
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

// Setup TLS config
func SetupWebsocketTLS(config WSTLSConfig) (*tls.Config, error) {
	caCertPool, err := auth.LoadCACert(config.CACertFile)
	if err != nil {
		return nil, err
	}

	serverCert, err := auth.LoadKeyPair(config.ServerCertFile, config.ServerKeyFile)
	if err != nil {
		return nil, err
	}

	return &tls.Config{
		Certificates: []tls.Certificate{serverCert},
		ClientCAs:    caCertPool,
		ClientAuth:   tls.RequireAndVerifyClientCert,
	}, nil
}

// HTTP handler for WebSocket upgrade
func HandleWebsocket(hub *Hub, w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Websocket upgrade error: %v", err)
		return
	}

	clientCN := ""
	verified := false
	if r.TLS != nil && len(r.TLS.PeerCertificates) > 0 {
		clientCert := r.TLS.PeerCertificates[0]
		clientCN = clientCert.Subject.CommonName
		verified = r.TLS.HandshakeComplete

		log.Printf("Client certificate - CN: %s, Verified: %v", clientCN, verified)
		if verified {
			now := time.Now()
			if now.Before(clientCert.NotBefore) || now.After(clientCert.NotAfter) {
				log.Printf("Client certificate CN %s is not valid", clientCN)
				verified = false
			}
		}
	} else {
		log.Printf("No client certificate provided")
	}

	userID := strings.ToLower(r.URL.Query().Get("userId"))
	sessionID := strings.ToLower(r.URL.Query().Get("sessionId"))

	if userID == "" {
		log.Printf("Missing userId parameter")
		conn.Close()
		return
	}

	if sessionID == "" {
		sessionID = userID
	}

	if !verified {
		log.Printf("Rejecting unverified client %s", userID)
	}

	client := &WSClient{
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
