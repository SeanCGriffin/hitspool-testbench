/*
 * packet.cpp
 *
 *  Created on: Nov 16, 2021
 *      Author: sgriffin
 */


#include "wub/packet.h"

#include "base_types.h"
#include "hitspool/types.h"

#ifdef PLATFORM_STANDALONE
    #pragma message( "Compiling " __FILE__ " for DESKTOP")
        #include "printer.h"
#else
    #pragma message( "Compiling " __FILE__ " for STM32")
    extern void print(const char *fmt, ...);
#endif

typedef struct {
    PayloadType_t pl_type : 2;
    u8 tdc : 6;
    u16 launch_time : 16;
} __attribute__ ((__packed__)) HitHeader;

typedef struct {
    HitHeader header : sizeof(HitHeader);
    u8 subsample : 7;
    u16 charge   : 12;
    u8 padding   : 5;
} __attribute__ ((__packed__)) SPEHit;

typedef struct {
    HitHeader header : sizeof(HitHeader);
    u16 nsamples : 16;
    u8 waveform[1];
} __attribute__ ((__packed__)) MPEHit;

#define calc_hit_size(X) _Generic((X), SPEHit: SPEHit_getsize, MPEHit: MPEHit_getsize)(X)

size_t SPEHit_getsize(SPEHit* h){
    return sizeof(SPEHit);
}

void SPEHit_tostring(char str[], SPEHit* h){
    sprintf(str, "SPEHit info:\r\n"
            "\t launch_t:    0x%8X\r\n"
            "\t tdc:         0x%8X\r\n"
            "\t subsample:   0x%8X\r\n"
            "\t charge:      0x%8X\r\n",
            h->launch_time,
            h->tdc,
            h->subsample,
            h->charge
            );
}

std::string SPEHit::tostring(){
    char buffer[1024];
    sprintf(buffer, "SPEHit info:\r\n"
            "\t launch_t:    0x%8X\r\n"
            "\t tdc:         0x%8X\r\n"
            "\t subsample:   0x%8X\r\n"
            "\t charge:      0x%8X\r\n",
            this->launch_time,
            this->tdc,
            this->subsample,
            this->charge
            );
    return std::string(buffer);
}

    //MPEHit constructor/destructor
    MPEHit::MPEHit() : Hit(PL_MPE, 0, 0){

    }
    MPEHit::MPEHit(u64 launch_time, u8 tdc, u16 nsamples, u8 *waveform) : Hit(PL_MPE, launch_time, tdc){
            this->nsamples = nsamples;
            //this->waveform = new u8(2 * 2 * nsamples); //nsamples, 2 bytes / sample, 2 traces
            memcpy(&this->waveform[0], waveform, 2 * 2 * nsamples);

    }

    MPEHit::~MPEHit(){
        //delete this->waveform;
        free(this);
    }

    //Overload the new operator so that the malloc will handle 
    void* MPEHit::operator new(size_t size, u16 nsamples){
        void* p = ::operator new(size + 2*nsamples * sizeof(u16));     

        return p;
    };

    void MPEHit::print_samples(u16 nsamples){
        u16 n_to_print = nsamples < this->nsamples ? nsamples : this->nsamples;
        for(int ch = 0; ch < 2; ch++){
            print("Ch%02d\r\n\t", ch);
            for(int i = 0; i < n_to_print; i++){
                //Cast array to u16 and go from there. 
                if(i%10 == 0 && i != 0)
                    print("\r\n\t");
                print("%5u ", ((u16*)(this->waveform))[i + ch*this->nsamples]);

            }
            print("\r\n");
        }     
        
    }




    size_t MPEHit::calc_size(){
        //-1 is so we don't double count the first byte. 
        return sizeof(MPEHit) + sizeof(u16) * (2*this->nsamples) -1;
    }

    std::string MPEHit::tostring(){
        char buffer[1024];        
        sprintf(buffer, "MPEHit info:\r\n"
                "\t launch_t:    0x%8X\r\n"
                "\t tdc:         0x%8X\r\n"
                "\t nsamples:    0x%8X\r\n",
                this->launch_time,
                this->tdc,
                this->nsamples
                );
        return std::string(buffer);

    }

    // MPETest::MPETest(u64 launch_time, u8 tdc, u16 nsamples, u8 *waveform) : Hit(PL_MPE, launch_time, tdc){
    //     this->nsamples = nsamples;

    // }

    //WUBuffer constructor/destructor
    WUBuffer::WUBuffer(u16 size, u16 nhits, u8* data){
        this->pl_type = PL_WUBUFF;
        this->size = size;
        this->nhits = nhits;
        this->data = new u8[size];
        memcpy(this->data, data, size);

    }

    WUBuffer::~WUBuffer(){
            delete this->data;
    };

    std::string WUBuffer::tostring(){
        char buffer[1024];
        sprintf(buffer, "WUBuffer info:\r\n"
                "\t size:        0x%8lX\r\n"
                "\t nhits:       0x%8X\r\n",
                this->size,
                this->nhits
                );
        return std::string(buffer);    
    }
