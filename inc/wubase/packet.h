/*
 * packet.h
 *
 *  Created on: Nov 16, 2021
 *      Author: sgriffin
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "base_types.h"
#include "hs_types.h"

#include <cstring>
#include <string>
//#include <new>

extern "C" {
	extern void print(const char *fmt, ...);
}
namespace hitspool {


//FIXME: When exact size/shape of fine timestamps is available, this will need a rejigger.
template <class T>
    class __attribute__ ((__packed__)) hitpacket {

        public:

            u8 PMT : 8;
            u64 trecv : 48;
            T* hit; //SPEHit, MPEHit, WUBBuf

        hitpacket(u8 PMT, u64 trecv, T* h){
        	this->PMT = PMT;
        	this->trecv = trecv;
            this->hit = (T*)malloc(h->calc_size());
            memcpy((void*)this->hit, (void*)h, h->calc_size());
        };

        ~hitpacket(){
        	delete this->hit;
        };

        std::string tostring(){
		    char buffer[1024];
		    sprintf(buffer, "Packet info:\r\n"
		            "\t Type:        %s\r\n"
		            "\t PMT:         0x%8X\r\n"
		            "\t trecv:       0x%8X\r\n"
		            "%s",
		            PLNameText[this->hit->pl_type],
		            this->PMT,
		            this->trecv,
		            this->hit->tostring().c_str()
		            );

			return std::string(buffer);			
        }
};

//Base Hit class containing attributes common to MPE and SPE hits.
class __attribute__ ((__packed__)) Hit {
	public:
		
		PayloadType_t pl_type : 2;
		u8 tdc : 6;
		u16 launch_time : 16;

		Hit();
		Hit(PayloadType_t pl_type, u16 launch_time, u8 tdc);
		~Hit();

};

class __attribute__ ((__packed__)) SPEHit : public Hit {

	public:
		u8 subsample : 7;
		u16 charge   : 12;
		u8 padding   : 5;

		SPEHit();
		SPEHit(u64 launch_time, u8 tdc, u8 subsample, u16 charge);
		~SPEHit();

		size_t calc_size();
		std::string tostring();
};

class __attribute__ ((__packed__)) MPEHit : public Hit {

	public:
		u16 nsamples : 16;
		u8 waveform[1];

		void* operator new(size_t size, u16 nsamples);
		MPEHit();
		MPEHit(u64 launch_time, u8 tdc, u16 nsamples, u8 *waveform);
		~MPEHit();

		void print_samples(u16 nsamples);

		size_t calc_size();
		std::string tostring();

};

// class __attribute__ ((__packed__)) MPETest : public Hit {

// 	public:
// 		u16 nsamples : 16;
// 		u8 waveform[1];

// 		MPETest(u64 launch_time, u8 tdc, u16 nsamples, u8 *waveform);
// 		~MPETest();

// 		void* operator new(size_t size, size_t nsamples){
//         	//printf("Overriding new operator; size is %d...%d\r\n", size, nsamples);
// 	        void* p = ::operator new(size + nsamples * sizeof(u8));		

// 	        return p;
// 	    };
// };

class __attribute__ ((__packed__)) WUBuffer {

	public:
		u32 size              : 32; //Payload size (bytes)
		u16 nhits             : 16; //Number of hits that were in this buffer
		PayloadType_t pl_type : 2;
		u8 padding            : 6;
		u8 *data; //Data buffer; a concatenated list of MPEHits and SPEHits.

		WUBuffer();
    	WUBuffer(u16 size, u16 nhits, u8* data);
    	~WUBuffer();

    	size_t calc_size();
		std::string tostring();

};

} /* namespace hitspool */

#endif /* PACKET_H_ */
