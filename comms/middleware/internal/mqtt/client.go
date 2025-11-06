package mqtt

import (
	"crypto/tls"
	"fmt"
	"log"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/debug"
	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var client *MQTTClient

type MQTTClient struct {
	Client mqtt.Client
	Host   string
	Port   int
	User   string
	Pass   string
}

// NewMQTTClient creates a connected MQTT client with optional TLS
func NewMQTTClient(clientID, host string, port int, user, pass string, tlsConfig *tls.Config) *MQTTClient {
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("ssl://%s:%d", host, port))
	opts.SetClientID(clientID)
	if user != "" {
		opts.SetUsername(user)
		opts.SetPassword(pass)
	}

	if tlsConfig != nil {
		opts.SetTLSConfig(tlsConfig)
	}

	opts.SetKeepAlive(2 * time.Second)
	opts.SetPingTimeout(1 * time.Second)
	opts.OnConnect = func(c mqtt.Client) {
		log.Printf("Connected to MQTT broker at %s:%d", host, port)
	}
	opts.OnConnectionLost = func(c mqtt.Client, err error) {
		log.Printf("Connection lost: %v", err)
	}

	c := mqtt.NewClient(opts)
	if token := c.Connect(); token.Wait() && token.Error() != nil {
		log.Fatalf("Failed to connect to MQTT Broker: %v", token.Error())
	}

	// Initialise callback for Latency Test
	debug.RegisterSendDebugCallback(SendDebugStatus)

	client = &MQTTClient{
		Client: c,
		Host:   host,
		Port:   port,
		User:   user,
		Pass:   pass,
	}

	return client
}
