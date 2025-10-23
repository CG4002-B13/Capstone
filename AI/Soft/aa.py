import torchaudio
import soundfile as sf
import numpy as np
from torch.nn.functional import pad

TARGET_SR = 16000
N_MELS = 64

def pad_or_truncate_wave(waveform):
    if waveform.shape[1] < TARGET_SR:
        total_pad = TARGET_SR - waveform.shape[1]
        pad_left = total_pad // 2
        pad_right = total_pad - pad_left
        new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)
    else:
        new_wav = waveform[:, :TARGET_SR]

    return new_wav

def extract_wave(waveform, sr):
    print(sr)
    len = waveform.shape[1]
    starts = [0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
    for sec_start in starts:
        sample_start = int(sec_start * sr)
        sample_end = sample_start + sr
        new_wave = waveform[:, sample_start:sample_end]
        torchaudio.save(f"temp_wavs/out_{sec_start}.wav", new_wave, sample_rate=sr, bits_per_sample=16)

resampler = torchaudio.transforms.Resample(orig_freq=8000, new_freq=TARGET_SR)
mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=TARGET_SR, n_mels=N_MELS)
db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

waveform, sr = torchaudio.load("detection_1.wav")
print(waveform)
extract_wave(waveform, sr)

mono_waveform = waveform.mean(dim=0, keepdim=True)
print(mono_waveform.shape)
if sr != TARGET_SR:
    mono_waveform = resampler(mono_waveform)
print(mono_waveform.shape)
mono_waveform = pad_or_truncate_wave(mono_waveform)
print(mono_waveform.shape)
mel_spec = mel_transformer(mono_waveform)  # shape: [n_mels, time_frames]
print(mel_spec.shape)
mel_spec = db_transformer(mel_spec)

# wave, sr = sf.read("Recording.wav")
# print(wave, wave.shape)
# print(sr)
# sf.write("test.wav", wave, 48000, subtype="FLOAT")