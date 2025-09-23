package config

import (
	"log"
	"os"
	"strconv"

	"github.com/joho/godotenv"
)

type Config struct {
	CACertLocation     string
	ServerCertLocation string
	ServerKeyLocation  string
	ClientCertLocation string
	ClientKeyLocation  string
	WebSocketPort      string
	MQTTHost           string
	MQTTPort           int
	MQTTUser           string
	MQTTPass           string
}

func LoadConfig() *Config {
	err := godotenv.Load()
	if err != nil {
		log.Println("Error loading .env file")
	}

	caCert := os.Getenv("CA_CERT")
	serverCert := os.Getenv("SERVER_CERT")
	serverKey := os.Getenv("SERVER_KEY")
	clientCert := os.Getenv("CLIENT_CERT")
	clientKey := os.Getenv("CLIENT_KEY")
	WSPort := os.Getenv("WS_PORT")
	mqttHost := os.Getenv("MQTT_HOST")
	mqttPortStr := os.Getenv("MQTT_PORT")
	mqttUser := os.Getenv("MQTT_USER")
	mqttPass := os.Getenv("MQTT_PASS")

	if caCert == "" {
		caCert = "../secrets/dev/devices-ca.crt"
	}

	if serverCert == "" {
		serverCert = "../secrets/dev/devices-server.crt"
	}

	if serverKey == "" {
		serverKey = "../secrets/dev/devices-server.key"
	}

	if clientCert == "" {
		clientCert = "../secrets/devs/devices-server.crt"
	}

	if clientKey == "" {
		clientKey = "../secrets/dev/devices-server.key"
	}

	if WSPort == "" {
		WSPort = "8443"
	}

	if mqttHost == "" {
		mqttHost = "127.0.0.1"
	}

	if mqttPortStr == "" {
		mqttPortStr = "1883"
	}

	mqttPort, err := strconv.Atoi(mqttPortStr)
	if err != nil {
		log.Fatalf("Invalid MQTT_PORT: %v", err)
	}

	return &Config{
		CACertLocation:     caCert,
		ServerCertLocation: serverCert,
		ServerKeyLocation:  serverKey,
		ClientCertLocation: clientCert,
		ClientKeyLocation:  clientKey,
		WebSocketPort:      WSPort,
		MQTTHost:           mqttHost,
		MQTTPort:           mqttPort,
		MQTTUser:           mqttUser,
		MQTTPass:           mqttPass,
	}
}
