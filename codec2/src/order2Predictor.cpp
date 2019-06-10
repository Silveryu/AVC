#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include <algorithm>
#include <array>
#include "bitstream.h"
#include "golomb.h"
#include <bitset>
#include <sndfile.hh>

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames

struct
{
	int format;
	int channels;
	int samplerate;        
	short quantsize;
} SF_METADATA;

void printUsage(void);
void encode(SndfileHandle& sndFileIn, string outfile);
void decode(string infile, string outfile);

int main(int argc, char *argv[]) {

	/****** TODO program flow

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
	if(argc < (encodeFlag ? 4 : 3)) {
	        printUsage();
	

	if(encodeFlag){
		encode();
	}else{
		decode();
	}

	*/  

	//read the file 
	string audio_fname = "sample.wav";
	string codec_fname = "testLinPredictOrder2";

	SndfileHandle sndFileIn { audio_fname };

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

	encode(sndFileIn, "binary");
	decode("binary", "predictive.wav");

	return 0;
}


void encode(SndfileHandle& sndFileIn, string outfile){

    SF_METADATA.format = sndFileIn.format();
    SF_METADATA.channels = sndFileIn.channels();
    SF_METADATA.samplerate = sndFileIn.samplerate();

	cout << "format: " << SF_METADATA.format << endl;
	cout << "channels: "  << SF_METADATA.channels << endl;
	cout << "samplerate: " << SF_METADATA.samplerate << endl;

	Golomb gout = Golomb(outfile, std::fstream::out, 2000);    

	size_t nFrames;
	vector<short> samples(FRAMES_BUFFER_SIZE * sndFileIn.channels());
	int cnt = 0;
	// Process by blocks
	while((nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE))){
		samples.resize(nFrames * sndFileIn.channels());
		int estimate, residual;
		size_t n { };
		for(uint i=0; i < samples.size(); i++){
			// draft code, should be optimized
			if(SF_METADATA.channels == 1){ 
				if(i==0){ estimate = 0; }
				else if(i==1){ estimate = samples[i-1]; }
				else{ estimate = 2 * samples[i-1] - samples[i-2]; }	
			}else{
				size_t channel = n++ % SF_METADATA.channels; // channel index
				if((i-channel) == 0){
					estimate = 0;
				}else if((i-channel) == 2){
					estimate = samples[i-2];
				}else{
					estimate = 2 * samples[i-2] - samples[i-4];
				}
			}
			residual = samples[i] - estimate;
			// entropy coding
			gout.encode(residual);
			cnt++;
		}
	}
	
	gout.close();

	cout << "CNT ENCODE: " << cnt << endl;

}

void decode(string infile, string outfile){

	Golomb gin = Golomb(infile, std::fstream::in, 2000);  

	std::fstream fs(infile, std::fstream::in | std::fstream::binary);

/*
	fs.read((char *)&SF_METADATA, sizeof(SF_METADATA));
	cout << "format: " << SF_METADATA.format << endl;
	cout << "channels: "  << SF_METADATA.channels << endl;
	cout << "samplerate: " << SF_METADATA.samplerate << endl;
*/

	// read data
	SndfileHandle sndFileOut { outfile, SFM_WRITE, SF_METADATA.format,
								SF_METADATA.channels, SF_METADATA.samplerate };
	if(sndFileOut.error()) {
		cerr << "Error: invalid output file" << endl;
		return;
	}

	vector<short> residuals(FRAMES_BUFFER_SIZE * SF_METADATA.channels);
	int residual;
	int cnt = 0;
	while(residual = gin.decode()){
		//residuals[i] = residual;
		cnt++;
		//cout << residual << endl;
	}

	cout << "CNT DECODE: " << cnt << endl;

	gin.close();

	/*
	vector<short> residuals(FRAMES_BUFFER_SIZE * SF_METADATA.channels);

	char * buffer = new char [FRAMES_BUFFER_SIZE];

	size_t nFrames;
	while(fs.read(buffer, FRAMES_BUFFER_SIZE)){
		for(int i=0; i < (sizeof(buffer)/sizeof(buffer[0])) ; i++){
			cout << gin.decode() << endl;
		}

		sndFileOut.writef(samples.data(), nFrames);

	}
	*/




	/* DRAFT CODE

	// read metadata	
	std::fstream fs = std::fstream(infile, std::fstream::in | std::fstream::binary) ;
	fs.read((char *)&SF_METADATA, sizeof(SF_METADATA));
	cout << "CHANNELS: " << SF_METADATA.channels << endl;

	// read data
	SndfileHandle sndFileOut { outfile, SFM_WRITE, SF_METADATA.format,
								SF_METADATA.channels, SF_METADATA.samplerate };
	if(sndFileOut.error()) {
		cerr << "Error: invalid output file" << endl;
		return;
	}

	vector<short> residuals(FRAMES_BUFFER_SIZE * SF_METADATA.channels);  
	
	int estimate, sample;

	/*
	while(qqlcoisa){

		residuals.resize(nFrames * SF_METADATA.channels);

		size_t n { };
		for(int i=0, i< qqlcoisa; i++){
			residuals[i] = gin.decode();
			if(channels == 1){
				if(i==0){ estimate = 0; }
				else if(i==1){ estimate = residuals[i-1]; }
				else{ estimate = residuals[i-1] + residuals[i-2]; }	
			}else{
				size_t channel = n++ % channels; // channel index
				if((i-channel) == 0){
					estimate = 0;
				}else if((i-channel) == 2){
					estimate = residuals[i-2];
				}else{
					estimate = residuals[i-2] + residuals[i-4];
				}
			}
		
			sample = residuals[i] + estimate;

			write(sample); // write sample to destination file
		
		}
	}

	gin.close();

	*/

}

void printUsage(void){
        cerr << "Usage: " 
        << "\n\torder2Predictor -e|--encode <input file> <output file> "
        << "\n\torder2Predictor -d|--decode <input file> <output file>" << endl;        
}