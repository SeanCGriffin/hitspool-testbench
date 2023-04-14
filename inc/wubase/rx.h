/*
 * wub_rx.h
 *
 *  Created on: April 13, 2023
 *      Author: sgriffin
 */

#ifndef WUBRX_H_
#define WUBRX_H_

#include "hitspool/types.h"

#define NUM_PMT 1
#define TARGET_BLOCKSIZE 4096 //Target size for writing to disk. More of a minimum than anything.
#define WRITEBUFFER_MAXISIZE TARGET_BLOCKSIZE+512 //Some fraction of an MPE hit.
#define WUBASEBUFFER_MAXSIZE (5*(sizeof(MPEHit) + 36)) //Assume 5 MPE hits sent at once?? Will need optimizing.



#endif