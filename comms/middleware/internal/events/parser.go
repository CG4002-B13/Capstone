package events

import (
	"encoding/json"
	"log"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/events/types"
	"github.com/ParthGandhiNUS/CG4002/internal/websocket"
	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var hub *websocket.Hub

func SetHub(h *websocket.Hub) {
	hub = h
}

func HandleGestures(c mqtt.Client, m mqtt.Message) {
	go func() {
		var msg types.GestureCommand
		if err := json.Unmarshal(m.Payload(), &msg); err != nil {
			log.Printf("Failed to unmarshal message: %v", err)
			return
		}
		log.Printf("[/gestures] %s", string(m.Payload()))

		event := types.WebsocketEvent{
			EventType: types.COMMAND_GESTURE,
			UserID:    "*", // fill if needed
			SessionID: "",  // fill correct session
			Timestamp: time.Now().UnixMilli(),
			Data:      msg.Axes, // the payload
		}

		// Create and push websocket event
		if hub != nil {
			hub.Broadcast(event)
		}
	}()
}

// HandleVoiceResult processes messages from /voice_result
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

		log.Printf("[/voice_result] %s", string(m.Payload()))

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
