import torch
import numpy as np
from torch.nn.functional import pad
from pynq import allocate

INPUT_SIZE = 64 * 81
OUTPUT_SIZE = 11
TARGET_LEN = 16000

labels_dict = {0: "chair", 1: "table", 2: "lamp", 3: "TV", 4: "bed", 5: "plant", 6: "sofa",
               7: "ODM", 8: "ODM", 9: "up", 10: "down"}

def pad_audio(waveform):
    total_pad = TARGET_LEN - waveform.shape[0]
    pad_left = total_pad // 2
    pad_right = total_pad - pad_left
    new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)

    return new_wav

def preprocess_audio(waveform, mel_transformer, db_transformer):
    waveform = waveform.astype(np.float32) / 32768.0
    waveform = torch.tensor(waveform)
    print(waveform)
    print(waveform.shape)
    if waveform.shape[0] < TARGET_LEN:
        waveform = pad_audio(waveform)
    print(waveform.shape)
    mel_spec = mel_transformer(waveform)
    mel_spec = db_transformer(mel_spec)
    print(mel_spec.shape)
    flattened = mel_spec.flatten().numpy().astype(np.float32).view(np.uint32)

    return flattened

def classify_audio(waveform, logger, dma, mel_transformer, db_transformer):
    flattened = preprocess_audio(waveform, mel_transformer, db_transformer)

    # Allocate buffers
    in_buffer = allocate(shape=(INPUT_SIZE,), dtype=np.uint32)
    logger.info("Input buffer allocated.")
    out_buffer = allocate(shape=(OUTPUT_SIZE,), dtype=np.uint32)

    # Prepare input buffer data
    in_buffer[:] = flattened
    logger.info("Input buffer data prepared.")

    # Start DMA transfer to the FPGA
    dma.sendchannel.transfer(in_buffer)
    logger.info("DMA 0 transfer started.")
    dma.recvchannel.transfer(out_buffer)

    dma.sendchannel.wait()
    dma.recvchannel.wait()
    
    # Convert output to float
    logits = np.array([out_buffer[i].view(np.float32) for i in range(OUTPUT_SIZE)], dtype=np.float32)
    logger.info(f"Output buffer received: {logits}")

    # Get predicted class (argmax)
    logits_tensor = torch.tensor(logits)
    predicted = torch.argmax(logits_tensor).item()
    logger.info(f"Predicted class: {labels_dict[predicted]}")

    # Free buffers
    in_buffer.freebuffer()
    out_buffer.freebuffer()
    logger.info("Input and output buffers freed.")

    return labels_dict[predicted]
