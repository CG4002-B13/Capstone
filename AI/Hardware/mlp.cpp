#include <iostream>
#include <cmath>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include "mlp_weights.h"

typedef ap_axiu<32,0,0,0> axis_word;

// ReLU activation
float relu(float x) {
    return x > 0 ? x : 0;
}

// MLP forward pass
void mlp_forward(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream) {
    #pragma HLS INTERFACE axis port=in_stream
    #pragma HLS INTERFACE axis port=out_stream
    #pragma HLS INTERFACE ap_ctrl_none port=return

    float input[INPUT_SIZE];
    float hidden[HIDDEN_SIZE];
    float output[OUTPUT_SIZE];

    // Read INPUT_SIZE values from AXI stream
    for (int j = 0; j < INPUT_SIZE; j++) {
        axis_word pkt = in_stream.read();
        input[j] = *(float*)&pkt.data; // read float from TDATA
    }

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
    
    // Write OUTPUT_SIZE values to AXI stream
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        axis_word pkt;
        pkt.data = *(ap_uint<32>*)&output[i]; // convert float to 32-bit TDATA
        pkt.last = (i == OUTPUT_SIZE-1) ? 1 : 0; // mark last word
        out_stream.write(pkt);
    }
}
