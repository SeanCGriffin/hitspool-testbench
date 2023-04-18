/*
 * hs_types.h
 *
 *  Created on: Oct 28, 2021
 *      Author: sgriffin
 */

#ifndef INC_HS_TYPES_H_
#define INC_HS_TYPES_H_

#include "base_types.h"

#define REGISTER_ENUM(x) x,
typedef enum {
	#include "hitspool/rc_names.txt"
	HS_RC_INVALID
} Hitspool_RC_t;
#undef REGISTER_ENUM

#define REGISTER_ENUM(x) x,
typedef enum {
	#include "hitspool/pl_type_names.txt"
	PL_INVALID
} PayloadType_t;
#undef REGISTER_ENUM

#define REGISTER_ENUM(x) x,
typedef enum {
    #include "steamer_rc_names.txt"
    RC_INVALID
} Streamer_RC_t;
#undef REGISTER_ENUM

extern const char* HitspoolRCNameText[];
extern const char* PLNameText[];
extern const char* StreamerRCNameText[];

typedef struct HitPacket{
    u8 PMT : 8;
    u64 trecv : 48;
    u8 hitdata[]; //SPEHit, MPEHit, WUBBuf
} __attribute__ ((__packed__)) HitPacket;


typedef struct HitHeader{
    PayloadType_t pl_type : 2;
    u8 tdc : 6;
    u16 launch_time : 16;
} __attribute__ ((__packed__)) HitHeader;

typedef struct SPEHit{
    HitHeader header;
    u8 subsample : 7;
    u16 charge   : 12;
    u8 padding   : 5;
} __attribute__ ((__packed__)) SPEHit;

typedef struct MPEHit{
    HitHeader header;
    u16 nsamples : 16;
    u16 waveform[];
} __attribute__ ((__packed__)) MPEHit;

#endif /* INC_HS_TYPES_H_ */
