#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include "mlp_weights.h"

typedef union {
    float f;
    unsigned int i;
} fp_conv;

typedef ap_axiu<32, 0, 0, 0> axis_word;

float relu(float x) {
    return x > 0 ? x : 0;
}

void mlp_forward(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream) {
#pragma HLS INTERFACE axis port=in_stream
#pragma HLS INTERFACE axis port=out_stream
#pragma HLS INTERFACE ap_ctrl_none port=return

    float input[INPUT_SIZE] = {0};
    float hidden[HIDDEN_SIZE] = {0};
    float output[OUTPUT_SIZE] = {0};

    axis_word pkt;
    fp_conv converter;
    // Read INPUT_SIZE values from AXI stream
    for (int i = 0; i < INPUT_SIZE; i++) {
        in_stream.read(pkt);
        converter.i = pkt.data;
        input[i] = converter.f;
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
        converter.f = output[i];
        pkt.data = converter.i;
        pkt.last = (i == (OUTPUT_SIZE - 1)) ? 1 : 0;
        out_stream.write(pkt);
    }
}
