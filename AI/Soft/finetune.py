import torch
import torch.nn as nn
from torch.utils.data import DataLoader, random_split
from CNN import CNN
from finetune_dataset import FinetuneDataset

PRETRAIN_CLASSES = 35
FINE_TUNED_CLASSES = 6
EPOCHS = 120

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(device)

def train_model():
    for epoch in range(EPOCHS):
        model.train()
        running_loss = 0.0

        for batch_idx, (x_batch, y_batch) in enumerate(train_loader):
            x_batch, y_batch = x_batch.to(device), y_batch.to(device)

            logits = model(x_batch)
            loss = loss_fn(logits, y_batch)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            # accumulate loss
            running_loss += loss.item()

            # print every 50 batches
            if (batch_idx + 1) % 50 == 0:
                print(f"Epoch [{epoch+1}/{EPOCHS}], Step [{batch_idx+1}/{len(train_loader)}], Loss: {loss.item():.4f}")

        # print epoch average
        avg_loss = running_loss / len(train_loader)
        print(f"Epoch [{epoch+1}/{EPOCHS}] finished → Average Loss: {avg_loss:.4f}")

def test_model():
    model.eval()  # set to evaluation mode

    correct = 0
    total = 0
    test_loss = 0.0

    with torch.no_grad():  # disable gradient computation
        for x_batch, y_batch in test_loader:
            x_batch, y_batch = x_batch.to(device), y_batch.to(device)

            logits = model(x_batch)
            loss = loss_fn(logits, y_batch)
            test_loss += loss.item() * x_batch.size(0)  # sum up batch loss

            # get predictions
            _, predicted = torch.max(logits, dim=1)
            correct += (predicted == y_batch).sum().item()
            total += y_batch.size(0)

    # average loss and accuracy
    avg_loss = test_loss / total
    accuracy = correct / total * 100

    print(f"Test Loss: {avg_loss:.4f}")
    print(f"Test Accuracy: {accuracy:.2f}%")

model = CNN(PRETRAIN_CLASSES)
model.load_state_dict(torch.load("model.pth", map_location=device))
model.classifier = nn.Sequential(nn.Linear(64, FINE_TUNED_CLASSES))
for layer in model.classifier:
    nn.init.xavier_uniform_(layer.weight)
    nn.init.zeros_(layer.bias)

for param in model.parameters():
    param.requires_grad = False  # freeze all layers

for param in model.classifier.parameters():  # except the new head
    param.requires_grad = True

model.to(device)

train_data = FinetuneDataset("data/furniture/labels.csv", "data/furniture/audio/training")
test_data = FinetuneDataset("data/furniture/labels.csv", "data/furniture/audio/testing")

train_loader = DataLoader(train_data, batch_size=len(train_data), shuffle=True, pin_memory=True)
test_loader = DataLoader(test_data, batch_size=len(test_data), shuffle=False, pin_memory=True)
print(len(test_data))

optimizer = torch.optim.Adam(model.parameters(), lr=1e-3)
loss_fn = torch.nn.CrossEntropyLoss()

train_model()
test_model()

torch.save(model.state_dict(), 'finetune.pth')
