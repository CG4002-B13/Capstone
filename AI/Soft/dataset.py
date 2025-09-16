import torch
import torchaudio
from extract_mel_spectogram import waveform_to_melspec

class AudioDataset(torch.utils.data.Dataset):
    def __init__(self, subset):
        self.dataset = torchaudio.datasets.SPEECHCOMMANDS(
                root="./data", download=True, subset=subset
        )
        
        # Map labels to integers
        self.labels = sorted(list(set(dat[2] for dat in self.dataset)))
        self.label_to_idx = {label: idx for idx, label in enumerate(self.labels)}

    def __len__(self):
        return len(self.dataset)

    def __getitem__(self, idx):
        waveform, _, label, *_ = self.dataset[idx]

        # Convert waveform tensor to numpy
        y = waveform.squeeze().numpy()

        # Use your function to compute log-mel spectrogram
        logmel = waveform_to_melspec(y)

        # Convert to torch tensor and add channel dimension
        x = torch.tensor(logmel).unsqueeze(0)  # (1, n_mels, frames)
        y_idx = torch.tensor(self.label_to_idx[label], dtype=torch.long)
        return x, y_idx
