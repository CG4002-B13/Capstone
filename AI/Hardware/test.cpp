#include <hls_stream.h>
#include <ap_axi_sdata.h>
using namespace std;

#define INPUT_SIZE 5
#define HIDDEN_SIZE 3
#define OUTPUT_SIZE 3

typedef union {
    float f;
    unsigned int i;
} fp_conv;

typedef ap_axiu<32, 0, 0, 0> axis_word;

void mlp_forward(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream);

int main() {
    hls::stream<axis_word> in_stream;
    hls::stream<axis_word> out_stream;

    fp_conv converter;
    axis_word pkt;

    float sample_input[INPUT_SIZE] = {1.0, -0.5, 0.3, 0.0, 0.0};

    for (int i = 0; i < INPUT_SIZE; i++) {
        converter.f = sample_input[i];
        pkt.data = converter.i;
        in_stream.write(pkt);
    }

    mlp_forward(in_stream, out_stream);

    for (int i = 0; i < OUTPUT_SIZE; i++) {
        out_stream.read(pkt);
        converter.i = pkt.data;
        float out_val = converter.f;
        cout << out_val << endl;
    }
}
