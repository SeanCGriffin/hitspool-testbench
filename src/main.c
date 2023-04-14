#include <stdio.h>

#include "ff_proxy.h"

int main(int argc, char*argv[]){

	if(argc != 2){
		printf("Usage: %s <base_directory for io>", argv[0]);
		return 1;
	}

    char* basepath = argv[1];


    long available = GetAvailableSpace(basepath);
    printf("Available space: %lu\r\n", available);

}