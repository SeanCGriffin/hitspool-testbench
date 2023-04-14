/*
 * hs_unit_tests.c
 *
 *  Created on: April 14, 2023
 *      Author: sgriffin
 */

#include "hitspool/unit_tests.h"

#include "base_types.h"
#include "hitspool/types.h"


//Write a pattern of nhits_spe, nhits_mpe, npatterns times.
Hitspool_RC_t hs_write_pseudo_random_hits(streamer *s, u32 nhits_spe, u32 nhits_mpe, u32 npatterns){
	print("-----------------------------------\r\n");
	print("---- hs_write_many_things() ----\r\n");
	print("-----------------------------------\r\n\n");
    srand(0); //seed == 0
    u16 nsamples = 24;
	u16 waveform_buffer[2 * nsamples];
	for (int i = 0; i < nsamples; i++){
		waveform_buffer[i] = 350;
        waveform_buffer[i + nsamples] = 3700;
    }

    int offset = rand() % 10; //int between 0 and 10
	SPEHit *speh = new SPEHit(0xABCD, 0x5, 0xA, 0xF);
	MPEHit *mpeh = new (nsamples) MPEHit(0xCDEF, 0x4, nsamples, (u8 *)waveform_buffer);	
}

Hitspool_RC_t hs_read_many_things(streamer* s, u32 nhits_spe, u32 nhits_mpe, u32 npatterns){
    print("-----------------------------------\r\n");
    print("---- hs_read_many_things() ----\r\n");
    print("-----------------------------------\r\n\n");

    u16 nsamples = 256;
    u16 waveform_buffer[2 * nsamples];
    for (int i = 0; i < 2 * nsamples; i++)
        waveform_buffer[i] = 2 * nsamples - i;

    SPEHit *speh = new SPEHit(0xABCD, 0x5, 0xA, 0xF);
    MPEHit *mpeh = new (nsamples) MPEHit(0xCDEF, 0x4, nsamples, (u8 *)waveform_buffer); 
}

Hitspool_RC_t hs_hit_io_unit_test() {

	print("-----------------------------------\r\n");
	print("---- hs_hit_io_unit_test() ----\r\n");
	print("-----------------------------------\r\n\n");

	streamer *s = new streamer();

	// print("Initializing write buffers, heads... ");
	// s->init_write_heads();
	// print("Done.\r\n");
	print("Initializing file handlers...\r\n");
	s->init_file_handlers(get_system_time());
	print("File handlers done.\r\n");
	print("-----------------------------------\r\n");
	s->print_buffer_heads();
	print("-----------------------------------\r\n");

    print("Hit: %d\t SPEHit size: %d\t MPEHit size: %d\r\n", sizeof(Hit), sizeof(SPEHit), sizeof(MPEHit) - 1);
	u16 nsamples = 10;
	u16 waveform_buffer[2 * nsamples];
	for (int i = 0; i < 2 * nsamples; i++)
		waveform_buffer[i] = i; //2 * nsamples - i;

	// For testing....
	char pattern[7];
	sprintf(pattern, "SCGPHD");
	

    SPEHit *spe_pattern = (SPEHit *)pattern;
	SPEHit *speh = new SPEHit(0xA, 0xB, 0xC, 0xD);

	MPEHit *mpeh = new (nsamples) MPEHit(0xCDEF, 0x4, nsamples, (u8 *)waveform_buffer);
    // print("Test MPEHit predicted size: 0x%X + 0x%X = 0x%X\r\n", 
    // sizeof(MPEHit)-1, 2*nsamples*sizeof(u16), 
    // sizeof(MPEHit)-1 + 2*nsamples*sizeof(u16));
    // print("Test MPEHit calculated size: 0x%X\r\n", mpeh->calc_size());

	// mpeh->print_samples(5);
	//  u8* hitbytes = (u8*)malloc(mpeh->calc_size() + speh->calc_size());
	//  memcpy(hitbytes, (u8*)mpeh, mpeh->calc_size());
	//  memcpy(hitbytes + mpeh->calc_size(), (u8*)mpeh, mpeh->calc_size());
	//  WUBuffer* wubuff = new WUBuffer(mpeh->calc_size() + speh->calc_size(), 2,
	//  hitbytes);
	//  //printf("%s\r\n", speh->tostring().c_str());

	hitpacket<SPEHit> *spep = new hitpacket<SPEHit>(0, 0x1234ABCD, speh);
	hitpacket<MPEHit> *mpep = new hitpacket<MPEHit>(0, 0x1234ABCD, mpeh);

	hitpacket<SPEHit> *spep_pattern = new hitpacket<SPEHit>(0, 0x1234ABCD, spe_pattern);
	spep_pattern->hit->pl_type = PL_SPE;

	int nhits_to_write = 5000;
	int nhits_written = 0;
	for (int i = 0; i < nhits_to_write; i++) {
		spep->hit->tdc++;
		s->add_hit(spep);
		nhits_written++;
	}

	
    // for (int i = 0; i < nhits_to_write; i++){
    //     ((u16*)(mpep->hit->waveform))[i%nsamples]+=100;
    //     mpep->hit->tdc++; 
    //     s->add_hit(mpep); nhits_written++;
    // }
    


	// Flush the write buffers and close the files.
	for (int i = 0; i < NUM_PMT; i++) {
		s->check_and_write_buffer(i, true);
	}
	s->close_file_handlers();

	// s->print_buffer_heads();
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
		// print("Attempting to read hit %d\r\n", nhits_read);
#ifdef PLATFORM_STANDALONE
		read_status = s->read_next_hit((s->file_handlers[0]), &next_hit_type, next_hit_contants);
#else
		read_status = s->read_next_hit(&(s->file_handlers[0]), &next_hit_type, next_hit_contants);
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
		return G_OK;
	else
		return G_NOTOK;
}
