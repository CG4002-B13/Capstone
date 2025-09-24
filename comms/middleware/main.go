package main

import (
	"log"
	"net/http"

	"github.com/ParthGandhiNUS/CG4002/config"
	"github.com/ParthGandhiNUS/CG4002/internal/events"
	"github.com/ParthGandhiNUS/CG4002/internal/mqttwrapper"
	"github.com/ParthGandhiNUS/CG4002/internal/websocket"
	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func main() {
	env := config.LoadConfig()

	wsTLSConfig := websocket.WSTLSConfig{
		CACertFile:     env.CACertLocation,
		ServerCertFile: env.ServerCertLocation,
		ServerKeyFile:  env.ServerKeyLocation,
	}

	mqttTLSConfig := mqttwrapper.MQTTTLSConfig{
		CACertFile:     env.CACertLocation,
		ClientCertFile: env.ClientCertLocation,
		ClientKeyFile:  env.ClientKeyLocation,
	}

	wsTLSConf, err := websocket.SetupWebsocketTLS(wsTLSConfig)
	if err != nil {
		log.Fatal("Failed to setup TLS:", err)
	}

	mqttTLSConf, err := mqttwrapper.SetupMQTTTLS(mqttTLSConfig)
	if err != nil {
		log.Fatal("Failed to authenticate with MQTT Broker: ", err)
	}

	hub := websocket.NewHub()
	go hub.Run()
	events.SetHub(hub)

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		websocket.HandleWebsocket(hub, w, r)
	})

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == "OPTIONS" {
			return
		}

		w.WriteHeader(http.StatusNotFound)
	})

	server := &http.Server{
		Addr:      ":" + env.WebSocketPort,
		TLSConfig: wsTLSConf,
	}

	log.Printf("Starting secure WebSocket server on port :%s", env.WebSocketPort)
	log.Printf("Secure WebSocket endpoint: wss://localhost:%s/ws?userId=<USER_ID>&sessionId=<SESSION_ID>", env.WebSocketPort)
	log.Printf("CA Certificate: %s", wsTLSConfig.CACertFile)
	log.Printf("Server Certificate: %s", wsTLSConfig.ServerCertFile)

	go func() {
		if err := server.ListenAndServeTLS(wsTLSConfig.ServerCertFile, wsTLSConfig.ServerKeyFile); err != nil {
			log.Fatal("Server failed to start:", err)
		}
	}()

	mqttClient := mqttwrapper.NewMQTTClient("GolangService", env.MQTTHost, env.MQTTPort, env.MQTTUser, env.MQTTPass, mqttTLSConf)

	handlers := map[string]mqtt.MessageHandler{
		"/gestures":     events.HandleGestures,
		"/voice_result": events.HandleVoiceResult,
	}

	for topic, handler := range handlers {
		mqttClient.Subscribe(topic, 0, handler)
	}

	select {}

}
