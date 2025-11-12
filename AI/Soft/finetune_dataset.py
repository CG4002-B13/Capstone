import os
import torch
import torchaudio
import pandas as pd
from torch.nn.functional import pad
from collections import Counter

MIC_SR = 8000
TARGET_SR = 16000
N_MELS = 64

class FinetuneDataset(torch.utils.data.Dataset):
    def __init__(self, csv_file, audio_dir, ignored_dir):
        self.waveforms = []
        self.labels = []
        self.df = pd.read_csv(csv_file)
        self.df.set_index("filename", inplace=True)
        self.audio_dir = audio_dir
        
        # Get ignored filenames
        with open(ignored_dir, "r") as f:
            ignored = f.readlines()
            ignored = [line.strip() for line in ignored]

        # Initialise resampler / mel transformer
        # self.resampler = torchaudio.transforms.Resample(orig_freq=8000, new_freq=TARGET_SR)
        self.mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=MIC_SR, n_mels=N_MELS)
        self.db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

        subfolders = os.listdir(audio_dir)
        for subfolder in subfolders:
            subfolder_path = os.path.join(audio_dir, subfolder)
            filepaths = os.listdir(subfolder_path)
            for audio_file in filepaths:
                furniture_name = audio_file.split("_")[2]
                if audio_file in ignored or furniture_name in ["increase", "decrease", "toggle"]:
                    continue

                audio_path = os.path.join(subfolder_path, audio_file)
                waveform, sr = torchaudio.load(audio_path)

                mel_spec = self.mel_transformer(waveform)  # shape: [n_mels, time_frames]
                mel_spec = self.db_transformer(mel_spec)
                
                label = self.df.at[audio_file, "label"]
                
                self.waveforms.append(mel_spec)
                self.labels.append(label)
        print(self.labels)
        counts = Counter(self.labels)
        print(sorted(counts.items()))

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