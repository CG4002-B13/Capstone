import torch
from mlp import MLP, INPUT_SIZE, HIDDEN_SIZE, OUTPUT_SIZE

# Create the same model as used in training
model = MLP(input_size=INPUT_SIZE, hidden_size=HIDDEN_SIZE, output_size=OUTPUT_SIZE)

# Load model
model.load_state_dict(torch.load("mlp.pth", weights_only=True))
model.eval()

# Prepare input (5 values, but only first 3 are non-zero)
input_tensor = torch.zeros(INPUT_SIZE)
input_tensor[0] = 1.0
input_tensor[1] = -0.5
input_tensor[2] = 0.3

# Run forward pass
with torch.no_grad():
    output = model(input_tensor)

print("Python model output:", output.numpy())
