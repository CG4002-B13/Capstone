package types

import (
	"strings"
)

type CommandType string
type VoiceCommandType string
type ResultType string
type StatusType string
type EventType string
type TimeField string

const (
	// Command Type
	MOVE       CommandType = "MOVE"
	ROTATE     CommandType = "ROTATE"
	SCREENSHOT CommandType = "SCREENSHOT"
	DEBUG      CommandType = "DEBUG"

	// Voice Command Types
	SELECT VoiceCommandType = "SELECT"
	DELETE VoiceCommandType = "DELETE"

	// Voice Status
	SUCCESS     StatusType = "SUCCESS"
	FAILED      StatusType = "FAILED"
	VOICE_DEBUG StatusType = "DEBUG"

	// Results from Voice Recognition
	TABLE     ResultType = "TABLE"
	CHAIR     ResultType = "CHAIR"
	LAMP      ResultType = "LAMP"
	TV        ResultType = "TV"
	BED       ResultType = "BED"
	PLANT     ResultType = "PLANT"
	SOFA      ResultType = "SOFA"
	ODM       ResultType = "ODM"
	DETECTION ResultType = "DETECTION"
	TOGGLE    ResultType = "TOGGLE"
	UP        ResultType = "UP"
	DOWN      ResultType = "DOWN"

	// Event Type
	COMMAND_SELECT     EventType = "COMMAND_SELECT"
	COMMAND_DELETE     EventType = "COMMAND_DELETE"
	COMMAND_MOVE       EventType = "COMMAND_MOVE"
	COMMAND_ROTATE     EventType = "COMMAND_ROTATE"
	COMMAND_SCREENSHOT EventType = "COMMAND_SCREENSHOT"
	COMMAND_SET        EventType = "COMMAND_SET"
	COMMAND_DEBUG      EventType = "COMMAND_DEBUG"
	DEBUG_VIDEO_PING   EventType = "DEBUG_VIDEO_PING"
	DEBUG_VIDEO_PONG   EventType = "DEBUG_VIDEO_PONG"
	DEBUG_GESTURE_PING EventType = "DEBUG_GESTURE_PING"
	DEBUG_GESTURE_PONG EventType = "DEBUG_GESTURE_PONG"
	SCREENSHOT_SEND    EventType = "SCREENSHOT_SEND"
	SCREENSHOT_RECEIVE EventType = "SCREENSHOT_RECEIVE"
	S3_UPLOAD_REQUEST  EventType = "S3_UPLOAD_REQUEST"
	S3_UPLOAD_RESPONSE EventType = "S3_UPLOAD_RESPONSE"
	S3_SYNC_REQUEST    EventType = "S3_SYNC_REQUEST"
	S3_SYNC_RESPONSE   EventType = "S3_SYNC_RESPONSE"
	S3_DELETE_REQUEST  EventType = "S3_DELETE_REQUEST"
	S3_DELETE_RESPONSE EventType = "S3_DELETE_RESPONSE"
	S3_ERROR           EventType = "S3_ERROR"

	// For Latency Testing
	INITIAL_MQTT_TIME   TimeField = "Original MQTT Time"
	ESP32_TO_SERVER     TimeField = "ESP32 To Server"
	ESP32_TO_ULTRA96    TimeField = "ESP32 To Ultra96"
	ULTRA96_TO_SERVER   TimeField = "Ultra96 To Server"
	SERVER_TO_VIS       TimeField = "Server To Visualiser"
	INFERENCE_TIME      TimeField = "Inference Time"
	INITIAL_SERVER_TIME TimeField = "Original Server Time"
	END_TO_END_GESTURE  TimeField = "End To End Gesture"
	END_TO_END_VOICE    TimeField = "End To End Voice"
)

func (c CommandType) ToEventType() EventType {
	return EventType("COMMAND_" + string(c))
}

func (v VoiceCommandType) ToEventType() EventType {
	return EventType("COMMAND_" + string(v))
}

func (e EventType) IsS3Event() bool {
	return strings.HasPrefix(string(e), "S3_")
}

func (e EventType) IsDebugEvent() bool {
	return strings.HasPrefix(string(e), "DEBUG_")
}

// esp32/command
type Command struct {
	Type      CommandType `json:"type"`
	Axes      []float64   `json:"axes,omitempty"`
	Timestamp int64       `json:"timestamp,omitempty"`
}

type SuccessInfo struct {
	Command VoiceCommandType `json:"command"`
	Result  ResultType       `json:"result"`
}

type FailedInfo struct {
	Error string `json:"error"`
}

type DebugInfo struct {
	Ultra96ReceiveTime int64 `json:"receiveTime"`
	InferenceTime      int64 `json:"inferenceTime"`
	Ultra96SendTime    int64 `json:"sendTime"`
}

// ultra96/voice_result
type VoiceCommand struct {
	Status StatusType   `json:"status"`
	Info   *SuccessInfo `json:"info,omitempty"`
	Error  *FailedInfo  `json:"error,omitempty"`
}

type VoiceDebug struct {
	Status StatusType `json:"status"`
	Info   DebugInfo  `json:"info"`
}

// To publish to websocket
type WebsocketEvent struct {
	EventType EventType   `json:"eventType"`
	UserID    string      `json:"userId"`
	SessionID string      `json:"sessionId"`
	Timestamp int64       `json:"timestamp"`
	Data      interface{} `json:"data"`
}

type LatencyInfo struct {
	ESP32ToServer      string `json:"Latency Between ESP32 and Server"`
	ESP32ToUltra96     string `json:"Latency Between ESP32 and Ultra96"`
	Ultra96ToServer    string `json:"Latency Between Ultra96 and Server"`
	ServerToVisualiser string `json:"Latency Between server to visualiser"`
	InferenceTime      string `json:"Inference Time"`
	EndToEndGesture    string `json:"E2E Latency for Gesture"`
	EndToEndVoice      string `json:"E2E Latency for Voices"`
}
