package websocket

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/debug"
	"github.com/ParthGandhiNUS/CG4002/internal/s3"
	"github.com/ParthGandhiNUS/CG4002/internal/types"
)

var s3Service *s3.S3Service

func SetS3Service(s3 *s3.S3Service) {
	s3Service = s3
}

func HandleS3Request(client *WSClient, event *types.WebsocketEvent) {
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)

	switch event.EventType {
	case types.S3_UPLOAD_REQUEST:
		go func() {
			defer cancel()
			handleUploadURLRequest(ctx, client, event)
		}()
	case types.S3_SYNC_REQUEST:
		go func() {
			defer cancel()
			handleSyncRequest(ctx, client, event)
		}()
	case types.S3_DELETE_REQUEST:
		go func() {
			defer cancel()
			handleDeleteRequest(ctx, client, event)
		}()

	default:
		cancel()
		log.Printf("Unknown S3 Event Type: %s", event.EventType)
		sendS3Error(client, event, "Invalid S3 Request Sent")
	}
}

func handleUploadURLRequest(ctx context.Context, client *WSClient, event *types.WebsocketEvent) {
	url, err := s3Service.GeneratePresignedUploadURL(
		ctx,
		fmt.Sprintf("%s/%d", event.SessionID, int(event.Timestamp)),
	)

	if ctx.Err() == context.DeadlineExceeded {
		log.Printf("Request timed out: %v", err)
		sendS3Error(client, event, "Request timed out")
		return
	}

	if err != nil {
		log.Println(err)
		sendS3Error(client, event, err.Error())
		return
	}

	response := &types.WebsocketEvent{
		EventType: types.S3_UPLOAD_RESPONSE,
		UserID:    client.UserID,
		SessionID: event.SessionID,
		Timestamp: time.Now().UnixMilli(),
		Data:      url,
	}
	sendS3Response(client, response)
}

func handleSyncRequest(ctx context.Context, client *WSClient, event *types.WebsocketEvent) {
	s3List, err := s3Service.ListUserImages(ctx, event.SessionID)

	if ctx.Err() == context.DeadlineExceeded {
		log.Printf("Request timed out: %v", err)
		sendS3Error(client, event, "Request timed out")
		return
	}

	if err != nil {
		log.Println(err)
		sendS3Error(client, event, err.Error())
		return
	}

	var clientList []string
	bytes, _ := json.Marshal(event.Data)
	if err := json.Unmarshal(bytes, &clientList); err != nil {
		log.Printf("Failed to convert data to []string format: %v", err)
	}

	missingInS3, missingOnClient := s3Service.CompareWithS3(clientList, s3List)

	var listGet, listPut []string
	responseData := make(map[string]interface{})

	for _, file := range missingInS3 {
		url, err := s3Service.GeneratePresignedUploadURL(ctx, file)

		if err != nil {
			log.Println(err)
			sendS3Error(client, event, err.Error())
			return
		}

		listPut = append(listPut, url)
		responseData["PUT"] = listPut
		responseData["PUTNames"] = missingInS3
	}

	for _, file := range missingOnClient {
		url, err := s3Service.GeneratePresignedDownloadURL(ctx, file)

		if err != nil {
			log.Println(err)
			sendS3Error(client, event, err.Error())
			return
		}

		listGet = append(listGet, url)
		responseData["GET"] = listGet
	}

	response := &types.WebsocketEvent{
		EventType: types.S3_SYNC_RESPONSE,
		UserID:    client.UserID,
		SessionID: event.SessionID,
		Timestamp: time.Now().UnixMilli(),
		Data:      responseData,
	}
	sendS3Response(client, response)
}

func handleDeleteRequest(ctx context.Context, client *WSClient, event *types.WebsocketEvent) {
	url, err := s3Service.GeneratePresignedDeleteURL(ctx, event.Data.(string))

	if ctx.Err() == context.DeadlineExceeded {
		log.Printf("Request timed out: %v", err)
		sendS3Error(client, event, "Request timed out")
		return
	}

	if err != nil {
		log.Println(err)
		sendS3Error(client, event, err.Error())
		return
	}

	response := &types.WebsocketEvent{
		EventType: types.S3_DELETE_RESPONSE,
		UserID:    client.UserID,
		SessionID: event.SessionID,
		Timestamp: time.Now().UnixMilli(),
		Data:      url,
	}
	sendS3Response(client, response)
}

func sendS3Error(client *WSClient, event *types.WebsocketEvent, errorMsg string) {
	response := &types.WebsocketEvent{
		EventType: types.S3_ERROR,
		UserID:    client.UserID,
		SessionID: event.SessionID,
		Timestamp: time.Now().UnixMilli(),
		Data:      errorMsg,
	}
	sendS3Response(client, response)
}

func sendS3Response(client *WSClient, response *types.WebsocketEvent) {
	data, err := json.Marshal(response)
	if err != nil {
		log.Printf("Failed to marshal response: %v", err)
		return
	}

	select {
	case client.Send <- data:
	case <-time.After(5 * time.Second):
		log.Printf("Failed to send response - client send channel timeout")
	}
}

func handleDebugResponse(event *types.WebsocketEvent) {
	timestamp := event.Timestamp
	serverInitialTime := debug.GetData(types.INITIAL_SERVER_TIME)
	mqttInitialTime := debug.GetData(types.INITIAL_MQTT_TIME)

	// if serverInitialTime == 0 || mqttInitialTime == 0 {
	// 	log.Printf("No initial debug time set")
	// 	return
	// }

	debug.AddData(types.SERVER_TO_VIS, timestamp-serverInitialTime)

	switch event.EventType {
	case types.DEBUG_GESTURE_PONG:
		debug.AddData(types.END_TO_END_GESTURE, timestamp-mqttInitialTime)
	case types.DEBUG_VIDEO_PONG:
		debug.AddData(types.END_TO_END_VOICE, timestamp-mqttInitialTime)
	default:
		log.Printf("Invalid Debug Response Sent")
		return
	}

}
