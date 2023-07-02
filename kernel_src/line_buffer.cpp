#include "ap_axi_sdata.h"
#include "hls_burst_maxi.h"
#include "./types.hpp"
#define HBM_SIZE 0x10000000
void write2HBM(hls::burst_maxi<data_cell_pack> mem, hls::stream<data_cell_pack>& stream_in, hls::stream<ap_uint<1>>& flag_stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return
    while(1){
#pragma HLS PIPELINE off
        for(int count = 0; count < (HBM_SIZE >> 12); count++){
#pragma HLS PIPELINE off
            mem.write_request(count << 6, 64);
write_burst_loop:
            for(int i = 0; i < 64; i++){
#pragma HLS PIPELINE 
                data_cell_pack i_temp;
                stream_in >> i_temp;
                mem.write(i_temp);
            }
            mem.write_response();
            flag_stream_out << 1;
        }
    }
}

void readfromHBM(hls::burst_maxi<data_cell_pack> mem, hls::stream<ap_uint<1>>& flag_stream_in, hls::stream<data_cell_pack>& stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return
    while(1){
#pragma HLS PIPELINE off
        for(int count = 0; count < (HBM_SIZE >> 12); count++){
#pragma HLS PIPELINE off
            ap_uint<1> temp;
            flag_stream_in >> temp;
            if(temp){
                mem.read_request(count << 6, 64);
read_burst_loop:
                for(int i = 0; i < 64; i++){
#pragma HLS PIPELINE 
                    data_cell_pack i_temp;
                    i_temp = mem.read();
                    stream_out << i_temp;
                }
            }
        }
    }

}

extern "C" {

void line_buffer(hls::burst_maxi<data_cell_pack> mem1, hls::burst_maxi<data_cell_pack> mem2, hls::stream<data_cell_pack>& stream_in, hls::stream<data_cell_pack>& stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INTERFACE m_axi port=mem1 bundle=gmem1 num_read_outstanding=1 num_write_outstanding=4
#pragma HLS INTERFACE m_axi port=mem2 bundle=gmem2 num_read_outstanding=4 num_write_outstanding=1


    hls::stream<ap_uint<1>,32> flag_stream_1;

#pragma HLS DATAFLOW
    write2HBM(mem1, stream_in,  flag_stream_1);
    readfromHBM(mem2, flag_stream_1, stream_out);
}

}
