#include <iostream>
#include <cmath>
using namespace std;

#define INPUT_SIZE 128
#define HIDDEN_SIZE 64
#define OUTPUT_SIZE 10

void mlp_forward(float input[], float output[]);

int main() {
    // Sample input (sample in)
    float input[INPUT_SIZE] = {0};
    input[0] = 1.0;
    input[1] = -0.5;
    input[2] = 0.3;
    float output[OUTPUT_SIZE];

    // Run the hardware function
    mlp_forward(input, output);

    // Print results
    cout << "MLP Output:" << endl;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        cout << output[i] << endl;
    }

    return 0;
}
