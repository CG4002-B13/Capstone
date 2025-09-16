import torch.nn as nn

class CNN(nn.Module):
    def __init__(self, n_classes):
        super().__init__()
        self.features = nn.Sequential(
                nn.Conv2d(1, 32, kernel_size=(3,3), padding=1),
                nn.ReLU(),
                nn.MaxPool2d((2,2)),

                nn.Conv2d(32, 64, kernel_size=(3,3), padding=1),
                nn.ReLU(),
                nn.MaxPool2d((2,2)),

                nn.Conv2d(64, 128, kernel_size=(3,3), padding=1),
                nn.ReLU(),
                nn.AdaptiveAvgPool2d((1,1))
        )
        self.classifier = nn.Linear(128, n_classes)

    def forward(self, x):
        f = self.features(x)
        f = f.view(f.size(0), -1)
        out = self.classifier(f)
        return out