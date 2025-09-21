package main

import (
	"log"
	"net/http"

	"github.com/ParthGandhiNUS/CG4002/config"
	"github.com/ParthGandhiNUS/CG4002/internal"
)

func main() {
	envConfig := config.LoadConfig()

	tlsConfig := internal.TLSConfig{
		CACertFile:     envConfig.CACertLocation,
		ServerCertFile: envConfig.ServerCertLocation,
		ServerKeyFile:  envConfig.ServerKeyLocation,
	}

	tlsConf, err := internal.SetupTLSConfig(tlsConfig)
	if err != nil {
		log.Fatal("Failed to setup TLS:", err)
	}

	hub := internal.NewHub()
	go hub.Run()

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		internal.HandleWebsocket(hub, w, r)
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
		Addr:      ":" + envConfig.WebSocketPort,
		TLSConfig: tlsConf,
	}

	log.Printf("Starting secure WebSocket server on port :%s", envConfig.WebSocketPort)
	log.Printf("Secure WebSocket endpoint: wss://localhost:%s/ws?userId=<USER_ID>&sessionId=<SESSION_ID>", envConfig.WebSocketPort)
	log.Printf("CA Certificate: %s", tlsConfig.CACertFile)
	log.Printf("Server Certificate: %s", tlsConfig.ServerCertFile)

	if err := server.ListenAndServeTLS(tlsConfig.ServerCertFile, tlsConfig.ServerKeyFile); err != nil {
		log.Fatal("Server failed to start:", err)
	}
}
