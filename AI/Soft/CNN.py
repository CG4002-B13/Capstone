import torch.nn as nn
import torch

class CNN(nn.Module):
    def __init__(self, n_classes):
        super().__init__()

        self.network = nn.Sequential(
                nn.Conv2d(1, 16, kernel_size=(3,3), padding=1, stride=2),
                nn.ReLU(),
                nn.Conv2d(16, 16, kernel_size=(3,3), padding=1, stride=1),
                nn.ReLU(),
                nn.Dropout(0.2),

                nn.Conv2d(16, 32, kernel_size=(3,3), padding=1, stride=2),
                nn.ReLU(),
                nn.Conv2d(32, 32, kernel_size=(3,3), padding=1, stride=1),
                nn.ReLU(),
                nn.Dropout(0.2),

                nn.Conv2d(32, 64, kernel_size=(3,3), padding=1, stride=2),
                nn.ReLU(),
                nn.Conv2d(64, 64, kernel_size=(3,3), padding=1, stride=1),
                nn.ReLU(),
                nn.Dropout(0.2),
                nn.AdaptiveAvgPool2d((1,1))
        )

        self.classifier = nn.Sequential(
                nn.Linear(64, n_classes)
        )
    
    def forward(self, x):
        x = self.network(x)
        x = torch.flatten(x, 1)
        out = self.classifier(x)

        return out