package types

type ActionType string
type CommandType string
type VoiceCommandType string
type ObjectType string
type StatusType string
type EventType string

const (
	// Gesture Actions Type
	MOVE   ActionType = "MOVE"
	ROTATE ActionType = "ROTATE"

	// Command Types
	START      CommandType = "START"
	STOP       CommandType = "STOP"
	SCREENSHOT CommandType = "SCREENSHOT"

	// Voice Command Types
	SELECT VoiceCommandType = "SELECT"
	DELETE VoiceCommandType = "DELETE"

	// Voice Status
	SUCCESS StatusType = "SUCCESS"
	FAILED  StatusType = "FAILED"

	// Objects
	TABLE ObjectType = "TABLE"
	CHAIR ObjectType = "CHAIR"
	LAMP  ObjectType = "LAMP"
	TV    ObjectType = "TV"
	BED   ObjectType = "BED"
	PLANT ObjectType = "PLANT"

	// Event Type
	COMMAND_SELECT     EventType = "COMMAND_SELECT"
	COMMAND_DELETE     EventType = "COMMAND_DELETE"
	COMMAND_MOVE       EventType = "COMMAND_MOVE"
	COMMAND_ROTATE     EventType = "COMMAND_ROTATE"
	COMMAND_START      EventType = "COMMAND_START"
	COMMAND_STOP       EventType = "COMMAND_STOP"
	COMMAND_SCREENSHOT EventType = "COMMAND_SCREENSHOT"
	COMMAND_SPEECH     EventType = "COMMAND_SPEECH"
	SCREENSHOT_SEND    EventType = "SCREENSHOT_SEND"
	SCREENSHOT_RECEIVE EventType = "SCREENSHOT_RECEIVE"
)

func (a ActionType) ToEventType() EventType {
	return EventType("COMMAND_" + string(a))
}

func (c CommandType) ToEventType() EventType {
	return EventType("COMMAND_" + string(c))
}

func (v VoiceCommandType) ToEventType() EventType {
	return EventType("COMMAND_" + string(v))
}

// esp32/gesture_data
type GestureCommand struct {
	Type ActionType `json:"type"`
	Axes []float64  `json:"axes"`
}

// esp32/command
type Command struct {
	Type CommandType `json:"type"`
}

type SuccessInfo struct {
	Command VoiceCommandType `json:"command"`
	Object  ObjectType       `json:"object"`
}

type FailedInfo struct {
	Error string `json:"error"`
}

// ultra96/voice_result
type VoiceCommand struct {
	Status StatusType   `json:"status"`
	Info   *SuccessInfo `json:"info,omitempty"`
	Error  *FailedInfo  `json:"error,omitempty"`
}

// To publish to websocket
type WebsocketEvent struct {
	EventType EventType   `json:"eventType"`
	UserID    string      `json:"userId"`
	SessionID string      `json:"sessionId"`
	Timestamp int64       `json:"timestamp"`
	Data      interface{} `json:"data"`
}
