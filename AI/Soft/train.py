import torch
from torch.utils.data import DataLoader
from CNN import CNN
from pretrain_dataset import SpeechCommandsDataset

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(device)

def train_model():
    for epoch in range(30):
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
                print(f"Epoch [{epoch+1}/30], Step [{batch_idx+1}/{len(train_loader)}], Loss: {loss.item():.4f}")

        # print epoch average
        avg_loss = running_loss / len(train_loader)
        print(f"Epoch [{epoch+1}/30] finished → Average Loss: {avg_loss:.4f}")

    # Save final model
    torch.save(model.state_dict(), 'model.pth')

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

train_data = SpeechCommandsDataset(subset="training", load_preprocessed=True)
test_data = SpeechCommandsDataset(subset="testing", load_preprocessed=True)

train_loader = DataLoader(train_data, batch_size=64, shuffle=True, pin_memory=True)
test_loader = DataLoader(test_data, batch_size=64, shuffle=False, pin_memory=True)

model = CNN(len(train_data.labels))
print(len(train_data.labels))
# model.load_state_dict(torch.load("model.pth", map_location=device))
model.to(device)

optimizer = torch.optim.Adam(model.parameters(), lr=1e-3)
loss_fn = torch.nn.CrossEntropyLoss()

train_model()
test_model()