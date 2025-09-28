import time
import ssl
from config import CERT_NAME, MODE
from pathlib import Path
import paho.mqtt.client as mqtt


class SecureMQTTClient:
    def __init__(self, username, password, host="127.0.0.1", port=8883, clientId=None):
        self.host = host
        self.port = port
        self.client = mqtt.Client(client_id=clientId)
        self.client.username_pw_set(username, password)

        self._ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self._intentional_disconnect = False

        self.client.on_connect = self._onConnect
        self.client.on_message = self._onMessage
        self.client.on_disconnect = self._onDisconnect

        self._loadCertificate()

    def _loadCertificate(self):
        secrets_dir = Path(__file__).resolve().parent.parent / "secrets" / "devices"

        cert_files = {
            "ca": str(secrets_dir / f"{CERT_NAME}ca.crt"),
            "client_cert": str(secrets_dir / f"{CERT_NAME}client.crt"),
            "client_key": str(secrets_dir / f"{CERT_NAME}client.key"),
        }

        for path in cert_files.values():
            if not Path(path).exists():
                raise FileNotFoundError(f"Missing certificate file: {path}")

        self._ssl_context.load_verify_locations(cert_files["ca"])
        self._ssl_context.load_cert_chain(
            cert_files["client_cert"], cert_files["client_key"]
        )
        self.client.tls_set_context(self._ssl_context)
        print(f"Successfully loaded certificates from {secrets_dir}")

    def _onConnect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Connected to broker!")
        else:
            print(f"Connection failed with code {rc}")

    def _onDisconnect(self, client, userdata, rc):
        if self._intentional_disconnect:
            print("Disconnected intentionally, will not reconnect.")
            return

        print(f"Unexpected disconnection (rc={rc}). Retrying...")
        delay = 1
        while not self._intentional_disconnect:
            try:
                self.client.reconnect()
                print("Reconnected successfully!")
                return
            except Exception as e:
                print(f"Reconnect failed ({e}), retrying in {delay}s...")
                time.sleep(delay)
                delay = min(delay * 2, 30)

    def _onMessage(self, client, userdata, msg):
        print(f"Received message on {msg.topic}: {msg.payload.decode()}")

    def isConnected(self):
        return self.client.is_connected()

    def connect(self):
        self._intentional_disconnect = False
        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()

    def disconnect(self):
        self._intentional_disconnect = True
        self.client.loop_stop()
        self.client.disconnect()
        print("Disconnected from broker")

    def publish(self, topic, payload, qos=0, retain=False):
        rc, _ = self.client.publish(
            topic=topic, payload=payload, qos=qos, retain=retain
        )
        if rc == 0:
            print(f"Sent `{payload}` to topic `{topic}`")
        else:
            print(f"Failed to send message to topic {topic}")

    def subscribe(self, topic, qos=0):
        self.client.subscribe(topic=topic, qos=qos)
