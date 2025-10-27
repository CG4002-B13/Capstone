package types

import "strings"

type CommandType string
type VoiceCommandType string
type ResultType string
type StatusType string
type EventType string

const (
	// Command Type
	MOVE       CommandType = "MOVE"
	ROTATE     CommandType = "ROTATE"
	SCREENSHOT CommandType = "SCREENSHOT"

	// Voice Command Types
	SELECT VoiceCommandType = "SELECT"
	DELETE VoiceCommandType = "DELETE"

	// Voice Status
	SUCCESS StatusType = "SUCCESS"
	FAILED  StatusType = "FAILED"

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
	COMMAND_SPEECH     EventType = "COMMAND_SPEECH"
	SCREENSHOT_SEND    EventType = "SCREENSHOT_SEND"
	SCREENSHOT_RECEIVE EventType = "SCREENSHOT_RECEIVE"
	S3_UPLOAD_REQUEST  EventType = "S3_UPLOAD_REQUEST"
	S3_UPLOAD_RESPONSE EventType = "S3_UPLOAD_RESPONSE"
	S3_SYNC_REQUEST    EventType = "S3_SYNC_REQUEST"
	S3_SYNC_RESPONSE   EventType = "S3_SYNC_RESPONSE"
	S3_DELETE_REQUEST  EventType = "S3_DELETE_REQUEST"
	S3_DELETE_RESPONSE EventType = "S3_DELETE_RESPONSE"
	S3_ERROR           EventType = "S3_ERROR"
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

// esp32/command
type Command struct {
	Type CommandType `json:"type"`
	Axes []float64   `json:"axes,omitempty"`
}

type SuccessInfo struct {
	Command VoiceCommandType `json:"command"`
	Result  ResultType       `json:"result"`
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
