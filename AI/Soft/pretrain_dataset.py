import os
import torch
import torchaudio
from tqdm import tqdm
from torch.nn.functional import pad

TARGET_SR = 16000
N_MELS = 64

class SpeechCommandsDataset(torch.utils.data.Dataset):
    def __init__(self, subset, load_preprocessed):
        self.preprocessed_waveforms = []
        self.preprocessed_labels = []
        
        if load_preprocessed == False:
            self.dataset = torchaudio.datasets.SPEECHCOMMANDS(root="./data", download=True, subset=subset)

            # Map labels to integers
            self.labels = sorted(list(set(dat[2] for dat in self.dataset)))
            self.label_to_idx = {label: idx for idx, label in enumerate(self.labels)}
            
            self.mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=TARGET_SR, n_mels=N_MELS)
            self.db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

            # Preprocess waveforms
            for waveform, sr, label, *_ in tqdm(self.dataset, desc="Preprocessing audio", total=len(self.dataset)):
                waveform = self.pad_or_truncate_wave(waveform)
                mel_spec = self.mel_transformer(waveform)  # shape: [n_mels, time_frames]
                mel_spec = self.db_transformer(mel_spec)
                
                self.preprocessed_waveforms.append(mel_spec)
                self.preprocessed_labels.append(self.label_to_idx[label])

            # Save spectograms for easier loading
            self.save_mel_specs(subset=subset)

        else:
            self.dataset = [f for f in os.listdir(f"./data/mel_specs/{subset}") if f.endswith(".pt")]
            self.labels = set()
            for entry in self.dataset:
                sample = torch.load(f"./data/mel_specs/{subset}/{entry}")
                mel_spec = sample["mel"]
                label = sample["label"]
                self.labels.add(label)
                self.preprocessed_waveforms.append(mel_spec)
                self.preprocessed_labels.append(label)
            print(len(self.labels))

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
            new_wav = waveform[:, :TARGET_SR]

        return new_wav

    def save_mel_specs(self, subset):
        for i, (mel, label) in enumerate(zip(self.preprocessed_waveforms, self.preprocessed_labels)):
            torch.save({"mel": mel, "label": label}, f"./data/mel_specs/{subset}/{i}.pt")