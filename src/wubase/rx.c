/*
 * wub_rx.c
 *
 *  Created on: April 13, 2023
 *      Author: sgriffin
 */

#include "wubase/rx.h"
#include "base_types.h"
#include "wubase/packet.h"

#include <stdbool.h>

char live_filenames[NUM_PMT][256]; //Filenames.
bool handler_active[NUM_PMT]; //Is file actively being written to?
bool handler_open[NUM_PMT];   //Is the file open?         

FRESULT f_op_res[NUM_PMT];  //File operation results. 

#define PLATFORM_STANDALONE
#ifdef PLATFORM_STANDALONE
    #pragma message( "Compiling " __FILE__ " for DESKTOP")
    #include "ff_proxy.h"
    #define f_read(a, b, c, d) f_read(&a, b, c, d)
    #include "printer.h"
#else
    #pragma message( "Compiling " __FILE__ " for STM32")
    //#include "ff.h"
    extern void print(const char *fmt, ...);
#endif

/*
 * Do the GDC trick to import the enum type names.
 */
// https://www.youtube.com/watch?v=iVBCBcEANBc
#define REGISTER_ENUM(x) #x,
const char *StreamerRCNameText[] = {
#include "steamer_rc_names.txt"
    "INVALID"

};
#undef REGISTER_ENUM


//File handlers
char live_filenames[NUM_PMT][256]; //Filenames.
bool handler_active[NUM_PMT]; //Is file actively being written to?
bool handler_open[NUM_PMT];   //Is the file open?    
#ifdef PLATFORM_STANDALONE
    #pragma message( "Initializing file_handlers as list of pointers.")
    FIL *file_handlers[NUM_PMT]; //Active file handles. 
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


void smr_init_io_buffers(){
	//Initialize all I/O buffers to 0 / NULL
    for (int i = 0; i < NUM_PMT; i++) {
        nhits_inbuff[i] = 0;
        sprintf(live_filenames[i], "NULL");
        handler_active[i] = false;
        handler_open[i] = false;

        n_consumed[i] = 0;
        buffer_full[i] = false;

        n_written[i] = 0; 
        n_written_thisfile[i] = 0;
        n_written_thisPMT[i] = 0;

        total_bytes_written = 0;
    }
}

void smr_init_write_heads() {
	//Clear the write buffer and  initialize write head pointer to start. 
	for (int i = 0; i < NUM_PMT; i++) {
		//sprintf((char *)write_buff[i], "");
		write_buff[i][0] = '\0';
		n_consumed[i] = 0;
		write_head[i] = (u8 *)write_buff[i];
	}
}

void smr_init_file_handlers(u64 inittime) {
    //Check if any PMTs are already open; if so close them. 
    for(int i = 0; i < NUM_PMT; i++){
        if(handler_open[i]){
            close_file_handlers();
            break; //All four happen at the same time.
        }
    }

	for (int i = 0; i < NUM_PMT; i++) {
		nhits_inbuff[i] = 0;
		handler_active[i] = FALSE;
		buffer_full[i] = FALSE;
        //Filename is inittime/1000000, i.e. seconds.
		sprintf(live_filenames[i], "hitspool/PMT%02d/0x%08llX.spool", i, (u64)inittime/1000000);

		f_op_res[i] =
			f_open(&file_handlers[i], live_filenames[i], 
				FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
		print("PMT%02d file opened with fres=(%d)\r\n", i, f_op_res[i]);
        handler_open[i] = true;
	}
	// print("\n");
}


// Print the first 12 bytes of the write buffer and the write head.
void smr_print_buffer_heads() {
    print("Buffer contents\r\n");
    print("-----------------------------------\r\n");
    char buff[6];
    for (int i = 0; i < NUM_PMT; i++) {
        print("PMT%02d:\r\n", i);
        print("Address of buffer[%02d] is %p\n", i, (void *)write_buff[i]);
        print("Address of head[%02d]   is %p\n", i, (void *)write_head[i]);
        memcpy(buff, write_head[i], 6);
        print("\tBuffer: %s\r\n"
              "\tHead:   %s\r\n",
              write_buff[i], buff);
    }
}

void smr_flush_file_handlers() {
	for (int i = 0; i < NUM_PMT; i++) {
		f_op_res[i] = f_sync(&file_handlers[i]);
		if (f_op_res[i] != FR_OK)
			print("ERROR syncing file %s; fres = %d\r\n", live_filenames[i], f_op_res[i]);
	}
}

void smr_close_file_handlers() {

	for (int i = 0; i < NUM_PMT; i++) {
		f_op_res[i] = f_close(&file_handlers[i]);
        handler_open[i] = false;
		if (f_op_res[i] != FR_OK)
			print("ERROR closing file %s; fres = %d\r\n", live_filenames[i], f_op_res[i]);
	}
	//print("Done closing all file handlers.\r\n");
}

void smr_print_IO_stats() {

	print("I/O stats:\r\n");
	print("------------------\r\n");
	for (int i = 0; i < NUM_PMT; i++) {
		print("PMT%02d:\r\n", i);
		print("\tbuffer_full:     %s\r\n", buffer_full[i] ? "TRUE" : "FALSE");
		print("\thandler_active:  %s\r\n", handler_active[i] ? "TRUE" : "FALSE");
		print("\tactive filename: %s\r\n", live_filenames[i]);
		print("\t----------------------------\r\n");
	}
	print("Total bytes written: 0x%16X\r\n", this->total_bytes_written);
}

void smr_print_IO_handlers() {

	print("PMT file handlers:\r\n");
	print("------------------\r\n");
	for (int i = 0; i < NUM_PMT; i++) {
		print("PMT%02d:\r\n", i);
		// print("\tbuffer_full:     %s\r\n", buffer_full[i] ? "TRUE" : "FALSE");
		// print("\thandler_active:  %s\r\n", handler_active[i] ? "TRUE" : "FALSE");
		// print("\tactive filename: %s\r\n", live_filenames[i]);
		print("\t----------------------------\r\n");
	}

	// print("\nMost recent hit data:\r\n");
	// print("---------------------\r\n");
	// print_SPEPacket(spep);
	// print_MPEPacket(mpep);
	// print_WUBPacket(wubp);
}

FRESULT smr_check_and_write_buffer(u8 PMT, bool force) {
	/*
	 *
	 * 
	 * 
	 */
	if ((n_consumed[PMT] >= TARGET_BLOCKSIZE) || force) {
		// Kick off the file write process.
		if (force) {
			print("Forcing write of PMT%02d buffer.\r\n"
				  "\t n_consumed: 0x%04X\r\n",
				  PMT, n_consumed[PMT]);
		} else {
			print("PMT%02d buffer meets threshold.\r\n"
				  "\t n_consumed: 0x%04X\r\n",
				  PMT, n_consumed[PMT]);
		}

		// do some writing things
		buffer_full[PMT]    = TRUE;
		handler_active[PMT] = TRUE;
		f_op_res[PMT] =
			f_write(&file_handlers[PMT], write_buff[PMT], n_consumed[PMT], &n_written[PMT]);

		if (f_op_res[PMT] != FR_OK) {
			print("ERROR writing file %s!\r\n", live_filenames[PMT]);
            print("Expected write: 0x%04X, actual  write: 0x%04X\r\n", 
                n_consumed[PMT], n_written[PMT]);
		}

		n_written_thisfile[PMT] += n_written[PMT];
        n_written_thisPMT[PMT]  += n_written[PMT];
		total_bytes_written     += n_written[PMT];

        print("\tn_written           = 0x%04x\r\n"
              "\tn_written_thisfile  = 0x%04x\r\n"
              "\tn_written_thisPMT   = 0x%04x\r\n"
              "\ttotal_bytes_written = 0x%04x\r\n",
              n_written[PMT],
              n_written_thisfile[PMT],
              n_written_thisPMT[PMT],
              total_bytes_written);

		// reset write heads
		n_consumed[PMT] = 0;
		write_head[PMT] = write_buff[PMT];

		// release the file handler
		buffer_full[PMT]    = FALSE;
		handler_active[PMT] = FALSE;
        print("----------------------------\r\n");    
	}
    


	return f_op_res[PMT];
}

Streamer_RC_t smr_read_next_hit(FIL *file, PayloadType_t *type, u8 *hitbuffer) {
	/*
	 * Read the next hit from the filename buffer.
	 */

    //print("read_next_hit()\r\n");
	FRESULT fres;
	Streamer_RC_t SMR_RC = STREAMER_RC_OK;
	u8 lead[sizeof(SPEHit)]; // this is the size of a SPEHit and MPEHit base unit.
	u8 data[1024];			 // FIXME: Make this match the maximum number of samples in an MPEHit.
	UINT br = 0;
	UINT btr = 6; // or sizeof(MPEHit)

	fres = f_read(file, lead, btr, &br);
	if (fres != FR_OK) {
		if (br == 0) // EOF          
			return STREAMER_RC_EOF;
		else{ // We read out fewer bytes than expected.
            print("Error reading from file; br=%d btr=%d\r\n", br, btr);
			return STREAMER_RC_DISK_ERR;
        }
	}
	// for(int i = 0; i < sizeof(SPEHit); i++){
	//    print("%s ", toBinaryString(lead[i]).c_str());
	// }
	// print("\n");
	memcpy(data, lead, 6); // the rest of data will be filled with event data.
	// data[7] = '\0';
	// print("%s\r\n", data);
	*type = static_cast<PayloadType_t>(lead[0] & 0x3);

	size_t s = 0;
	u16 nsamples = 0;
	MPEHit *mpe;
	SPEHit *spe;

	switch (*type) {
	case PL_SPE:
		s = sizeof(SPEHit);
		hitbuffer = (u8 *)malloc(s);
		memcpy(hitbuffer, lead, s);
		SMR_RC = STREAMER_RC_OK;
        spe = (SPEHit*)hitbuffer;
        //print("%s\r\n", spe->tostring().c_str());
		break;

	case PL_MPE:
		//print("Read type: %s\r\n", PLNameText[*type]);
		nsamples = ((MPEHit *)lead)->nsamples;
		//print("nsamples from cast: %d\r\n", nsamples);

		s = sizeof(MPEHit) + sizeof(u16) * 2 * ((MPEHit *)lead)->nsamples;
		s--; // subtract 1 because the first data byte is built into MPEHit already.
		//print("Total size: 0x%X\r\n", s);
		btr = s - sizeof(MPEHit);

		fres = f_read(file, data + 6, btr, &br);
		//print("btr = %d\t br= %d\r\n", btr, br);
		if (fres != FR_OK) {
			// We read out fewer bytes than expected.
			return STREAMER_RC_DISK_ERR;
		}
		hitbuffer = (u8 *)malloc(s);
		memcpy(hitbuffer, data, s);
		mpe = (MPEHit *)hitbuffer;
		//print("%s\r\n", mpe->tostring().c_str());
		//mpe->print_samples(200);

		SMR_RC = STREAMER_RC_OK;
		break;

	default:
		mpe = NULL;
		spe = NULL;
		SMR_RC = STEAMER_RC_TYPE_ERR;
	}
	//print("%d\r\n", SMR_RC);
	return SMR_RC;
}

