import ssl
from config import CERT_NAME
from pathlib import Path
import paho.mqtt.client as mqtt


class SecureMQTTClient:
    def __init__(self, host="127.0.0.1", port=1883, clientId=None):
        self.host = host
        self.port = port
        self.client = mqtt.Client(client_id=clientId)
        self._ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)

        self.client.on_connect = self._onConnect
        self.client.on_message = self._onMessage

    def _loadCertificate(self):
        secrets_dir = Path(__file__).resolve().parent.parent / "secrets"

        cert_files = {
            "ca": str(secrets_dir / f"{CERT_NAME}ca.crt"),
            "client_cert": str(secrets_dir / f"{CERT_NAME}client.crt"),
            "client_key": str(secrets_dir / f"{CERT_NAME}client.key"),
        }

        missing_files = [
            path for path in cert_files.values() if not Path(path).exists()
        ]
        if missing_files:
            raise FileNotFoundError(
                f"Missing certificate files: {', '.join(missing_files)}"
            )

        try:
            self._ssl_context.load_verify_locations(cert_files["ca"])
            self._ssl_context.load_cert_chain(
                cert_files["client_cert"], cert_files["client_key"]
            )
            self.client.tls_set_context(self._ssl_context)
            print(f"Successfully loaded certificates from {secrets_dir}")

        except Exception as e:
            raise RuntimeError(f"Failed to load SSL certificates: {e}")

    def _onConnect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Connected to broker!")
        else:
            print(f"Connection failed with code {rc}")

    def _onMessage(self, client, userdata, msg):
        print(f"Recevied message on {msg.topic}: {msg.payload.decode()}")

    def connect(self):
        self._loadCertificate()
        self.client.connect(host=self.host, port=self.port, keepalive=60)
        self.client.loop_start()

    def disconnect(self):
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
