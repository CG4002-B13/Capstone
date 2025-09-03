import torch
import torch.nn as nn
import torch.optim as optim

INPUT_SIZE = 5
HIDDEN_SIZE = 3
OUTPUT_SIZE = 3

class MLP(nn.Module):
    def __init__(self, input_size, hidden_size, output_size):
        super(MLP, self).__init__()
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(hidden_size, output_size)
    
    def forward(self, x):
        fc1_out = self.fc1(x)
        relu1_out = self.relu(fc1_out)
        fc2_out = self.fc2(relu1_out)
        return fc2_out

# Helper function to write layer weights/bias
def write_layer(f, weight, bias, layer_name):
    w = weight.detach().numpy()
    b = bias.detach().numpy()

    # Write weights
    f.write(f"float {layer_name}_weights[{w.shape[0]}][{w.shape[1]}] = {{\n")
    for row in w:
        f.write("    {" + ", ".join([f"{val:.6f}" for val in row]) + "},\n")
    f.write("};\n\n")

    # Write bias
    f.write(f"float {layer_name}_bias[{b.shape[0]}] = {{")
    f.write(", ".join([f"{val:.6f}" for val in b]))
    f.write("};\n\n")

if __name__ == "__main__":
    # Sample dataset
    X = torch.randn(1000, INPUT_SIZE)  # 1000 samples, 128 features
    y = torch.randint(0, OUTPUT_SIZE, (1000,))  # 10 classes

    # DataLoader
    dataset = torch.utils.data.TensorDataset(X, y)
    loader = torch.utils.data.DataLoader(dataset, batch_size=32, shuffle=True)

    # Initialize model, loss, optimizer
    model = MLP(input_size=INPUT_SIZE, hidden_size=HIDDEN_SIZE, output_size=OUTPUT_SIZE)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)

    # Training loop
    epochs = 30
    for epoch in range(epochs):
        total_loss = 0
        for batch_X, batch_y in loader:
            optimizer.zero_grad()
            outputs = model(batch_X)
            loss = criterion(outputs, batch_y)
            loss.backward()
            optimizer.step()
            total_loss += loss.item()
        print(f"Epoch [{epoch+1}/{epochs}] Loss: {total_loss/len(loader):.4f}")

    # Save model for testing
    torch.save(model.state_dict(), "mlp.pth")

    with open("../Hardware/mlp_weights.h", "w") as f:
        f.write("#ifndef MLP_WEIGHTS_H\n#define MLP_WEIGHTS_H\n\n")

        f.write(f"#define INPUT_SIZE {INPUT_SIZE}\n")
        f.write(f"#define HIDDEN_SIZE {HIDDEN_SIZE}\n")
        f.write(f"#define OUTPUT_SIZE {OUTPUT_SIZE}\n\n")

        write_layer(f, model.fc1.weight, model.fc1.bias, "layer1")
        write_layer(f, model.fc2.weight, model.fc2.bias, "layer2")

        f.write("#endif\n")
