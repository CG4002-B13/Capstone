#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include "mel_inputs.h"
#include <iostream>
using namespace std;

#define N_CLASSES 11
#define IN_H  64
#define IN_W  81
#define IN_C   1

typedef union {
    float f;
    unsigned int i;
} fp_conv;

typedef ap_axiu<32, 0, 0, 0> axis_word;

void cnn_accel(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream);

int main() {
    hls::stream<axis_word> in_stream;
    hls::stream<axis_word> out_stream;

    fp_conv converter;
    axis_word pkt;

    float sample_input[IN_H][IN_W][IN_C];
    for (int h = 0; h < IN_H; h++) {
        for (int w = 0; w < IN_W; w++) {
            sample_input[h][w][0] = mel_input_0[h][w];
        }
    }
    
    for (int h = 0; h < IN_H; h++) {
        for (int w = 0; w < IN_W; w++) {
            converter.f = sample_input[h][w][0];
            pkt.data = converter.i;
            in_stream.write(pkt);
        }
    }

    cnn_accel(in_stream, out_stream);

    for (int i = 0; i < N_CLASSES; i++) {
        out_stream.read(pkt);
        converter.i = pkt.data;
        float out_val = converter.f;
        cout << out_val << endl;
    }
}