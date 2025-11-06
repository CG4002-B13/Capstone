package mqtt

import (
	"encoding/json"
	"log"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/debug"
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
		payload := m.Payload()

		var base struct {
			Status types.StatusType `json:"status"`
		}
		if err := json.Unmarshal(payload, &base); err != nil {
			log.Printf("Failed to unmarshal message: %v", err)
			return
		}

		switch base.Status {
		case types.VOICE_DEBUG:
			var msg types.DebugInfo
			if err := json.Unmarshal(payload, &msg); err != nil {
				log.Printf("Failed to unmarshal message: %v", err)
				return
			}

			if !debug.IsDebugActive() {
				log.Printf("Debug Session not started")
				SendDebugStatus("Debug session not started")
			}

			currTime := time.Now().UnixMilli()
			ultra96ToServer := currTime - msg.Ultra96SendTime
			esp32ToUltra96 := msg.Ultra96ReceiveTime - debug.GetData(types.ORIGINAL_TIME)

			debug.AddData()

		case types.SUCCESS:
			var msg types.VoiceCommand
			if err := json.Unmarshal(payload, &msg); err != nil {
				log.Printf("Failed to unmarshal message: %v", err)
				return
			}

			var eventType types.EventType
			data := msg.Info
			switch data.Result {
			case types.ODM, types.UP, types.DOWN:
				eventType = types.COMMAND_SET
			default:
				eventType = msg.Info.Command.ToEventType()
			}

			log.Printf("[ultra96/voice_result] %s", string(m.Payload()))

			// Extract Command and Object and attach to data
			event := types.WebsocketEvent{
				EventType: eventType,
				UserID:    "*", // fill if needed
				SessionID: "",  // fill correct session
				Timestamp: time.Now().UnixMilli(),
				Data:      data, // the payload
			}

			// Create and push websocket event
			if hub != nil {
				hub.Broadcast(event)
			}
		default:
			return
		}

	}()
}

func SendDebugStatus(m interface{}) {
	var payload []byte
	var err error

	switch v := m.(type) {
	case string:
		payload = []byte(v)
	case map[string]interface{}:
		payload, err = json.Marshal(v)
		if err != nil {
			// handle error, maybe log and return
			return
		}
	default:
		// unsupported type
		return
	}

	client.Publish("debug/status", 0, false, payload)
}
