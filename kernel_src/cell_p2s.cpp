#include "./types.hpp"

void cell_p2s(hls::stream<data_cell_pack>& data_cell_pack_stream_in, hls::stream<data_cell>& cell_stream_out){
#pragma HLS interface ap_ctrl_none port = return

loop_p2s:
    while(1){
#pragma HLS pipeline II = 4 style=flp
        
        data_cell_pack i_temp;
#pragma HLS aggregate variable=i_temp compact=bit
        data_cell_pack_stream_in >> i_temp;

loop_cell_p2s:
        for (ap_uint<12> i = 0; i < 4;i++){
            data_cell o_temp;
#pragma HLS aggregate variable=o_temp compact=bit
            o_temp = i_temp.data[i];
            cell_stream_out << o_temp;
        }
    
    }
    
}
