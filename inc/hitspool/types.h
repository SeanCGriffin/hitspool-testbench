/*
 * hs_types.h
 *
 *  Created on: Oct 28, 2021
 *      Author: sgriffin
 */

#ifndef INC_HS_TYPES_H_
#define INC_HS_TYPES_H_

#define REGISTER_ENUM(x) x,
typedef enum {
	#include "hitspool/rc_names.txt"
	PL_INVALID
} Hitspool_RC_t;
#undef REGISTER_ENUM

#define REGISTER_ENUM(x) x,
typedef enum {
    #include "steamer_rc_names.txt"
    RC_INVALID
} Streamer_RC_t;
#undef REGISTER_ENUM

extern const char* PLNameText[];
extern const char* StreamerRCNameText[];

#endif /* INC_HS_TYPES_H_ */
