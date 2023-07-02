#include "./types.hpp"

void mm2s(data_cell mem[], int N, hls::stream<data_cell>& o_stream){
loop_mm2s:
	for (int i = 0; i < N; i++){
#pragma HLS pipeline
		data_cell value = mem[i];
		o_stream << value; 
	}
}


extern "C" {

void SwitchingDMA_read(data_cell ReadPort[], int M, hls::stream<data_cell>& o_stream){
#pragma HLS INTERFACE s_axilite port = return
#pragma HLS INTERFACE m_axi port = ReadPort offset = slave bundle = gmem_read num_read_outstanding=4  num_write_outstanding=1 max_read_burst_length=64 max_write_burst_length=2

    mm2s(ReadPort, M, o_stream);
}

}
