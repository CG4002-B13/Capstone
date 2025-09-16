import torch
import torchaudio
from tqdm import tqdm
from torch.nn.functional import pad
from extract_mel_spectogram import waveform_to_melspec

TARGET_SR = 16000
N_MELS = 64

class AudioDataset(torch.utils.data.Dataset):
    def __init__(self, subset):
        self.dataset = torchaudio.datasets.SPEECHCOMMANDS(
                root="./data", download=True, subset=subset
        )
        
        # Map labels to integers
        self.labels = sorted(list(set(dat[2] for dat in self.dataset)))
        self.label_to_idx = {label: idx for idx, label in enumerate(self.labels)}
        
        # Initialise resampler / mel transformer
        self.resampler = torchaudio.transforms.Resample(new_freq=TARGET_SR)
        self.mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=TARGET_SR, n_mels=N_MELS)

        # Preprocess waveforms
        self.preprocessed_waveforms = []
        self.preprocessed_labels = []
        for waveform, sr, label, *_ in tqdm(self.dataset, desc="Preprocessing audio", total=len(self.dataset)):
            if sr != TARGET_SR:
                waveform = self.resampler(waveform)
            
            waveform = self.pad_or_truncate_wave(waveform)
            mel_spec = self.mel_transformer(waveform)  # shape: [n_mels, time_frames]
            mel_spec = torch.log1p(mel_spec)

            self.preprocessed_waveforms.append(mel_spec)
            self.preprocessed_labels.append(self.label_to_idx[label])

    def __len__(self):
        return len(self.dataset)

    def __getitem__(self, idx):
        return self.preprocessed_waveforms[idx], self.preprocessed_labels[idx]
    
    def pad_or_truncate_wave(self, waveform):
        if waveform.shape[1] < TARGET_SR:
            total_pad = TARGET_SR - waveform.shape[1]
            pad_left = total_pad // 2
            pad_right = total_pad - pad_left
            new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)
        else:
            new_wav = waveform[:TARGET_SR]

        return new_wav
