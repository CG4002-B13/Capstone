package websocket

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/s3"
	"github.com/ParthGandhiNUS/CG4002/internal/types"
)

var s3Service *s3.S3Service

func SetS3Service(s3 *s3.S3Service) {
	s3Service = s3
}

func HandleS3Request(client *WSClient, event *types.WebsocketEvent) {
	ctx := context.Background()

	switch event.EventType {
	case types.S3_UPLOAD_REQUEST:
		handleUploadURLRequest(ctx, client, event)
	case types.S3_SYNC_REQUEST:
		handleSyncRequest(ctx, client, event)
	case types.S3_DELETE_REQUEST:
		handleDeleteRequest(ctx, client, event)
	default:
		sendS3Error(client, event, "Invalid S3 Request Sent")
	}
}

func handleUploadURLRequest(ctx context.Context, client *WSClient, event *types.WebsocketEvent) {
	url, err := s3Service.GeneratePresignedUploadURL(
		ctx,
		fmt.Sprintf("%s/%d", event.SessionID, int(event.Timestamp)),
	)

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
		responseData["GETNames"] = missingOnClient
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
	client.Send <- data
}
