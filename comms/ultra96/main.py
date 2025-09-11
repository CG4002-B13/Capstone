import time
from config import MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
from mqtt_client import SecureMQTTClient


def main():
    mqtt_client = SecureMQTTClient(
        username=MQTT_USER,
        password=MQTT_PASS,
        host=MQTT_HOST,
        port=MQTT_PORT,
        clientId="ultra96",
    )
    mqtt_client.connect()
    mqtt_client.subscribe(topic="esp32/testing")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        mqtt_client.disconnect()


if __name__ == "__main__":
    main()
