/*
 * wub_rx.h
 *
 *  Created on: April 13, 2023
 *      Author: sgriffin
 */

#ifndef WUBRX_H_
#define WUBRX_H_

#include "hitspool/types.h"

#include <stdbool.h>

#define PLATFORM_STANDALONE

#ifdef PLATFORM_STANDALONE
    #pragma message( "Compiling " __FILE__ " for DESKTOP")
    #include "ff_proxy.h"
    #define f_read(a, b, c, d) f_read(&a, b, c, d)
    #include "printer.h"
#else
    #pragma message( "Compiling " __FILE__ " for STM32")
    #include "ff.h"
    extern void print(const char *fmt, ...);
#endif

#define NUM_PMT 1
#define TARGET_BLOCKSIZE 4096 //Target size for writing to disk. More of a minimum than anything.
#define WRITEBUFFER_MAXISIZE TARGET_BLOCKSIZE+512 //Some fraction of an MPE hit.
#define WUBASEBUFFER_MAXSIZE (5*(sizeof(MPEHit) + 36)) //Assume 5 MPE hits sent at once?? Will need optimizing.

typedef struct { 
    //File handlers
    char live_filenames[NUM_PMT][256]; //Filenames.
    bool handler_active[NUM_PMT]; //Is file actively being written to?
    bool handler_open[NUM_PMT];   //Is the file open?    
	FRESULT f_op_res[NUM_PMT];  //File operation results.     

    #ifdef PLATFORM_STANDALONE
        #pragma message( "Initializing file_handlers as list of pointers.")
        FIL* file_handlers[NUM_PMT]; //Active file handles. 
    #else
        FIL file_handlers[NUM_PMT]; //Active file handles. 
    #endif

    //File I/O buffers
    u8 write_buff[NUM_PMT][WRITEBUFFER_MAXISIZE]; //Active writing buffer
    u8* write_head[NUM_PMT];    //Pointer to write head in write_buff.
    u32 n_consumed[NUM_PMT];    //Number of bytes consumed in active buffer (should be write_head - write_buff).
    bool buffer_full[NUM_PMT]; //Is the file's I/O buffer full / past threshold? 

    //Hit counters
    u16 nhits_inbuff[NUM_PMT];

    //Byte trackers.            
    UINT n_written[NUM_PMT];         //Number of bytes written in most recent output operation.
    u64 n_written_thisfile[NUM_PMT]; //Total number of bytes written to the active file handles. 
    u64 n_written_thisPMT[NUM_PMT];  //Sum total number of bytes written for a given PMT.
    u64 total_bytes_written;         //Sum total of number of bytes written to all files. 

    u64 bytes_avail; //Bytes available in filesystem.
    u64 bytes_total; //Full size of available filesystem (bytes)

} streamer;


void smr_init_io_buffers(streamer* s);
void smr_init_write_heads(streamer* s);
void smr_init_file_handlers(streamer* s, u64 inittime);
void smr_print_buffer_heads(streamer* s);
void smr_flush_file_handlers(streamer* s);
void smr_close_file_handlers(streamer* s);
void smr_print_IO_stats(streamer* s);
void smr_print_IO_handlers(streamer* s);
FRESULT smr_check_and_write_buffer(streamer* s, u8 PMT, bool force);
Streamer_RC_t smr_read_next_hit(FIL *file, PayloadType_t *type, u8 *hitbuffer);
Hitspool_RC_t smr_add_hit(streamer*s, HitPacket* hp);


#endif