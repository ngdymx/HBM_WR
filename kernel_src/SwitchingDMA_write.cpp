#include "./types.hpp"


void s2mm(data_cell mem[], int N, hls::stream<data_cell>& i_stream){
loop_s2mm:
	for (int i = 0; i < N; i++){
#pragma HLS pipeline
		data_cell payload;
#pragma HLS aggregate variable=payload compact=bit
		i_stream >> payload;
		mem[i] = payload;
	}
}
extern "C" {

void SwitchingDMA_write(data_cell WritePort[], int M, hls::stream<data_cell>& i_stream){
#pragma HLS INTERFACE s_axilite port = return
#pragma HLS INTERFACE m_axi port = WritePort offset = slave bundle = gmem_write num_read_outstanding=1  num_write_outstanding=4 max_read_burst_length=2 max_write_burst_length=64

        s2mm(WritePort, M, i_stream);
}

}
