#ifndef GOLOMB_H
#define GOLOMB_H

#include <bitset>
#include <iostream>
#include <fstream>
#include <vector>
#include "bitstream.h"
#include "metadata.h"

class Golomb{
  
  private:

    BitStream bs;
    size_t m;
    size_t b;
    bool isMPow2;  
    unsigned int t;


  public:
    
    Golomb(std::string filename, std::fstream::openmode mode, size_t m): 
        bs(filename, mode | std::fstream::binary), m(m){
        this->b = ceil(log2(m));
        this->isMPow2 = !(m & (m - 1));
        this->t = pow(2, b) - m;
        
        std::cout << "Golomb parameters: " << std::endl;
        std::cout << "\t" << "b: " << b << std::endl;
        std::cout << "\t" << "m: " << m << std::endl;
        std::cout << "\t" << "t: " << t << std::endl;
    }

    Golomb(std::string filename, std::fstream::openmode mode): 
        bs(filename, mode | std::fstream::binary){
    }

    static unsigned int interleave(int signedN){
        return (signedN<0) ? -2*signedN-1 : 2*signedN; 
    }
    static int reverse_interleave(unsigned int n){
        return (n%2!=0) ? ((int)n+1)/-2 : n/2;
    }

    void setM(size_t m){
        this->m = m;
        this->b = ceil(log2(m));
        this->isMPow2 = !(m & (m - 1));
        this->t = pow(2, b) - m;
    }

    void encode(int signedN){    
        // interleave
        unsigned int n = interleave(signedN);
        // unary code
        unsigned int q = int(n/m);
        // binary code
        unsigned int r = n % m;
        // write q-length string of 1 bits + a 0 bit
        for(size_t i = 0; i < q; ++i)
            bs.writebit(1);
        bs.writebit(0);
        if( isMPow2 )
            bs.writebits(r, b);      
        else{
            if(r < t)
                bs.writebits(r, b-1);
            else
                bs.writebits(r+t, b);
        }
    }

    int decode(){
    
        uint8_t bit;
        unsigned int n = 0;
        bs.readbit(bit);
        while(bit){
            n++;
            bs.readbit(bit);
        }
        uint64_t x;

        if(b!=0)
            bs.readbits(x, b-1);
        else
            x = 0;

        if(x < t || b == 0)
            n = n*m + x;
        else{
            bs.readbit(bit);
            x = 2*x + bit;
            n = n*m + x-t;
        }
        //de-interleave
        int signedN = reverse_interleave(n);
        return signedN;
    }

    void decodeN(int* ptr, size_t nItems, unsigned int m){
        setM(m);
        for(size_t i=0; i < nItems; i++)
            ptr[i] = decode();
    }

    void encodeN(int* ptr, size_t nItems, unsigned int m){
        setM(m);
        for(size_t i=0; i < nItems; i++)
            encode(ptr[i]);
    }

    void writeSoundFileMetadata(soundfile_metadata& meta){
        bs.writeMetadata((char *)&meta.format, sizeof(meta.format));
        bs.writeMetadata((char *)&meta.frames, sizeof(meta.frames));
        bs.writeMetadata((char *)&meta.channels, sizeof(meta.channels));
        bs.writeMetadata((char *)&meta.samplerate, sizeof(meta.samplerate));
        bs.writeMetadata((char *)&meta.quantsize, sizeof(meta.quantsize));
        bs.writeMetadata((char *)&meta.FRAMES_BUFFER_SIZE, sizeof(meta.FRAMES_BUFFER_SIZE));
        bs.writeMetadata((char *)&meta.type, sizeof(meta.type));
    }

    void readSoundFileMetadata(soundfile_metadata& meta){
        bs.readMetadata((char *)&meta.format, sizeof(meta.format));
        bs.readMetadata((char *)&meta.frames, sizeof(meta.frames));
        bs.readMetadata((char *)&meta.channels, sizeof(meta.channels));
        bs.readMetadata((char *)&meta.samplerate, sizeof(meta.samplerate));
        bs.readMetadata((char *)&meta.quantsize, sizeof(meta.quantsize));
        bs.readMetadata((char *)&meta.FRAMES_BUFFER_SIZE, sizeof(meta.FRAMES_BUFFER_SIZE));
        bs.readMetadata((char *)&meta.type, sizeof(meta.type));
    }

    void writeBlockMetadata(block_metadata& meta){
        bs.writebits(meta.m, sizeof(meta.m)*8);
    }

    void readBlockMetadata(block_metadata& meta){
        uint64_t m;
        bs.readbits(m, sizeof(meta.m)*8);
        meta.m = m;
    }

    void close(){
		bs.close();
	}
};

#endif