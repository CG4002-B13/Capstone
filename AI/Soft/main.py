import time
import soundfile as sf
import numpy as np
import json
from config import MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
from mqtt_client import SecureMQTTClient

counter = 0

def ai_callback(data):
    global counter
    waveform = np.frombuffer(data, dtype=np.int16)
    print(waveform)
    print(waveform.shape)
    command = waveform[0]
    waveform = waveform[1:]
    print(command)

    sf.write(f"output{counter}.wav", waveform, 8000, subtype='PCM_16')

    counter += 1
    if counter == 3:
        counter = 0

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
