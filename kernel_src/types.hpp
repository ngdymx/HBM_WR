#ifndef __TYPEDEF_HPP__
#define __TYPEDEF_HPP__

#include "ap_fixed.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

//#define DEBUG

typedef ap_int<128> data_cell; // data appears on the interface

typedef struct{
    data_cell data[4];
}data_cell_pack;

#endif 
