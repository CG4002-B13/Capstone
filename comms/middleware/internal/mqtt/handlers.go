package mqtt

import (
	"crypto/tls"
	"log"

	"github.com/ParthGandhiNUS/CG4002/internal/auth"
	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type MQTTTLSConfig struct {
	CACertFile     string
	ClientCertFile string
	ClientKeyFile  string
}

// SetupMQTTTLS loads CA and client certs
func SetupMQTTTLS(config MQTTTLSConfig) (*tls.Config, error) {
	caCertPool, err := auth.LoadCACert(config.CACertFile)
	if err != nil {
		return nil, err
	}

	clientCert, err := auth.LoadKeyPair(config.ClientCertFile, config.ClientKeyFile)
	if err != nil {
		return nil, err
	}

	log.Printf("Loaded MQTT TLS certs: CA=%s, ClientCert=%s", config.CACertFile, config.ClientCertFile)

	return &tls.Config{
		RootCAs:      caCertPool,
		Certificates: []tls.Certificate{clientCert},
		MinVersion:   tls.VersionTLS12,
	}, nil
}

// Subscribe to a topic with a callback
func (m *MQTTClient) Subscribe(topic string, qos byte, callback mqtt.MessageHandler) {
	token := m.Client.Subscribe(topic, qos, callback)
	token.Wait()
	if token.Error() != nil {
		log.Fatalf("Subscribe error: %v", token.Error())
	}
	log.Printf("Subscribed to topic: %s", topic)
}

// Publish a message to a topic
func (m *MQTTClient) Publish(topic string, qos byte, retained bool, payload interface{}) {
	token := m.Client.Publish(topic, qos, retained, payload)
	token.Wait()
	if token.Error() != nil {
		log.Printf("Publish error: %v", token.Error())
	}
}
