#include "./types.hpp"

void cell_s2p(hls::stream<data_cell>& cell_stream_in, hls::stream<data_cell_pack>& data_cell_pack_stream_out){
#pragma HLS interface ap_ctrl_none port = return

loop_s2p:
    while(1){
#pragma HLS pipeline II = 4 style=flp

        data_cell_pack o_temp;
#pragma HLS aggregate variable=o_temp compact=bit

loop_cell_s2p:
        for (ap_uint<12> i = 0; i < 4;i++){
            data_cell i_temp;
#pragma HLS aggregate variable=i_temp compact=bit
            cell_stream_in >> i_temp;
            o_temp.data[i] = i_temp;
        }

        data_cell_pack_stream_out << o_temp;
    }
}
