package config

import (
	"log"
	"os"

	"github.com/joho/godotenv"
)

type Config struct {
	CACertLocation     string
	ServerCertLocation string
	ServerKeyLocation  string
	WebSocketPort      string
}

func LoadConfig() *Config {
	err := godotenv.Load()
	if err != nil {
		log.Println("Error loading .env file")
	}

	caCert := os.Getenv("CA_CERT")
	serverCert := os.Getenv("SERVER_CERT")
	serverKey := os.Getenv("SERVER_KEY")
	WSPort := os.Getenv("WS_PORT")

	if caCert == "" {
		caCert = "../secrets/dev/visualiser-ca.crt"
	}

	if serverCert == "" {
		serverCert = "../secrets/dev/visualiser-server.crt"
	}

	if serverKey == "" {
		serverKey = "../secrets/dev/visualiser-server.key"
	}

	if WSPort == "" {
		WSPort = "8443"
	}

	return &Config{
		CACertLocation:     caCert,
		ServerCertLocation: serverCert,
		ServerKeyLocation:  serverKey,
		WebSocketPort:      WSPort,
	}
}
