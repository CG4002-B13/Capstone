#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_fixed.h>
#include <cmath>
using namespace std;

typedef ap_fixed<32, 12> CNN_DTYPE; // 16-bit wide, 6 integer bits, rest fractional
#include "CNN_weights.h"

typedef ap_axiu<32,0,0,0> axis_word;

typedef union {
    float f;
    unsigned int i;
} fp_conv;

#define N_CLASSES 6
#define IN_H  64
#define IN_W  81
#define IN_C   1
#define C1_OUT 16
#define C2_OUT 32
#define C3_OUT 64
#define K 3
#define PADDING 1
#define STRIDE 2

// -----------------------------
// Convolution + BN + ReLU block
// -----------------------------
template<int H_in, int W_in, int H_out, int W_out, int IN_CH, int OUT_CH, int STR>
void conv_bn_relu(
    CNN_DTYPE in_fm[H_in][W_in][IN_CH],
    CNN_DTYPE out_fm[H_out][W_out][OUT_CH],
    const CNN_DTYPE w[OUT_CH][IN_CH][K][K],     // Conv weights
    const CNN_DTYPE b[OUT_CH]                 // Conv bias
) {
    for (int oh = 0; oh < H_out; oh++) {
        for (int ow = 0; ow < W_out; ow++) {
            for (int oc = 0; oc < OUT_CH; oc++) {
                CNN_DTYPE acc = b[oc];
                for (int kh = 0; kh < K; kh++) {
                    for (int kw = 0; kw < K; kw++) {
                        for (int ic = 0; ic < IN_CH; ic++) {
#pragma HLS PIPELINE II=1
                            int h_in = oh * STR + kh - PADDING;
                            int w_in = ow * STR + kw - PADDING;
                            if (h_in >= 0 && h_in < H_in && w_in >= 0 && w_in < W_in) {
                                acc += in_fm[h_in][w_in][ic] * w[oc][ic][kh][kw];
                            }
                        }
                    }
                }
                // ReLU
                out_fm[oh][ow][oc] = (acc > (CNN_DTYPE)0.0) ? acc : (CNN_DTYPE)0.0;
            }
        }
    }
}

// -----------------------------
// Adaptive Average Pooling to 1×1
// -----------------------------
template<int H, int W, int C>
void adaptive_avg_pool(CNN_DTYPE in_fm[H][W][C], CNN_DTYPE out_fm[C]) {
    for (int c = 0; c < C; c++) {
        CNN_DTYPE sum = 0;
        for (int h = 0; h < H; h++) {
            for (int w = 0; w < W; w++) {
                sum += in_fm[h][w][c];
            }
        }
        out_fm[c] = sum / (H * W);
    }
}

// -----------------------------
// Fully Connected layer
// -----------------------------
template<int IN_DIM, int OUT_DIM>
void linear(
    CNN_DTYPE in_vec[IN_DIM],
    CNN_DTYPE out_vec[OUT_DIM],
    const CNN_DTYPE w[OUT_DIM][IN_DIM],
    const CNN_DTYPE b[OUT_DIM],
    bool relu
) {
    for (int o = 0; o < OUT_DIM; o++) {
        CNN_DTYPE acc = b[o];
        for (int i = 0; i < IN_DIM; i++) {
            acc += in_vec[i] * w[o][i];
        }
        CNN_DTYPE val = acc;
        out_vec[o] = relu ? (val > (CNN_DTYPE)0.0 ? val : (CNN_DTYPE)0.0) : val;
    }
}

void cnn_accel(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream) {
#pragma HLS INTERFACE axis port=in_stream
#pragma HLS INTERFACE axis port=out_stream
#pragma HLS INTERFACE ap_ctrl_none port=return

    // Buffers
    CNN_DTYPE fm_in[IN_H][IN_W][IN_C];
    CNN_DTYPE fm1[IN_H/STRIDE][IN_W/STRIDE + 1][C1_OUT];
    CNN_DTYPE fm1b[IN_H/STRIDE][IN_W/STRIDE + 1][C1_OUT];
    CNN_DTYPE fm2[IN_H/(STRIDE*2)][IN_W/(STRIDE*2) + 1][C2_OUT];
    CNN_DTYPE fm2b[IN_H/(STRIDE*2)][IN_W/(STRIDE*2) + 1][C2_OUT];
    CNN_DTYPE fm3[IN_H/(STRIDE*4)][IN_W/(STRIDE*4) + 1][C3_OUT];
    CNN_DTYPE fm3b[IN_H/(STRIDE*4)][IN_W/(STRIDE*4) + 1][C3_OUT];
    CNN_DTYPE pooled[C3_OUT];
    CNN_DTYPE out[N_CLASSES];

    axis_word pkt;
    fp_conv converter;
    for (int h = 0; h < IN_H; h++) {
        for (int w = 0; w < IN_W; w++) {
            pkt = in_stream.read();
            converter.i = pkt.data;
            fm_in[h][w][0] = converter.f;
        }
    }

    // 1st conv block
    conv_bn_relu<IN_H,IN_W,IN_H/STRIDE,IN_W/STRIDE + 1,IN_C,C1_OUT,STRIDE>(fm_in, fm1, conv1_w, conv1_b);
    conv_bn_relu<IN_H/STRIDE,IN_W/STRIDE + 1,IN_H/STRIDE,IN_W/STRIDE + 1,C1_OUT,C1_OUT,1>(fm1, fm1b, conv2_w, conv2_b);

    // 2nd conv block
    conv_bn_relu<IN_H/STRIDE,IN_W/STRIDE + 1,IN_H/(STRIDE*2),IN_W/(STRIDE*2) + 1,C1_OUT,C2_OUT,STRIDE>(fm1b, fm2, conv3_w, conv3_b);
    conv_bn_relu<IN_H/(STRIDE*2),IN_W/(STRIDE*2) + 1,IN_H/(STRIDE*2),IN_W/(STRIDE*2) + 1,C2_OUT,C2_OUT,1>(fm2, fm2b, conv4_w, conv4_b);

    // 3rd conv block
    conv_bn_relu<IN_H/(STRIDE*2),IN_W/(STRIDE*2) + 1,IN_H/(STRIDE*4),IN_W/(STRIDE*4) + 1,C2_OUT,C3_OUT,STRIDE>(fm2b, fm3, conv5_w, conv5_b);
    conv_bn_relu<IN_H/(STRIDE*4),IN_W/(STRIDE*4) + 1,IN_H/(STRIDE*4),IN_W/(STRIDE*4) + 1,C3_OUT,C3_OUT,1>(fm3, fm3b, conv6_w, conv6_b);

    // GAP
    adaptive_avg_pool<IN_H/(STRIDE*4),IN_W/(STRIDE*4) + 1,C3_OUT>(fm3b, pooled);

    // Classifier
    linear<64,N_CLASSES>(pooled, out, linear1_w, linear1_b, false);

    // Write output logits
    for (int i = 0; i < N_CLASSES; i++) {
        converter.f = out[i];
        pkt.data = converter.i;
        pkt.last = (i == (N_CLASSES - 1)) ? 1 : 0;
        out_stream.write(pkt);
    }
}