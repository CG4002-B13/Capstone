package websocket

import (
	"encoding/json"
	"log"
	"strings"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/types"
	"github.com/gorilla/websocket"
)

// WSClient represents a connected client
type WSClient struct {
	UserID    string
	SessionID string
	Conn      *websocket.Conn
	Send      chan []byte
	Hub       *Hub
	ClientCN  string
	IsMaster  bool
}

// Read messages from client and forward to hub
func (c *WSClient) readPump() {
	defer func() {
		c.Hub.unregister <- c
		c.Conn.Close()
	}()
	c.Conn.SetReadLimit(512 * 1024)
	c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
	c.Conn.SetPongHandler(func(string) error {
		c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
		return nil
	})

	for {
		_, message, err := c.Conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("WebSocket error for client %s: %v", c.UserID, err)
			}
			break
		}

		log.Printf("Raw message from client %s: %s", c.UserID, string(message))

		var websocketEvent types.WebsocketEvent
		if err := json.Unmarshal(message, &websocketEvent); err != nil {
			log.Printf("Invalid message format from client %s: %v", c.UserID, err)
			continue
		}

		websocketEvent.UserID = strings.ToLower(c.UserID)
		websocketEvent.SessionID = strings.ToLower(c.SessionID)

		if websocketEvent.EventType.IsS3Event() {
			HandleS3Request(c, &websocketEvent)
		} else if websocketEvent.EventType.IsDebugEvent() {
			handleDebugResponse(&websocketEvent)
		} else {
			c.Hub.broadcast <- websocketEvent
		}
	}
}

// Write messages to client
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
				log.Printf("Write error for client %s: %v", c.UserID, err)
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
