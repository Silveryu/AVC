#ifndef GOLOMB_H
#define GOLOMB_H

#include <bitset>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
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

    bool decode(int& signedN){
    
        uint8_t bit;
        unsigned int n = 0;
        
        if(!bs.readbit(bit))
            return false;
        
        while(bit){
            n++;
            if(!bs.readbit(bit))
                return false;
        }
        uint64_t x;

        if(b!=0){
            if(!bs.readbits(x, b-1))
                return false;
        }
        else
            x = 0;

        if(x < t || b == 0)
            n = n*m + x;
        else{
            if(!bs.readbit(bit))
                return false;
            x = 2*x + bit;
            n = n*m + x-t;
        }
        //de-interleave
        signedN = reverse_interleave(n);
        return true;
    }

    bool decodeN(int* ptr, size_t nItems, unsigned int m){
        setM(m);
        for(size_t i=0; i < nItems; i++){
            if(!decode(ptr[i]))
                return false;
        }
        return true;
    }

    bool decodeN(int* ptr, size_t nItems){
        for(size_t i=0; i < nItems; i++){
            if(!decode(ptr[i]))
                return false;
        }
        return true;
    }

    void encodeN(int* ptr, size_t nItems, unsigned int m){
        setM(m);
        for(size_t i=0; i < nItems; i++)
            encode(ptr[i]);
    }

    void encodeN(int* ptr, size_t nItems){
        setM(m);
        for(size_t i=0; i < nItems; i++)
            encode(ptr[i]);
    }

    void writeEncoderMetadata(encoder_metadata& meta){
        bs.writeMetadata((char *)&meta.blockSize, sizeof(meta.blockSize));
        bs.writeMetadata((char *)&meta.searchArea, sizeof(meta.searchArea));
        bs.writeMetadata((char *)&meta.periodicity, sizeof(meta.periodicity));
    }

    void readEncoderMetadata(encoder_metadata& meta){
        bs.readMetadata((char *)&meta.blockSize, sizeof(meta.blockSize));
        bs.readMetadata((char *)&meta.searchArea, sizeof(meta.searchArea));
        bs.readMetadata((char *)&meta.periodicity, sizeof(meta.periodicity));
    }
    
    void writeVideoFileMetadata(video_metadata& meta){
        bs.writeMetadata(meta.header.c_str(), meta.header.length());
        bs.writeMetadata("\0", sizeof(char));
    }

    void readVideoFileMetadata(video_metadata& meta){
        bs.readString(meta.header);
    }

    void writeBlockMetadata(block_metadata& meta){
        bs.writebits(meta.m, sizeof(meta.m)*8);
    }

    void readBlockMetadata(block_metadata& meta){
        uint64_t m;
        bs.readbits(m, sizeof(meta.m)*8);
        meta.m = m;
    }
        
    void writeLossyMetadata(lossy_metadata& meta){
        bs.writeMetadata((char *)&meta.c1_quant, sizeof(meta.c1_quant));
        bs.writeMetadata((char *)&meta.c2_quant, sizeof(meta.c2_quant));
        bs.writeMetadata((char *)&meta.c3_quant, sizeof(meta.c3_quant));
    }

    void readLossyMetadata(lossy_metadata& meta){
        bs.readMetadata((char *)&meta.c1_quant, sizeof(meta.c1_quant));
        bs.readMetadata((char *)&meta.c2_quant, sizeof(meta.c2_quant));
        bs.readMetadata((char *)&meta.c3_quant, sizeof(meta.c3_quant));
    }

    void close(){
		bs.close();
	}

    // get E[X] for residuals
    // parameter estimation of p[X ~ Geometric(p), P(k) = (1-p)^k * p]
    // best m is M = floor(-1/log2(1-p))
    static void estimateM(std::vector<int> residuals, block_metadata& block_meta){        

        size_t residual_sums = 0;
	    for (auto residual : residuals)
		    residual_sums += interleave(residual);
        
        double p =  residuals.size()/(double)residual_sums;
        block_meta.m = floor(-1/log2(1-p));
        if(block_meta.m == 0)
            block_meta.m = 1;
    }

    static void estimateM(std::vector<std::vector<int>> planeResiduals, block_metadata& block_meta){        
        // residuals size can be known beforehand
        size_t residual_sums = 0;
        size_t residual_size = 0;
        for(auto mbResiduals : planeResiduals){
            residual_size += mbResiduals.size();
            for(auto residual : mbResiduals)
                residual_sums += interleave(residual);
        }
        
        double p =  residual_size/(double)residual_sums;
        block_meta.m = floor(-1/log2(1-p));
        if(block_meta.m == 0)
            block_meta.m = 1;
    }


};

#endif