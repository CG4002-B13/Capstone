import torch
import torchaudio
import pandas as pd
import os
import csv
from CNN import CNN
from torch.nn.functional import pad

def pad_or_truncate_wave(waveform):
    if waveform.shape[1] < TARGET_SR:
        total_pad = TARGET_SR - waveform.shape[1]
        pad_left = total_pad // 2
        pad_right = total_pad - pad_left
        new_wav = pad(waveform, (pad_left, pad_right), mode='constant', value=0)
    else:
        new_wav = waveform[:, :TARGET_SR]

    return new_wav

# ---- Config ----
data_folder = "./data/furniture_recorded/training/person_0"
labels_path = "./data/furniture_recorded/labels.csv"
model_path = "./finetune.pth"
csv_input_path = "./mel_inputs.csv"
csv_logits_path = "./logits.csv"
num_files = 8

NUM_CLASSES = 11
MIC_SR = 8000
N_MELS = 64
TARGET_SR = 16000

df = pd.read_csv(labels_path)
df.set_index("filename", inplace=True)
resampler = torchaudio.transforms.Resample(orig_freq=48000, new_freq=TARGET_SR)
mel_transformer = torchaudio.transforms.MelSpectrogram(sample_rate=MIC_SR, n_mels=N_MELS)
db_transformer = torchaudio.transforms.AmplitudeToDB(stype="power")

# ---- Load model ----
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = CNN(NUM_CLASSES)
model.load_state_dict(torch.load(model_path, map_location=device))
model.to(device)
model.eval()

audio_files = os.listdir(data_folder)
files_to_use = audio_files[:num_files]

inputs_list = []
labels = []
for f in files_to_use:
    waveform, sr = torchaudio.load(data_folder + "/" + f)
    # mono_waveform = waveform.mean(dim=0, keepdim=True)
    # if sr != TARGET_SR:
    #     mono_waveform = resampler(mono_waveform)

    # mono_waveform = pad_or_truncate_wave(mono_waveform)
    mel_spec = mel_transformer(waveform)  # shape: [n_mels, time_frames]
    mel_spec = db_transformer(mel_spec)
    label = df.at[f, "label"]

    inputs_list.append(mel_spec)
    labels.append(label)

# Stack into a batch
inputs = torch.stack(inputs_list, dim=0).float()

# ---- Save inputs to CSV ----
with open("mel_inputs.h", 'w') as f:
    f.write("// Auto-generated mel spectrogram inputs in HxW format\n\n")
    f.write("#pragma once\n\n")

    for idx, mel_spec in enumerate(inputs_list):
        mel_spec = mel_spec.squeeze(0)
        H, W = mel_spec.shape
        array_name = f"mel_input_{idx}"

        f.write(f"const float {array_name}[{H}][{W}] = {{\n")
        for h in range(H):
            f.write("    {")
            f.write(", ".join(f"{mel_spec[h, w]:.6f}" for w in range(W)))
            f.write("}")
            if h != H - 1:
                f.write(",\n")
            else:
                f.write("\n")
        f.write("};\n\n")

# ---- Run model ----
inputs = inputs.to(device)
with torch.no_grad():
    logits = model(inputs)  # shape: [N, num_classes]

for i in range(len(model.network)):
    if i == 0:
        test = model.network[i](inputs)
    else:
        test = model.network[i](test)

# ---- Save logits to CSV ----
with open(csv_logits_path, 'w', newline='') as f:
    writer = csv.writer(f)
    for i in range(logits.shape[0]):
        writer.writerow(logits[i].cpu().tolist())

labels_tensor = torch.tensor(labels, dtype=torch.int, device=device)
correct = 0
_, predicted = torch.max(logits, dim=1)
correct += (predicted == labels_tensor).sum().item()
print(correct, labels)