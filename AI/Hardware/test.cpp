#include <iostream>
#include <cmath>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
using namespace std;

#define INPUT_SIZE 5
#define HIDDEN_SIZE 3
#define OUTPUT_SIZE 3

typedef ap_axiu<32,0,0,0> axis_word;

void mlp_forward(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream);

int main() {
    hls::stream<axis_word> in_stream;
    hls::stream<axis_word> out_stream;

    float sample_input[INPUT_SIZE] = {1.0, -0.5, 0.3, 0.0, 0.0};

    for (int i = 0; i < INPUT_SIZE; i++) {
        axis_word pkt;
        pkt.data = *(ap_uint<32>*)&sample_input[i];   // reinterpret float bits as uint32
        pkt.last = (i == INPUT_SIZE - 1) ? 1 : 0;     // mark last element
        in_stream.write(pkt);
    }

    // Call your HLS MLP function
    mlp_forward(in_stream, out_stream);

    // Read and unpack output floats
    cout << "MLP Output:" << endl;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        axis_word pkt = out_stream.read();
        float val = *(float*)&pkt.data;  // reinterpret back to float
        cout << val << endl;
    }

    return 0;
}
