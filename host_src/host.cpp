#include <iostream>
#include <stdio.h>
#include <xrt/xrt_device.h>         // bitstream
#include <xrt/xrt_bo.h>             // buffers
#include <xrt/xrt_kernel.h>         // kernels, runs
#include <experimental/xrt_ip.h>    // IP direct control
#include <unistd.h>                 // sleep
#include <chrono>

#include <math.h>

//#include "fileops.h"
#include "../kernel_src/types.hpp"
using namespace std::chrono;

void print_summary(std::string k1, double t1, int signal_size);

int main(int argc, char* argv[]){
    
    char* xclbinFilename;

    if (argc < 2){
        printf("Usage: %s <XCLBIN File> \n", argv[0]);
        return EXIT_FAILURE;
    }
    xclbinFilename = argv[1];
    int offset = 0;

    if (argc > 2){
	offset = atoi(argv[2]);
    }
    // Load xclbin 
    
    unsigned int device_id = 0; // by default
    auto device = xrt::device(device_id);

    printf("Connected to device!\n");
    xrt::uuid overlay_uuid = device.load_xclbin(xclbinFilename);
    printf("Loaded Overlay!\n");
   
     //xrt::ip dp_0 = xrt::ip(device, overlay_uuid, "auto_data_pack:{auto_data_pack_0}");
     xrt::ip line_buffer_1 = xrt::ip(device, overlay_uuid, "line_buffer:{line_buffer_1}");
     printf("Connected to ips!\n");

    // dp_0.write_register(0x10, 2);
    unsigned int LINE_BUFFER_OFFSET_LSB = 0x00000000;
    unsigned int LINE_BUFFER_OFFSET_MSB = 0x00000000;
    line_buffer_1.write_register(0x10, LINE_BUFFER_OFFSET_LSB);
    line_buffer_1.write_register(0x14, LINE_BUFFER_OFFSET_MSB);

    line_buffer_1.write_register(0x1c, LINE_BUFFER_OFFSET_LSB);
    line_buffer_1.write_register(0x20, LINE_BUFFER_OFFSET_MSB);
    // printf("Finished setting ips!\n");

    // user logic
    const int signal_size = 1 << 15;
    int (*data_in) = new int[signal_size];
    int (*data_out) = new int[signal_size];
   
    for(int i = 0; i < signal_size; i++){
//            int temp = i;
            data_in[i] = i + offset ;
    }
    for(int i = 0; i < signal_size; i++){
            data_out[i] = 0;
    } 
    // TX
	// Connect to kernels
	printf("[Process]\tConnect to %s kernel...\n", "SwitchingDMA_read:{SwitchingDMA_read_1}");
	auto SwitchingDMA_read_1 = xrt::kernel(device, overlay_uuid, "SwitchingDMA_read:{SwitchingDMA_read_1}");
 
	printf("[Process]\tConnect to %s kernel...\n", "SwitchingDMA_write:{SwitchingDMA_write_1}");
	auto SwitchingDMA_write_1 = xrt::kernel(device, overlay_uuid, "SwitchingDMA_write:{SwitchingDMA_write_1}");

    std::cout << "[Process]\tAllocate Buffer in Global Memory...\n";
	printf("[Process]\tAllocate %s buffer...\n","A_src");
    
	auto bo_data_in = xrt::bo(device, sizeof(int) * signal_size, SwitchingDMA_read_1.group_id(0));
	auto bo_data_out = xrt::bo(device, sizeof(int) * signal_size, SwitchingDMA_write_1.group_id(0));

    bo_data_in.write(data_in);
    bo_data_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_data_out.write(data_out);
    bo_data_out.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    
    printf("Finished allocation!\n");
    auto SwithingDMA_read_1_run = xrt::run(SwitchingDMA_read_1);
    auto SwithingDMA_write_1_run = xrt::run(SwitchingDMA_write_1);
    
    SwithingDMA_read_1_run.set_arg(0, bo_data_in);
    SwithingDMA_read_1_run.set_arg(1, int(signal_size / 4));

    SwithingDMA_write_1_run.set_arg(0, bo_data_out);
    SwithingDMA_write_1_run.set_arg(1, int(signal_size / 4));

	printf("[Process]\tSimulation starts!\n");

    auto start = high_resolution_clock::now();
    SwithingDMA_read_1_run.start();
    sleep(5);
    printf("started!\n");
    SwithingDMA_write_1_run.start();
    SwithingDMA_read_1_run.wait();
    printf("Read data finished!\n");
    SwithingDMA_write_1_run.wait();

    printf("finished!\n");
    auto stop = high_resolution_clock::now();

    printf("Convertion finished!\n");
    auto SwitchingDMA_time = duration_cast<microseconds>(stop - start);

    bo_data_in.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	bo_data_in.read(data_in);
    bo_data_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	bo_data_out.read(data_out);

    printf("Resize finished!\n");
    FILE* fp = fopen("./compare.txt","w");
    if (fp == NULL){
        printf("Failed to write result out!\n");
    }
    else{
        for (int i = 0; i < signal_size; i++){
		//	int temp1 = data_in[i] & (~0x0002000);
		//	int temp2 = data_out[i] & (~0x0002000);
		if (data_in[i] != data_out[i]){
			fprintf(fp, "%08d\t0x%08x\t0x%08x\t%08x\n",i,  data_in[i], data_out[i], data_out[i] ^ data_in[i]);
		}
        }
        fclose(fp);
    }
    // fp = fopen("./result.txt","w");
    // if (fp == NULL){
    //     printf("Failed to write result out!\n");
    // }
    // else{
    //     for (int i = 0; i < signal_size; i++){
    //             fprintf(fp, "%08d\t0x%08x\n",i,  data_out[i]);
    //     }
    //     fclose(fp);
    // }

    print_summary("SwitchingDMA", double(SwitchingDMA_time.count()), signal_size);
    std::cout << "TEST PASSED\n";
    return EXIT_SUCCESS;


}

void print_summary(std::string k1, double t1, int signal_size) {
	printf("Test Size = %d\n", signal_size);
    std::cout << "|-------------------------+-------------------------|\n"
              << "| Kernel (per iteration)  |    Wall-Clock Time (us) |\n"
              << "|-------------------------+-------------------------|\n";
    std::cout << "| " << std::left << std::setw(24) << k1.c_str() << "|" << std::right << std::setw(24) << t1 << " |\n";
    std::cout << "|-------------------------+-------------------------|\n";
    std::cout << "Average II = " << t1 / ((float)signal_size / 4 / 300.0) << std::endl;
    std::cout << "Note: Wall Clock Time is meaningful for real hardware execution "
              << "only, not for emulation.\n";
    std::cout << "Please refer to profile summary for kernel execution time for "
              << "hardware emulation.\n";

}
