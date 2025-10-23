import time
import soundfile as sf
import numpy as np
import json
from config import MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
from mqtt_client import SecureMQTTClient

def ai_callback(data):
    waveform = data.split(",")
    waveform = waveform[:-1]
    waveform = np.array(waveform, dtype=np.int16)
    sf.write("output.wav", waveform, 2000, subtype='PCM_16')
    print(waveform)

def main():
    mqtt_client = SecureMQTTClient(
        username=MQTT_USER,
        password=MQTT_PASS,
        host=MQTT_HOST,
        port=MQTT_PORT,
        clientId="ultra96",
        on_message_callback=ai_callback
    )
    mqtt_client.connect()
    mqtt_client.subscribe(topic="esp32/voice_data")

    data = {"status": "SUCCESS", "info": {"command": "SELECT", "object": "TABLE"}}
    json_data = json.dumps(data)

    mqtt_client.publish("ultra96/voice_result", json_data)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        mqtt_client.disconnect()


if __name__ == "__main__":
    main()
