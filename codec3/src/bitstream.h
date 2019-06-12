#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <iostream>
#include <fstream>
#include <cmath>

struct metadata{};

class BitStream{
  
  private:
	uint8_t  writeidx = 0;
	uint8_t writebuf = 0;
	
	uint8_t readidx = 0;
	uint8_t readbuf = 0;
	
	std::fstream fs;

  public:
	BitStream(std::string filename, std::fstream::openmode mode): 
		fs(filename, mode | std::fstream::binary){}

	
	void writebit(uint8_t val) {
		writebuf = writebuf | (val << writeidx);
		writeidx++;
		if(writeidx == 8){
			fs.write((char *) &writebuf, sizeof(uint8_t));
			writeidx = 0;
			writebuf = 0;
		}
	}

	// bool if still reading 
	bool readbit(uint8_t& val){
		if(fs){
			if(readidx % 8 == 0){
				fs.read((char *)&readbuf, sizeof(uint8_t));
				readidx = 0;
			}
			val = readbuf & 0x01;
			readbuf = readbuf >> 1;
			readidx++;
			return true;
		}
		else{
			return false;
		}
	}

	void writebits(uint64_t val, uint8_t nbits){
		for(short i=0; i < nbits; ++i)
			writebit((val >> (nbits-1-i) ) & 0x01);
	}

	// signed read
	bool readbits(int64_t& val, uint8_t nbits){
		val = 0;
		uint8_t bit;
		bool reading_file = true;
		for(size_t i=0; i< nbits; ++i){
			this->readbit(bit);
			if(fs)
				val = val | ( bit << (nbits-1-i) );
			else{
				reading_file = false;
				break;
			}
		}
		val = utos(val, nbits);

		return reading_file;
	}

	// unsigned read
	bool readbits(uint64_t& val, uint8_t nbits){
		val = 0;
		uint8_t bit;
		bool reading_file = true;
	
			for(size_t i=0; i< nbits; ++i){
				this->readbit(bit);
				if(fs)
					val = val | ( bit << (nbits-1-i) );
				else{
					reading_file = false;
					break;
				}
			}
		return reading_file;
	}

	// unsigned to signed
	int64_t utos(uint64_t uval, short nbits){
		unsigned msb_mask = 1 << (nbits-1);
		return (uval^msb_mask)-msb_mask;
	}


	void writeMetadata(const char* s, std::streamsize n){
		fs.write(s, n);
	}

	bool readMetadata(char* s, std::streamsize n){
		if(fs){
			fs.read(s, n);
			return true;
		}
		else{
			return false;
		}
	}

	// read till '\0' char
	bool readString(std::string& line){
		if(fs){
			getline(fs, line,'\0');
			return true;
		}
		else{
			return false;
		}
	}

	
	
	void close(){
		if (fs.is_open()){
			if(writeidx != 0)
				fs.write((char *) &writebuf, sizeof(uint8_t));
			fs.close();
		}
	}

	void open(std::string filename, std::fstream::openmode mode = std::fstream::out | std::fstream::in){
		fs.open(filename, mode | std::fstream::binary);
	}

};

#endif