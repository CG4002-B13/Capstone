from CNN import CNN
import torch
import os
import csv

# ---- Config ----
data_folder = "./data/mel_specs/training"
model_path = "./model.pth"
csv_input_path = "./mel_inputs.csv"
csv_logits_path = "./logits.csv"
num_files = 10

# ---- Load model ----
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = CNN(35)
model.load_state_dict(torch.load("model.pth", map_location=device))
model.to(device)
model.eval()

# ---- Collect .pt files ----
all_files = [f for f in os.listdir(data_folder) if f.endswith(".pt")]
files_to_use = all_files[:num_files]

inputs_list = []
labels = []
for f in files_to_use:
    sample = torch.load(os.path.join(data_folder, f))  # assume shape [C, H, W] or [1, H, W]
    data = sample["mel"]
    print(data.shape)
    inputs_list.append(data)
    labels.append(sample["label"])

# Stack into a batch
inputs = torch.stack(inputs_list, dim=0).float()  # shape: [N, C, H, W]

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
    print(test.shape)

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