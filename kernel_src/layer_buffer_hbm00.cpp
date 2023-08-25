#include "hls_task.h"
#include "ap_axi_sdata.h"
#include "hls_burst_maxi.h"
#include "./types.hpp"
#define HBM_SIZE 0x10000000
#define HBM_OFFSET 0x00000000

void cell_p2s(hls::stream<data_cell_pack>& data_cell_pack_stream_in, hls::stream<data_cell>& cell_stream_out){
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS PIPELINE style=flp II=1
    static data_cell_pack i_temp;
    static ap_uint<3> counter = 0;
    static bool inited=false;

    if(!inited){
        counter = 0;
        if(!data_cell_pack_stream_in.empty()){
            inited = true;
        }
    }
    else{
        if (counter == 0){
            data_cell_pack_stream_in >> i_temp;
        }
        data_cell o_temp;
        o_temp = i_temp.data[counter];
        cell_stream_out << o_temp;
        if(counter == 3){
            counter = 0;
        }
        else{
            counter++;
        }
    }
}

void cell_s2p(hls::stream<data_cell>& cell_stream_in, hls::stream<data_cell_pack>& data_cell_pack_stream_out){
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS PIPELINE style=flp II=1
    static data_cell_pack o_temp;
    static ap_uint<3> counter = 0;
    static bool inited=false;

    if(!inited){
        o_temp.data[0] = {};
        o_temp.data[1] = {};
        o_temp.data[2] = {};
        o_temp.data[3] = {};
        counter = 0;
        if(!cell_stream_in.empty()){
            inited = true;
        }
    }
    else{
        data_cell i_temp;
        cell_stream_in >> i_temp;
        o_temp.data[counter] = i_temp;
        if(counter == 3){
            data_cell_pack_stream_out << o_temp;
        }
        if(counter == 3){
            counter = 0;
        }
        else{
            counter++;
        }
    }
}
void write2HBM(hls::burst_maxi<data_cell_pack> mem, hls::stream<data_cell_pack>& stream_in, hls::stream<ap_uint<1>>& flag_stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return

    for(int count = 0; count < (HBM_SIZE >> 12); count++){
#pragma HLS PIPELINE off
            mem.write_request((HBM_OFFSET >> 6) + (count << 6), 64);
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

void readfromHBM(hls::burst_maxi<data_cell_pack> mem, hls::stream<ap_uint<1>>& flag_stream_in, hls::stream<data_cell_pack>& stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return
    for(int count = 0; count < (HBM_SIZE >> 12); count++){
#pragma HLS PIPELINE off
        ap_uint<1> temp;
        flag_stream_in >> temp;
        if(temp){
            mem.read_request((HBM_OFFSET >> 6) + (count << 6), 64);
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

extern "C" {

void layer_buffer_hbm00(hls::burst_maxi<data_cell_pack> mem1, hls::burst_maxi<data_cell_pack> mem2, hls::stream<data_cell>& stream_in, hls::stream<data_cell>& stream_out){
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INTERFACE m_axi port=mem1 bundle=gmem1 num_read_outstanding=1 num_write_outstanding=4 latency=4 offset=slave
#pragma HLS INTERFACE m_axi port=mem2 bundle=gmem2 num_read_outstanding=4 num_write_outstanding=1 latency=4 offset=slave


    hls_thread_local hls::stream<ap_uint<1>, 96> flag_stream_1;
    hls_thread_local hls::stream<data_cell_pack> pack_stream_out;
    hls_thread_local hls::stream<data_cell_pack> pack_stream_in;

    hls_thread_local hls::task s2p(cell_s2p, stream_in, pack_stream_out);
    hls_thread_local hls::task inst_s2mm(write2HBM, mem1, pack_stream_out,  flag_stream_1);
    hls_thread_local hls::task inst_mm2s(readfromHBM, mem2, flag_stream_1, pack_stream_in);
    hls_thread_local hls::task p2s(cell_p2s, pack_stream_in, stream_out);
}

}
