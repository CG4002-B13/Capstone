package internal

import (
	"crypto/tls"
	"fmt"
	"log"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type MQTTClient struct {
	Client mqtt.Client
	Host   string
	Port   int
	User   string
	Pass   string
}

type MQTTTLSConfig struct {
	CACertFile     string
	ClientCertFile string
	ClientKeyFile  string
}

func SetupMQTTTLS(config MQTTTLSConfig) (*tls.Config, error) {
	caCertPool, err := LoadCACert(config.CACertFile)
	if err != nil {
		return nil, err
	}

	clientCert, err := LoadKeyPair(config.ClientCertFile, config.ClientKeyFile)
	if err != nil {
		return nil, err
	}

	return &tls.Config{
		RootCAs:      caCertPool,
		Certificates: []tls.Certificate{clientCert},
		MinVersion:   tls.VersionTLS12,
	}, nil
}

func NewMQTTClient(clientID string, host string, port int, user string, pass string, tlsConfig *tls.Config) *MQTTClient {
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("ssl://%s:%d", host, port))
	opts.SetClientID(clientID)
	if user != "" {
		opts.SetUsername(user)
		opts.SetPassword(pass)
	}

	opts.SetTLSConfig(tlsConfig)
	opts.SetKeepAlive(2 * time.Second)
	opts.SetPingTimeout(1 * time.Second)
	opts.OnConnect = func(c mqtt.Client) {
		log.Printf("Connected to MQTT broker at %s:%v", host, port)
	}
	opts.OnConnectionLost = func(c mqtt.Client, err error) {
		log.Printf("Connection lost: %v\n", err)
	}

	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Fatalf("Failed to connect to MQTT Broker: %v", token.Error())
	}

	return &MQTTClient{
		Client: client,
		Host:   host,
		Port:   port,
		User:   user,
		Pass:   pass,
	}
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
