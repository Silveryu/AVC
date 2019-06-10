#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include <algorithm>
#include "binarystream.h"
#include "metadata.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames

void printUsage(void);
void encode(SndfileHandle& sndFileIn, string outfile, short quantsize);
void decode(string infile, string outfile);

int main(int argc, char *argv[]) {

	string encodeArg = argv[1];
	
	bool encodeFlag; 
	if(encodeArg == "-d" || encodeArg == "--decode")
		encodeFlag = false;
	else if(encodeArg == "-e" || encodeArg == "--encode")
		encodeFlag = true;
	else{
		printUsage();
		return 1;
	}

	if(argc < (encodeFlag ? 5 : 4)) {
		printUsage();
		return 1;
	}

	string infile = argv[2];
	string outfile = argv[3];	
	
	if(encodeFlag){	

		SndfileHandle sndFileIn { infile };
		if(sndFileIn.error()) {
			cerr << "Error: invalid input file" << endl;
			return 1;
		}

		if((sndFileIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
			cerr << "Error: file is not in WAV format" << endl;
			return 1;
		}
		
		if((sndFileIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
			cerr << "Error: file is not in PCM_16 format" << endl;
			return 1;
		}

		short quantsize = stoi(argv[4]);
		encode(sndFileIn, outfile, quantsize);
	}
	else{
		decode(infile, outfile);
	}

}

void printUsage(void){
	cerr << "Usage: " 
			<< "\n\twavquant -e|--encode <input file> <output file> <quantsize>"
			<< "\n\twavquant -d|--decode <input file> <output file>" << endl;	
}

void encode(SndfileHandle& sndFileIn, string outfile, short quantsize){

	SF_METADATA.format = sndFileIn.format();
	SF_METADATA.channels = sndFileIn.channels();
	SF_METADATA.samplerate = sndFileIn.samplerate();
	SF_METADATA.quantsize = quantsize;

	cout << "format: " << SF_METADATA.format << endl;
	cout << "channels: "  << SF_METADATA.channels << endl;
	cout << "samplerate: " << SF_METADATA.samplerate << endl;
	cout << "quantsize: " << SF_METADATA.quantsize << endl;

	// write metadata
	BinaryStream ostream { outfile, fstream::out };
	ostream.write((char *)&SF_METADATA, sizeof(SF_METADATA));

	// write data
	size_t nFrames;
	vector<short> samples(FRAMES_BUFFER_SIZE * SF_METADATA.channels);
	while((nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE))){
		samples.resize(nFrames * SF_METADATA.channels);
		for(auto s : samples)
			ostream.writebits(s >> (sizeof(short)*8 - SF_METADATA.quantsize), SF_METADATA.quantsize);
	}
}

void decode(string infile, string outfile){

	// read metadata
	BinaryStream istream {infile, fstream::in};
	istream.read((char *)&SF_METADATA, sizeof(SF_METADATA));

	cout << "format: " << SF_METADATA.format << endl;
	cout << "channels: "  << SF_METADATA.channels << endl;
	cout << "samplerate: " << SF_METADATA.samplerate << endl;
	cout << "quantsize: " << SF_METADATA.quantsize << endl;
	
	// read data
	SndfileHandle sndFileOut { outfile, SFM_WRITE, SF_METADATA.format,
	  SF_METADATA.channels, SF_METADATA.samplerate };
	if(sndFileOut.error()) {
		cerr << "Error: invalid output file" << endl;
		return;
	}

	// logic of reading a compressed file exemplified here
 	size_t nFrames;
	vector<short> samples(FRAMES_BUFFER_SIZE * SF_METADATA.channels);
	while((nFrames = istream.readf(samples.data(), FRAMES_BUFFER_SIZE, SF_METADATA.channels, SF_METADATA.quantsize))){

		// restore values
		std::for_each(samples.begin(), samples.end(), [](short& s) { s = s << (sizeof(short)*8 - SF_METADATA.quantsize);});

		sndFileOut.writef(samples.data(), nFrames);
	}

}


