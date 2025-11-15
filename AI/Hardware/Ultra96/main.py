import torchaudio
import logging
import time
import json
import numpy as np
from config import MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS
from mqtt_client import SecureMQTTClient
from pynq import Overlay, PL
from cnn_inference import classify_audio

MIC_SR = 8000
N_MELS = 64

command_dict = {0: "SELECT", 1: "DELETE"}

def setup_logger():
    logger = logging.getLogger("ai_logger")
    logger.setLevel(logging.INFO)
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s",
                                datefmt="%Y-%m-%d %H:%M:%S")
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    return logger

def setup_ai_tools():
    mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=MIC_SR, n_mels=N_MELS)
    db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

    return mel_transformer, db_transformer

def main():
    logger = setup_logger()
    mel_transformer, db_transformer = setup_ai_tools()
    isDebugMode = False
    start_time = 0
    timeout = False
    pkt_counter = 0
    pkt_list = []

    PL.reset()
    ol = Overlay('/home/xilinx/design_1.bit')
    dma = ol.axi_dma_0
    print(dma)
    logger.info("DMA Block 0 checked.")
    print(ol)
    logger.info("CNN Block activated.")

    # Debug Callback to activate debug mode
    def debug_callback():
        nonlocal isDebugMode
        isDebugMode = True

    # AI Callback function to gather packets
    def ai_callback(data):
        nonlocal pkt_counter, start_time
        print(pkt_counter)
        if pkt_counter == 0:
            start_time = time.time()

        in_pkt = np.frombuffer(data, dtype=np.int16)
        pkt_list.append(in_pkt)
        pkt_counter += 1

    # Setup MQTT client
    mqtt_client = SecureMQTTClient(
        username=MQTT_USER,
        password=MQTT_PASS,
        host=MQTT_HOST,
        port=MQTT_PORT,
        clientId="ultra96",
        ai_callback=ai_callback,
        debug_callback=debug_callback
    )
    mqtt_client.connect()
    mqtt_client.subscribe(topic="esp32/voice_data")
    mqtt_client.subscribe(topic="esp32/command")
    
    try:
        while True:
            # Check for 2s timeout once first packet received
            if (len(pkt_list) >= 1 and time.time() - start_time > 2) or len(pkt_list) == 3:
                timeout = True
            
            if timeout == True:
                if len(pkt_list) == 3:
                    print(len(pkt_list[0]), len(pkt_list[1]), len(pkt_list[2]))
                    print(pkt_list[0])
                    waveform = np.append(pkt_list[0][1:], pkt_list[2][1:])
                    command = command_dict[pkt_list[0][0]]
                    pred = classify_audio(waveform, logger, dma, mel_transformer, db_transformer)
                    if isDebugMode:
                        end_time = time.time()
                        out_pkt = {"status": "DEBUG", "info": {"receiveTime": int(start_time * 1000), "inferenceTime": int(1000 * (end_time - start_time)), "sendTime": int(end_time * 1000)}}
                        isDebugMode = False
                    else:
                        out_pkt = {"status": "SUCCESS", "info": {"command": command, "result": pred}}
                elif len(pkt_list) == 2:
                    waveform = pkt_list[1][1:]
                    command = command_dict[pkt_list[0][0]]
                    pred = classify_audio(waveform, logger, dma, mel_transformer, db_transformer)
                    if isDebugMode:
                        end_time = time.time()
                        out_pkt = {"status": "DEBUG", "info": {"receiveTime": int(start_time * 1000), "inferenceTime": int(1000 * (end_time - start_time)), "sendTime": int(end_time * 1000)}}
                        isDebugMode = False
                    else:
                        out_pkt = {"status": "SUCCESS", "info": {"command": command, "result": pred}}
                else:
                    out_pkt = {"status": "FAILED", "info": "Did not receive all packets!"}
                
                pkt_counter = 0
                timeout = False
                pkt_list.clear()
                json_pkt = json.dumps(out_pkt)
                mqtt_client.publish("ultra96/voice_result", json_pkt)            
    except KeyboardInterrupt:
        mqtt_client.disconnect()

if __name__ == "__main__":
    main()
