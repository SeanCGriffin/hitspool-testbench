/*
 * hs_unit_tests.h
 *
 *  Created on: April 14, 2023
 *      Author: sgriffin
 */

#ifndef HS_UNIT_TESTS_H_
#define HS_UNIT_TESTS_H_

#include "hitspool/types.h"
#include "wubase/rx.h"

Hitspool_RC_t hs_write_pseudo_random_hits(streamer *s, u32 nhits_spe, u32 nhits_mpe, u32 npatterns);

Hitspool_RC_t hs_read_many_things(streamer* s, u32 nhits_spe, u32 nhits_mpe, u32 npatterns);

Hitspool_RC_t hs_hit_io_unit_test();

#endif