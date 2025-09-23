package websocket

import (
	"log"
	"sync"
)

// Hub manages all websocket clients and sessions
type Hub struct {
	sessions     map[string]map[*WSClient]bool
	userSessions map[string][]*WSClient
	broadcast    chan BroadcastMessage
	register     chan *WSClient
	unregister   chan *WSClient
	mutex        sync.RWMutex
}

// BroadcastMessage holds a message for broadcasting to a session
type BroadcastMessage struct {
	SessionID string
	Data      []byte
	Sender    *WSClient
	EventType string
}

// Create a new hub
func NewHub() *Hub {
	return &Hub{
		sessions:     make(map[string]map[*WSClient]bool),
		userSessions: make(map[string][]*WSClient),
		broadcast:    make(chan BroadcastMessage),
		register:     make(chan *WSClient),
		unregister:   make(chan *WSClient),
	}
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
