import time
from config import MQTT_HOST, MQTT_PORT
from mqtt_client import SecureMQTTClient


def main():
    mqtt_client = SecureMQTTClient(host=MQTT_HOST, port=MQTT_PORT, clientId="ultra96")
    mqtt_client.connect()
    mqtt_client.subscribe(topic="esp32/testing")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        mqtt_client.disconnect()


if __name__ == "__main__":
    main()
