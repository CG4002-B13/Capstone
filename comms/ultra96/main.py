from config import MQTT_HOST, MQTT_PORT
from mqtt_client import SecureMQTTClient


def main():
    mqtt_client = SecureMQTTClient(host=MQTT_HOST, port=MQTT_PORT, clientId="ultra96")
    mqtt_client.connect()


if __name__ == "__main__":
    main()
