package mqtt

import (
	"encoding/json"
	"log"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/types"
	"github.com/ParthGandhiNUS/CG4002/internal/websocket"
	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var hub *websocket.Hub

func SetHub(h *websocket.Hub) {
	hub = h
}

// Reads messages from esp32/command and forward to WS
func HandleCommand(c mqtt.Client, m mqtt.Message) {
	go func() {
		var msg types.Command
		if err := json.Unmarshal(m.Payload(), &msg); err != nil {
			log.Printf("Failed to unmarshal message: %v", err)
			return
		}

		log.Printf("[esp32/command] %s", string(m.Payload()))

		var data any
		if len(msg.Axes) > 0 {
			data = msg.Axes
		} else {
			data = nil
		}

		event := types.WebsocketEvent{
			EventType: msg.Type.ToEventType(),
			UserID:    "*", // fill if needed
			SessionID: "",  // fill correct session
			Timestamp: time.Now().UnixMilli(),
			Data:      data, // the payload
		}

		// Create and push websocket event
		if hub != nil {
			hub.Broadcast(event)
		}
	}()
}

// HandleVoiceResult processes messages from ultra96/voice_result and forward to WS
func HandleVoiceResult(c mqtt.Client, m mqtt.Message) {
	go func() {
		var msg types.VoiceCommand
		if err := json.Unmarshal(m.Payload(), &msg); err != nil {
			log.Printf("Failed to unmarshal message: %v", err)
			return
		}

		if msg.Status == types.FAILED {
			return
		}

		log.Printf("[ultra96/voice_result] %s", string(m.Payload()))

		// Extract Command and Object and attach to data
		data := msg.Info
		event := types.WebsocketEvent{
			EventType: types.COMMAND_SPEECH,
			UserID:    "*", // fill if needed
			SessionID: "",  // fill correct session
			Timestamp: time.Now().UnixMilli(),
			Data:      data, // the payload
		}

		// Create and push websocket event
		if hub != nil {
			hub.Broadcast(event)
		}
	}()
}
