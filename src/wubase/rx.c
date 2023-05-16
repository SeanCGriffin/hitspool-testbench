/*
 * wub_rx.c
 *
 *  Created on: April 13, 2023
 *      Author: sgriffin
 */

#include "wubase/rx.h"
#include "base_types.h"

#include "hitspool/types.h"

#include "wubase/packet.h"

#include <stdbool.h>
#include <stdlib.h> //malloc
#include <stdbool.h>

#ifdef PLATFORM_STANDALONE
	#include<unistd.h>
#endif

/*
 * Do the GDC trick to import the enum type names.
 */
// https://www.youtube.com/watch?v=iVBCBcEANBc
#define REGISTER_ENUM(x) #x,
const char *HitspoolRCNameText[] = {
#include "hitspool/rc_names.txt"
    "INVALID"

};
#undef REGISTER_ENUM
#define REGISTER_ENUM(x) #x,
const char *PLNameText[] = {
#include "hitspool/pl_type_names.txt"
    "INVALID"

};
#undef REGISTER_ENUM
#define REGISTER_ENUM(x) #x,
const char *StreamerRCNameText[] = {
#include "steamer_rc_names.txt"
    "INVALID"

};
#undef REGISTER_ENUM

// template <typename T> u32 add_hit(T* hit_packet){
// 	u8 PMT = hit_packet->PMT; 
// 	u16 write_size = hit_packet->hit->calc_size();
// 	memcpy(write_head[PMT], (u8*)hit_packet->hit, write_size);
// 	write_head[PMT]+=write_size;
// 	n_consumed[PMT]+=write_size;
// 	nhits_inbuff[PMT]++;
	
// 	check_and_write_buffer(PMT, false);
// 	return 0;
// };

    // #ifdef PLATFORM_STANDALONE
    //     #pragma message( "Initializing file_handlers as list of pointers.")
    //     FIL* file_handlers[NUM_PMT]; //Active file handles. 
    // #else
    //     FIL file_handlers[NUM_PMT]; //Active file handles. 
    // #endif

Hitspool_RC_t smr_add_hit(streamer*s, HitPacket* hp){
	u8 PMT = hp->PMT;

	PayloadType_t type = (PayloadType_t)(hp->hitdata[0] & 0x3); //0b11
	size_t write_size = 0;

	switch(type){
		case PL_SPE:
			write_size = SPEHit_calc_size((SPEHit*)hp->hitdata);
			break;
		case PL_MPE:
			write_size = MPEHit_calc_size((MPEHit*)hp->hitdata);
			break;
		default:
			return HS_RC_INVALID;
	}

	memcpy(s->write_head[PMT], (u8*)hp->hitdata, write_size);
	s->write_head[PMT]+=write_size;
	s->n_consumed[PMT]+=write_size;
	s->nhits_inbuff[PMT]++;
	
	smr_check_and_write_buffer(s, PMT, false);

	return HS_RC_OK;
}

void smr_init_io_buffers(streamer* s){
	//Initialize all I/O buffers to 0 / NULL
    for (int i = 0; i < NUM_PMT; i++) {
        s->nhits_inbuff[i] = 0;
        sprintf(s->live_filenames[i], "NULL");
        s->handler_active[i] = false;
        s->handler_open[i] = false;

        s->n_consumed[i] = 0;
        s->buffer_full[i] = false;

        s->n_written[i] = 0; 
        s->n_written_thisfile[i] = 0;
        s->n_written_thisPMT[i] = 0;

        s->total_bytes_written = 0;
    }
}

void smr_init_write_heads(streamer* s) {
	//Clear the write buffer and  initialize write head pointer to start. 
	for (int i = 0; i < NUM_PMT; i++) {
		sprintf(s->write_buff[i], "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		//s->write_buff[i][0] = '\0';
		s->n_consumed[i] = 0;
		s->write_head[i] = (u8 *)s->write_buff[i];
	}
}

void smr_init_file_handlers(streamer* s, u64 inittime) {
    //Check if any PMTs are already open; if so close them. 
    for(int i = 0; i < NUM_PMT; i++){
        if(s->handler_open[i]){
            smr_close_file_handlers(s);
            break; //All four happen at the same time.
        }
    }

	for (int i = 0; i < NUM_PMT; i++) {
		s->nhits_inbuff[i] = 0;
		s->handler_active[i] = false;
		s->buffer_full[i] = false;
        //Filename is inittime/1000000, i.e. seconds.
		sprintf(s->live_filenames[i], "hitspool/PMT%02d/0x%08llX.spool", i, (u64)inittime/1000000);

		s->f_op_res[i] =
			f_open(&(s->file_handlers[i]), s->live_filenames[i], 
					FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
		print("PMT%02d file opened with fres=(%d)\r\n", i, s->f_op_res[i]);
        s->handler_open[i] = true;

		// f_sync(&(s->file_handlers[i]));
		
		// #ifdef PLATFORM_STANDALONE
		// 	size_t btw = 6, bw = 0;
		// 	f_write(&(s->file_handlers[i]), "1", 1, &bw);		
		// #endif
		

	}
	// print("\n");
}


// Print the first 12 bytes of the write buffer and the write head.
void smr_print_buffer_heads(streamer* s) {
    print("Buffer contents\r\n");
    print("-----------------------------------\r\n");
    char buff[6];
    for (int i = 0; i < NUM_PMT; i++) {
        print("PMT%02d:\r\n", i);
        print("Address of buffer[%02d] is %p\n", i, (void *)s->write_buff[i]);
        print("Address of head[%02d]   is %p\n", i, (void *)s->write_head[i]);
        memcpy(buff, s->write_head[i], 6);
        print("\tBuffer: %s\r\n"
              "\tHead:   %s\r\n",
              s->write_buff[i], buff);
    }
}

void smr_flush_file_handlers(streamer* s) {
	for (int i = 0; i < NUM_PMT; i++) {
		s->f_op_res[i] = f_sync(&(s->file_handlers[i]));
		if (s->f_op_res[i] != FR_OK)
			print("ERROR syncing file %s; fres = %d\r\n", s->live_filenames[i], s->f_op_res[i]);
	}
}

void smr_close_file_handlers(streamer* s) {

	for (int i = 0; i < NUM_PMT; i++) {
		s->f_op_res[i] = f_close(&(s->file_handlers[i]));
        s->handler_open[i] = false;
		if (s->f_op_res[i] != FR_OK)
			print("ERROR closing file %s; fres = %d\r\n", s->live_filenames[i], s->f_op_res[i]);
	}
	//print("Done closing all file handlers.\r\n");
}

void smr_print_IO_stats(streamer* s) {

	print("I/O stats:\r\n");
	print("------------------\r\n");
	for (int i = 0; i < NUM_PMT; i++) {
		print("PMT%02d:\r\n", i);
		print("\tbuffer_full:     %s\r\n", s->buffer_full[i] ? "TRUE" : "FALSE");
		print("\thandler_active:  %s\r\n", s->handler_active[i] ? "TRUE" : "FALSE");
		print("\tactive filename: %s\r\n", s->live_filenames[i]);
		print("\t----------------------------\r\n");
	}
	print("Total bytes written: 0x%16X\r\n", s->total_bytes_written);
}

void smr_print_IO_handlers(streamer* s) {

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

FRESULT smr_check_and_write_buffer(streamer* s, u8 PMT, bool force) {
	/*
	 *
	 * 
	 * 
	 */
	if ((s->n_consumed[PMT] >= TARGET_BLOCKSIZE) || force) {
		// Kick off the file write process.
		if (force) {
			print("Forcing write of PMT%02d buffer.\r\n"
				  "\tn_consumed: 0x%04X\r\n",
				  PMT, s->n_consumed[PMT]);
		} else {
			print("PMT%02d buffer meets threshold.\r\n"
				  "\tn_consumed: 0x%04X\r\n",
				  PMT, s->n_consumed[PMT]);
		}
		
		// do some writing things
		s->buffer_full[PMT]    = true;
		s->handler_active[PMT] = true;
		
		s->f_op_res[PMT] =
			f_write(&(s->file_handlers[PMT]), s->write_buff[PMT], 
					s->n_consumed[PMT], &(s->n_written[PMT]));
		

		if (s->f_op_res[PMT] != FR_OK) {
			print("ERROR writing file %s!\r\n", s->live_filenames[PMT]);
            print("Expected write: 0x%04X, actual  write: 0x%04X\r\n", 
                s->n_consumed[PMT], s->n_written[PMT]);
		}
		
		s->n_written_thisfile[PMT] += s->n_written[PMT];
        s->n_written_thisPMT[PMT]  += s->n_written[PMT];
		s->total_bytes_written     += s->n_written[PMT];

        print("\tn_written           = 0x%04x\r\n"
              "\tn_written_thisfile  = 0x%04x\r\n"
              "\tn_written_thisPMT   = 0x%04x\r\n"
              "\ttotal_bytes_written = 0x%04x\r\n",
              s->n_written[PMT],
              s->n_written_thisfile[PMT],
              s->n_written_thisPMT[PMT],
              s->total_bytes_written);

		// reset write heads
		s->n_consumed[PMT] = 0;
		s->write_head[PMT] = s->write_buff[PMT];

		// release the file handler
		s->buffer_full[PMT]    = false;
		s->handler_active[PMT] = false;
        print("----------------------------\r\n");    
	}
    


	return s->f_op_res[PMT];
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
	*type = (PayloadType_t)(lead[0] & 0x3); //0b11
	//*type = static_cast<PayloadType_t>(lead[0] & 0x3);

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

