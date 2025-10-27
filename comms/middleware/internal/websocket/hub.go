package websocket

import (
	"encoding/json"
	"log"
	"sync"

	"github.com/ParthGandhiNUS/CG4002/internal/types"
)

var hub *Hub

// Hub manages all websocket clients and sessions
type Hub struct {
	sessions      map[string]map[*WSClient]bool
	sessionMaster map[string]*WSClient
	broadcast     chan types.WebsocketEvent
	register      chan *WSClient
	unregister    chan *WSClient
	mutex         sync.RWMutex
}

// Create a new hub
func NewHub() *Hub {
	hub = &Hub{
		sessions:      make(map[string]map[*WSClient]bool),
		sessionMaster: make(map[string]*WSClient),
		broadcast:     make(chan types.WebsocketEvent),
		register:      make(chan *WSClient),
		unregister:    make(chan *WSClient),
	}
	return hub
}

// Run the hub to process register/unregister/broadcast events
func (h *Hub) Run() {
	for {
		select {
		case client := <-h.register:
			h.registerClient(client)
		case client := <-h.unregister:
			h.unregisterClient(client)
		case message := <-h.broadcast:
			h.broadcastMessage(message)
		}
	}
}

// Helper function to register clients to server
func (h *Hub) registerClient(client *WSClient) {
	h.mutex.Lock()
	defer h.mutex.Unlock()

	// Make session if session does not exist
	if h.sessions[client.SessionID] == nil {
		h.sessions[client.SessionID] = make(map[*WSClient]bool) // Add client to session
		h.sessionMaster[client.SessionID] = client              // Add client as master
		client.IsMaster = true
		log.Printf("(User: %s) is now MASTER of session %s", client.UserID, client.SessionID)
	} else {
		client.IsMaster = false
	}

	h.sessions[client.SessionID][client] = true

	log.Printf("(User: %s, CN: %s) joined room %s. Session size: %v",
		client.UserID, client.ClientCN, client.SessionID, len(h.sessions[client.SessionID]),
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

		if client.IsMaster {
			log.Printf("MASTER has left session %s. Closing all clientt conenctions.", client.SessionID)
			for remainingClient := range clients {
				close(remainingClient.Send) // Close client send channel
				remainingClient.Conn.Close()

				log.Printf("Closed client user: %s due to master disconnect", remainingClient.UserID)
			}

			delete(h.sessions, client.SessionID)
			delete(h.sessionMaster, client.SessionID)

			log.Printf("Session %s terminated due to master disconnect", client.SessionID)
			return
		}

		// Clean up empty sessions
		if len(clients) == 0 {
			delete(h.sessions, client.SessionID)
		}

		log.Printf("User: %s has left session %s", client.UserID, client.SessionID)
	}
}

// Helper function to send message to all clients connected to the server
func (h *Hub) broadcastMessage(event types.WebsocketEvent) {
	h.mutex.RLock()
	defer h.mutex.RUnlock()

	data, _ := json.Marshal(event)

	// Send to all clients
	if event.UserID == "*" {
		for _, client := range h.sessionMaster {
			select {
			case client.Send <- data:
			default:
				close(client.Send)
				if clients, ok := h.sessions[client.SessionID]; ok {
					delete(clients, client)
				}
			}
		}
		return
	}

	if clients, ok := h.sessions[event.SessionID]; ok {
		for client := range clients {
			if client.UserID == event.UserID {
				continue // dont need to send itself
			}

			select {
			case client.Send <- data:
			default:
				close(client.Send)
				delete(clients, client)
			}
		}
	}
}

// Function to send message to broadcast channel
func (h *Hub) Broadcast(event types.WebsocketEvent) {
	h.broadcast <- event
}
