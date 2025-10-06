import os
import torch
import torchaudio
import pandas as pd
from torch.nn.functional import pad

TARGET_SR = 16000
N_MELS = 64

class FinetuneDataset(torch.utils.data.Dataset):
    def __init__(self, csv_file, audio_dir):
        self.waveforms = []
        self.labels = []
        self.df = pd.read_csv(csv_file)
        self.df.set_index("filename", inplace=True)
        self.audio_dir = audio_dir

        # Initialise resampler / mel transformer
        self.resampler = torchaudio.transforms.Resample(orig_freq=48000, new_freq=TARGET_SR)
        self.mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=TARGET_SR, n_mels=N_MELS)
        self.db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

        filepaths = os.listdir(audio_dir)
        print(filepaths)
        for audio_path in filepaths:
            
            waveform, sr = torchaudio.load(audio_dir + "/" + audio_path)
            mono_waveform = waveform.mean(dim=0, keepdim=True)
            if sr != TARGET_SR:
                mono_waveform = self.resampler(mono_waveform)

            mono_waveform = self.pad_or_truncate_wave(mono_waveform)
            mel_spec = self.mel_transformer(mono_waveform)  # shape: [n_mels, time_frames]
            mel_spec = self.db_transformer(mel_spec)

            label = self.df.at[audio_path, "label"]

            self.waveforms.append(mel_spec)
            self.labels.append(label)

    def __len__(self):
        return len(self.waveforms)

    def __getitem__(self, idx):
        return self.waveforms[idx], self.labels[idx]
    
    def pad_or_truncate_wave(self, waveform):
        if waveform.shape[1] < TARGET_SR:
            total_pad = TARGET_SR - waveform.shape[1]
            pad_left = total_pad // 2
            pad_right = total_pad - pad_left
            new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)
        else:
            new_wav = waveform[:, :TARGET_SR]

        return new_wav