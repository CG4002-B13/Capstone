import torch
import torchaudio
import os
from torch.nn.functional import pad

DIR = "data/up_down/raw_1s"
MIC_SR = 8000
TARGET_LEN = 16000
DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

resampler = torchaudio.transforms.Resample(orig_freq=16000, new_freq=8000)

def pad_wave(waveform):
    total_pad = TARGET_LEN - waveform.shape[1]
    pad_left = total_pad // 2
    pad_right = total_pad - pad_left
    new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)

    return new_wav

sub_dirs = os.listdir(DIR)
for dir in sub_dirs:
    subpath = os.path.join(DIR, dir)
    for i, audio_file in enumerate(os.listdir(subpath)):
        filepath = os.path.join(subpath, audio_file)
        waveform, sr = torchaudio.load(filepath)
        print(waveform.shape)
        waveform = resampler(waveform)
        print(waveform.shape)
        waveform = pad_wave(waveform)
        print(waveform.shape)
        torchaudio.save(f"data/up_down/8k_2s/{dir}/{dir}_{i}.wav", waveform, MIC_SR, encoding="PCM_S", bits_per_sample=16)
