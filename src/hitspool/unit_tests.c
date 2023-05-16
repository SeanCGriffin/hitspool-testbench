/*
 * hs_unit_tests.c
 *
 *  Created on: April 14, 2023
 *      Author: sgriffin
 */

#include "hitspool/unit_tests.h"

#include "base_types.h"
#include "hitspool/types.h"
#include "wubase/rx.h"

#include <stdlib.h>

int get_system_time(){
	return 0;
}

//Write a pattern of nhits_spe, nhits_mpe, npatterns times.
Hitspool_RC_t hs_write_pseudo_random_hits(streamer *s, u32 nhits_spe, u32 nhits_mpe, u32 npatterns){
	// print("-----------------------------------\r\n");
	// print("---- hs_write_many_things() ----\r\n");
	// print("-----------------------------------\r\n\n");
    // srand(0); //seed == 0
    // u16 nsamples = 24;
	// u16 waveform_buffer[2 * nsamples];
	// for (int i = 0; i < nsamples; i++){
	// 	waveform_buffer[i] = 350;
    //     waveform_buffer[i + nsamples] = 3700;
    // }

    // int offset = rand() % 10; //int between 0 and 10
	// SPEHit *speh = new SPEHit(0xABCD, 0x5, 0xA, 0xF);
	// MPEHit *mpeh = new (nsamples) MPEHit(0xCDEF, 0x4, nsamples, (u8 *)waveform_buffer);	
}

Hitspool_RC_t hs_read_many_things(streamer* s, u32 nhits_spe, u32 nhits_mpe, u32 npatterns){
    // print("-----------------------------------\r\n");
    // print("---- hs_read_many_things() ----\r\n");
    // print("-----------------------------------\r\n\n");

    // u16 nsamples = 256;
    // u16 waveform_buffer[2 * nsamples];
    // for (int i = 0; i < 2 * nsamples; i++)
    //     waveform_buffer[i] = 2 * nsamples - i;

    // SPEHit *speh = {0xABCD, 0x5, 0xA, 0xF};
    // MPEHit *mpeh = new (nsamples) MPEHit(0xCDEF, 0x4, nsamples, (u8 *)waveform_buffer); 
}

Hitspool_RC_t hs_hit_io_unit_test() {

	print("-----------------------------------\r\n");
	print("---- hs_hit_io_unit_test() ----\r\n");
	print("-----------------------------------\r\n\n");

	//streamer *s = malloc(sizeof(streamer));
	streamer smr = {0};
	streamer *s = &smr;

	print("Initializing write buffers, heads... ");
	smr_init_write_heads(s);
	print("Done.\r\n");

	print("Initializing I/O buffers...");
	smr_init_io_buffers(s);
	print("Done.\r\n");

	print("Initializing file handlers...\r\n");
	smr_init_file_handlers(s, get_system_time());
	print("File handlers done.\r\n");
	print("-----------------------------------\r\n");
	smr_print_buffer_heads(s);
	print("-----------------------------------\r\n");


	//Build the test items. 

	u16 nsamples = 10;
	u16 waveform_buffer[2 * nsamples];
	for (int i = 0; i < 2 * nsamples; i++)
		waveform_buffer[i] = i; //2 * nsamples - i;

	// For testing....
	char buff[256];
	char pattern[7];
	sprintf(pattern, "SCGPHD");
	
	SPEHit sstr = {{PL_SPE, 0x1, 0x2}, 0x3, 0x4, 0x5};
    SPEHit *speh = &sstr;
	speh->header.pl_type = PL_SPE;
	
	// SPEHit_tostring(buff, speh);
	// print("%s", buff);

	HitPacket *spep = malloc(sizeof(HitPacket) + sizeof(SPEHit));
	spep->PMT = 0;
	spep->trecv = 0;
	memcpy(spep->hitdata, speh, SPEHit_calc_size(speh));



	MPEHit *mpeh = (MPEHit*)malloc(sizeof(MPEHit) + sizeof(u16) * 2 * nsamples);
	mpeh->header.launch_time = 0x93;
	mpeh->header.tdc = 0x12;
	mpeh->nsamples = nsamples;
	memcpy(mpeh->waveform, waveform_buffer, 2*2*nsamples);
	// MPEHit_tostring(buff, mpeh);
	// print("%s", buff);
	
	HitPacket *mpep = malloc(sizeof(HitPacket) + MPEHit_calc_size(mpeh));
	mpep->PMT = 0;
	mpep->trecv = 0;
	memcpy(mpep->hitdata, mpeh, MPEHit_calc_size(mpeh));

	// print("Test MPEHit predicted size: 0x%X + 0x%X + 0x%X = 0x%X\r\n", 
	// 	sizeof(HitHeader),
	// 	2,
	// 	2*2*mpeh->nsamples,
	// 	sizeof(HitHeader) + 2 + 2*2*mpeh->nsamples
	// );
    // print("Test MPEHit calculated size: 0x%X\r\n", MPEHit_calc_size(mpeh));
		 

	int nhits_to_write = 5000;
	int nhits_written = 0;
	for (int i = 0; i < nhits_to_write; i++) {
		//print("%d\r\n", i);
		spep->trecv++;
		smr_add_hit(s, spep);
		nhits_written++;
	}


	// Flush the write buffers and close the files.
	for (int i = 0; i < NUM_PMT; i++) {
		smr_check_and_write_buffer(s, i, true);
	}
	smr_close_file_handlers(s);

	// smr_print_buffer_heads();
	print("-----------------------------------\r\n");
	print("Opening PMT0 file for reading.\r\n");


	FRESULT fres = f_open(&(s->file_handlers[0]), s->live_filenames[0], FA_READ);
	if (fres != FR_OK)
		print("Error opening file for reading.\r\n");

	print("Reading hits...\r\n");
	print("-----------------------------------\r\n");

	Streamer_RC_t read_status = STREAMER_RC_OK;
	PayloadType_t next_hit_type;
	u8 *next_hit_contants = NULL;
	u8 rbuff[1024];
	UINT br = 0;
	UINT btr = 6; // or sizeof(MPEHit)

	int nhits_read = 0;
	while (1) {
		//print("Attempting to read hit %d\r\n", nhits_read);
#ifdef PLATFORM_STANDALONE
		read_status = smr_read_next_hit((s->file_handlers[0]), &next_hit_type, next_hit_contants);
#else
		read_status = smr_read_next_hit(&(s->file_handlers[0]), &next_hit_type, next_hit_contants);
#endif
		if (read_status != STREAMER_RC_OK) {
			if (read_status == STREAMER_RC_EOF) {
				print("Reached EOF; exiting read.\r\n");
			} else {
				print("Streamer exited with code %d\r\n", read_status);
			}

			break;
		} else {
			nhits_read++;
		}
		if (next_hit_contants != NULL)
			free(next_hit_contants);
	}
	if (nhits_read == nhits_written)
		return HS_RC_OK;
	else
		return HS_RC_NOTOK;
}
