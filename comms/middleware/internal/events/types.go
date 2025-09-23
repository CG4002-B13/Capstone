package events

type ActionType string
type CommandType string
type ObjectType string
type StatusType string

const (
	// Gesture Actions
	MOVE       ActionType = "MOVE"
	ROTATE     ActionType = "ROTATE"
	SELECT     ActionType = "SELECT"
	SCREENSHOT ActionType = "SCREENSHOT"

	// Voice Status
	SUCCESS StatusType = "SUCCESS"
	FAILED  StatusType = "FAILED"

	// Voice Commands
	CREATE CommandType = "CREATE"
	DELETE CommandType = "DELETE"

	// Objects
	TABLE ObjectType = "TABLE"
	CHAIR ObjectType = "CHAIR"
	LAMP  ObjectType = "LAMP"
	TV    ObjectType = "TV"
	BED   ObjectType = "BED"
	PLANT ObjectType = "PLANT"
)

// /Gestures
type GestureCommand struct {
	Type ActionType `json:"type"`
	Axes []float64  `json:"axes"`
}

type SuccessInfo struct {
	Command CommandType `json:"command"`
	Object  ObjectType  `json:"object"`
}

type FailedInfo struct {
	Error string `json:"error"`
}

// /voice_result
type VoiceCommand struct {
	Status StatusType   `json:"status"`
	Info   *SuccessInfo `json:"info,omitempty"`
	Error  *FailedInfo  `json:"error,omitempty"`
}

// To publish to websocket
type WebsocketEvent struct {
	EventType string      `json:"eventType"`
	UserID    string      `json:"userId"`
	SessionID string      `json:"sessionId"`
	Timestamp int64       `json:"timestamp"`
	Data      interface{} `json:"data"`
}
