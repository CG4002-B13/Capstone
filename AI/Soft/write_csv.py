import os
import csv

labels = {"chair": 0, "table": 1, "lamp": 2, "tv": 3, "bed": 4, "plant": 5, "sofa": 6,
          "ODM": 7, "OBM": 7, "detection": 8, "up": 9, "down": 10}

# Root directory containing the subfolders
root_dir = "data/furniture_recorded"
output_csv = "data/furniture_recorded/labels.csv"

# Collect all audio file paths
audio_files = []

for subfolder in os.listdir(root_dir):
    subfolder_path = os.path.join(root_dir, subfolder)
    if not os.path.isdir(subfolder_path):
        continue
    
    for person in os.listdir(subfolder_path):
        temp_path = os.path.join(subfolder_path, person)
        print(temp_path)
        # Check if it's an audio file (adjust extensions if needed)
        for audio_file in os.listdir(temp_path):
            furniture_name = audio_file.split("_")[2]
            if furniture_name in ["increase", "decrease", "toggle"]:
                continue
            audio_files.append([audio_file, labels[furniture_name]])

# Write to CSV
with open(output_csv, "w", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)
    writer.writerow(["filename", "label"])  # header
    writer.writerows(audio_files)

print(f"Saved {len(audio_files)} entries to {output_csv}")