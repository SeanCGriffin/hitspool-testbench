//#include <stdio.h>
#include <stdlib.h>

#include "ff_proxy.h"

#include "wubase/rx.h" //streamer 
#include "hitspool/unit_tests.h"



int main(int argc, char*argv[]){

	if(argc != 2){
		printf("Usage: %s <base_directory for io>", argv[0]);
		return 1;
	}

    char* basepath = argv[1];
	printf("%s\r\n", basepath);

    long available = GetAvailableSpace(basepath);
    	printf("Available space: %lu\r\n", available);


	// size_t btw = 0, bw =0;

	// FRESULT fres = FR_NO_PATH;
	// FIL *file_handlers[1]; //Active file handles. 
	// fres = f_open(&file_handlers[0], "hitspool/PMT00/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	// print("fres = %d\r\n", fres);
	// fres = f_write(&file_handlers[0], basepath, strlen(basepath), &bw);
	// print("fres = %d\r\n", fres);
	// f_close(&file_handlers[0]);

	//return 0;
	if(!hs_hit_io_unit_test()){
		print("hs_hit_io_unit_test() exited successfully!");
	} else {
		print("hs_hit_io_unit_test() exited with an error!");
	}
	
}