/*
 * packet.h
 *
 *  Created on: Nov 16, 2021
 *      Author: sgriffin
 */

#ifndef PACKET_H_
#define PACKET_H_


#include <stdlib.h> //size_t
#include "hitspool/types.h"

size_t SPEHit_calc_size(SPEHit* h);
void SPEHit_tostring(char str[], SPEHit* h);

size_t MPEHit_calc_size(MPEHit* h);
void MPEHit_tostring(char str[], MPEHit*h );
void MPEHit_print_samples(MPEHit* h, u16 nsamples);

#endif /* PACKET_H_ */
