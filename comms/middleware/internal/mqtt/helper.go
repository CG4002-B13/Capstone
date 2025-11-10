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

		var event types.WebsocketEvent
		switch msg.Type {
		case types.DEBUG:
			log.Printf("Detected type debug")
			debug.StartDebugSession()
			debug.AddData(types.INITIAL_SERVER_TIME, time.Now().UnixMilli())
			debug.AddData(types.INITIAL_MQTT_TIME, msg.Timestamp)
			debug.AddData(types.ESP32_TO_SERVER, time.Now().UnixMilli()-msg.Timestamp)

			event = types.WebsocketEvent{
				EventType: types.DEBUG_GESTURE_PING,
				UserID:    "*",
				SessionID: "",
				Timestamp: time.Now().UnixMilli(),
				Data:      nil,
			}

		default:
			var data any
			if len(msg.Axes) > 0 {
				data = msg.Axes
			} else {
				data = nil
			}

			event = types.WebsocketEvent{
				EventType: msg.Type.ToEventType(),
				UserID:    "*", // fill if needed
				SessionID: "",  // fill correct session
				Timestamp: time.Now().UnixMilli(),
				Data:      data, // the payload
			}

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

		log.Printf("[ultra96/voice_result] %s", string(payload))

		var event types.WebsocketEvent
		switch base.Status {
		case types.VOICE_DEBUG:
			var data types.VoiceDebug
			if err := json.Unmarshal(payload, &data); err != nil {
				log.Printf("Failed to unmarshal message: %v", err)
				return
			}

			if !debug.IsDebugActive() {
				log.Printf("Debug Session not started")
				SendDebugStatus("Debug session not started")
			}

			initialTime := debug.GetData(types.INITIAL_MQTT_TIME)
			ultra96ToServer := time.Now().UnixMilli() - data.Info.Ultra96SendTime
			esp32ToUltra96 := data.Info.Ultra96ReceiveTime - initialTime
			infTime := data.Info.InferenceTime

			debug.AddData(types.ULTRA96_TO_SERVER, ultra96ToServer)
			debug.AddData(types.ESP32_TO_ULTRA96, esp32ToUltra96)
			debug.AddData(types.INFERENCE_TIME, infTime)

			event = types.WebsocketEvent{
				EventType: types.DEBUG_VIDEO_PING,
				UserID:    "*",
				SessionID: "",
				Timestamp: time.Now().UnixMilli(),
				Data:      nil,
			}

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

			// Extract Command and Object and attach to data
			event = types.WebsocketEvent{
				EventType: eventType,
				UserID:    "*", // fill if needed
				SessionID: "",  // fill correct session
				Timestamp: time.Now().UnixMilli(),
				Data:      data, // the payload
			}

		default:
			return
		}
		// Create and push websocket event
		if hub != nil {
			hub.Broadcast(event)
		}

	}()
}

func SendDebugStatus(m interface{}) {
	var payload []byte
	var err error

	switch v := m.(type) {
	case string:
		payload = []byte(v)
	case *types.LatencyInfo:
		payload, err = json.MarshalIndent(v, "", "  ")
		if err != nil {
			// handle error, maybe log and return
			log.Printf("Error marshalling JSON for debug status")
			return
		}
	default:
		// unsupported type
		log.Printf("Unsupported type for debug status")
		return
	}

	client.Publish("debug/status", 0, false, payload)
}
