package config

import (
	"os"
)

type Config struct {
	CACertLocation     string
	ServerCertLocation string
	ServerKeyLocation  string
	WebSocketPort      string
}

func LoadConfig() *Config {
	caCert := os.Getenv("CA_CERT")
	serverCert := os.Getenv("SERVER_CERT")
	serverKey := os.Getenv("SERVER_KEY")
	WSPort := os.Getenv("WS_PORT")

	if caCert == "" {
		caCert = "../secrets/visualiser-ca.crt"
	}

	if serverCert == "" {
		serverCert = "../secrets/visualiser-server.crt"
	}

	if serverKey == "" {
		serverKey = "../secrets/visualiser-server.key"
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
