#include <iostream>
#include <cmath>
#include "mlp_weights.h"

// ReLU activation
float relu(float x) {
    return x > 0 ? x : 0;
}

// MLP forward pass
void mlp_forward(float input[INPUT_SIZE], float output[OUTPUT_SIZE]) {
    float hidden[HIDDEN_SIZE];

    // Layer 1
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        hidden[i] = layer1_bias[i];
        for (int j = 0; j < INPUT_SIZE; j++) {
            hidden[i] += input[j] * layer1_weights[i][j];
        }
        hidden[i] = relu(hidden[i]);
    }

    // Layer 2
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        output[i] = layer2_bias[i];
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            output[i] += hidden[j] * layer2_weights[i][j];
        }
    }
}
