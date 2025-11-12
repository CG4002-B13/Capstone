import torch
import torch.nn as nn
import numpy as np
from CNN import CNN  # your CNN definition

MODEL_NAME = "finetune.pth"
NUM_CLASSES = 11

def tensor_to_c_array(name, tensor):
    arr = tensor.detach().cpu().numpy()
    dims = "".join(f"[{d}]" for d in arr.shape)
    flat = ", ".join([f"{v}" for v in arr.flatten()])
    return f"static const CNN_DTYPE {name}{dims} = {{ {flat} }};\n"

def export_weights(model_path, header_path, n_classes):
    model = CNN(n_classes=n_classes)
    state_dict = torch.load(model_path, map_location="cpu")
    model.load_state_dict(state_dict)
    model.eval()

    lines = []
    lines.append("#pragma once\n")
    
    conv_count = 1
    bn_count   = 1
    linear_count = 1

    for name, module in model.named_modules():
        # skip the top-level module itself
        if name == "":
            continue

        if isinstance(module, nn.Conv2d):
            lines.append(tensor_to_c_array(f"conv{conv_count}_w", module.weight))
            if module.bias is not None:
                lines.append(tensor_to_c_array(f"conv{conv_count}_b", module.bias))
            conv_count += 1

        elif isinstance(module, nn.BatchNorm2d):
            lines.append(tensor_to_c_array(f"bn{bn_count}_scale", module.weight))  # gamma
            lines.append(tensor_to_c_array(f"bn{bn_count}_shift", module.bias))     # beta
            lines.append(tensor_to_c_array(f"bn{bn_count}_running_mean", module.running_mean))
            lines.append(tensor_to_c_array(f"bn{bn_count}_running_var", module.running_var))
            bn_count += 1

        elif isinstance(module, nn.Linear):
            lines.append(tensor_to_c_array(f"linear{linear_count}_w", module.weight))
            lines.append(tensor_to_c_array(f"linear{linear_count}_b", module.bias))
            linear_count += 1

    with open(header_path, "w") as f:
        f.writelines(lines)

    print(f"Exported weights to {header_path}")

if __name__ == "__main__":
    export_weights(MODEL_NAME, "CNN_weights.h", NUM_CLASSES)
