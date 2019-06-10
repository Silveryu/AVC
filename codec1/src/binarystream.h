#ifndef BINARYSTREAM_H
#define BINARYSTREAM_H

#include <iostream>


class BinaryStream{
  
  private:
	short  writeidx = 0;
	uint8_t writebuf = 0;
	short readidx = 0;
	uint8_t readbuf = 0;
	std::fstream fs;

  public:
	BinaryStream(std::string filename, std::fstream::openmode mode): 
		fs(filename, mode | std::fstream::binary){}

	void writebit(uint8_t val) {
		writebuf = writebuf | (val << writeidx);
		writeidx++;
		if(writeidx == 8){
			fs.write((char *) &writebuf, sizeof(int8_t));
			writeidx = 0;
			writebuf = 0;
		}
	}
		// log2(val) < nbits < 64
	void writebits(uint64_t val, short nbits){
		for(short i=0; i < nbits; ++i)
			writebit((val >> (nbits-1-i) ) & 0x01);
	}

	uint8_t readbit(void){
		if(readidx % 8 == 0){
			fs.read((char *)&readbuf, sizeof(int8_t));
			readidx = 0;
		}
		uint8_t val = readbuf & 0x01;
		readbuf = readbuf >> 1;
		readidx++;
		return val;
	}

	// nbits <= 16   
	short readbits(short nbits ,bool convert = true){
		uint64_t val = 0;
		for(short i=0; i<nbits; i++)
			val = val | readbit() << (nbits-1-i);
		return convert ? utos(val, nbits) : val;;
	}

	// unsigned to signed
	short utos(uint64_t uval, short nbits){
		unsigned msb_mask = 1 << (nbits-1);
		return (uval^msb_mask)-msb_mask;
	}
	
	void seekg(std::streampos pos){
		fs.seekg(pos);
	}

	void seekg(std::streamoff off, std::ios_base::seekdir way){
		fs.seekg(off, way);
	}

	std::streampos tellg(){
		return fs.tellg();
	} 

	std::streampos tellp(){
		return fs.tellp();
	} 

	std::ostream& write(const char* s, std::streamsize n){
		return fs.write(s, n);
	}

	std::istream& read(char* s, std::streamsize n){
		return fs.read(s, n);
	}
	
	void close(){
		fs.close();
	}

	void open(std::string filename, std::fstream::openmode mode = std::fstream::out | std::fstream::in){
		fs.open(filename, mode | std::fstream::binary);
	}

	size_t readf(short *ptr, size_t frames, int samplesPerFrame, short nbits){

		// get num of bytes left 
		size_t currpos = fs.tellg();
		fs.seekg(0, std::fstream::end);
		size_t length = (size_t)fs.tellg() - currpos;
		fs.seekg(currpos, std::fstream::beg);

		size_t nFrames;
		size_t nSamples = frames*samplesPerFrame;
		
		if(length*8 > nSamples*nbits){	
			nFrames = frames;
			for(size_t i = 0; i < nSamples; i++)
				ptr[i] = readbits(nbits);		
		}
		else{
			nSamples = length*8 / nbits;
			nFrames = nSamples / samplesPerFrame;
			for(size_t i = 0; i < nSamples; i++)
				ptr[i] = readbits(nbits);
		}
		return nFrames;	
	}


	size_t readvq(unsigned int *ptr, size_t nIndices, short nbits){

		// get num of bytes left 
		size_t currpos = fs.tellg();
		fs.seekg(0, std::fstream::end);
		size_t length = (size_t)fs.tellg() - currpos;
		fs.seekg(currpos, std::fstream::beg);

		size_t nSamples = nIndices;
		
		if(length*8 > nSamples*nbits){	
			for(size_t i = 0; i < nSamples; i++)
				ptr[i] = readbits(nbits, false);		
		}
		else{
			nSamples = length*8 / nbits;			
			for(size_t i = 0; i < nSamples; i++)
				ptr[i] = readbits(nbits, false);
		}
		return nSamples;	
	}

	~BinaryStream(){
		if (fs.is_open()){
			if(writeidx != 0)
				fs.write((char *) &writebuf, sizeof(uint8_t));
			fs.close();
		}
	}
	
};

#endif