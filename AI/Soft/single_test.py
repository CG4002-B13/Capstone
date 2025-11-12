import torch
import torchaudio
from torch.nn.functional import pad
from CNN import CNN

MODEL_PATH = "finetune.pth"
AUDIO_PATH = "output1.wav"
MIC_SR = 8000
TARGET_LEN = 16000
NUM_CLASSES = 11
DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

vocab = {0: "chair", 1: "table", 2: "lamp", 3: "tv", 4: "bed", 5: "plant", 6: "sofa",
          7: "ODM", 8: "detection", 9: "up", 10: "down"}

def pad_or_truncate_wave(waveform):
    if waveform.shape[1] < TARGET_LEN:
        total_pad = TARGET_LEN - waveform.shape[1]
        pad_left = total_pad // 2
        pad_right = total_pad - pad_left
        new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)
    else:
        new_wav = waveform[:, :TARGET_LEN]

    return new_wav

model = CNN(NUM_CLASSES)
model.load_state_dict(torch.load(MODEL_PATH, map_location=DEVICE))
model.to(DEVICE)
model.eval()

waveform, sr = torchaudio.load(AUDIO_PATH)
print(waveform.shape)
waveform = pad_or_truncate_wave(waveform)
print(waveform.shape)
torchaudio.save("resampled.wav", waveform, MIC_SR, encoding="PCM_S", bits_per_sample=16)

mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=MIC_SR, n_mels=64)
db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

mel_spec = mel_transformer(waveform)
print(mel_spec.shape)
mel_spec = db_transformer(mel_spec)
x = mel_spec.unsqueeze(0).to(DEVICE)  # add batch dimension
print(x.shape)
with torch.no_grad():
    logits = model(x)
    print(logits)
    pred_class = torch.argmax(logits, dim=1)

print(f"Predicted class: {vocab[pred_class.item()]}")
