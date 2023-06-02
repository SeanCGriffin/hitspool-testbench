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


    if(!hs_hit_io_unit_test()){
		print("hs_hit_io_unit_test() exited successfully!\r\n");
	} else {
		print("hs_hit_io_unit_test() exited with an error!\r\n");
	}
	
}