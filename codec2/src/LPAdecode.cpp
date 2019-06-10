#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sndfile.hh>
#include <vector>
#include "bitstream.h"
#include "golomb.h"
#include "metadata.h"
#include "predictors.h"


using namespace std;

bool sndVerifications(SndfileHandle& sndFile);
void decode(string codec_fname, string audio_fname);

int main(int argc, char *argv[]) {
	
	// predictive decoding process 
	if(argc < 3) {
		cerr << "Usage: LPAdecode <input file> <output file>" << endl;
		return 1;
	}

	string codec_fname = argv[1];
	string audio_fname = argv[2];
	
	decode(codec_fname, audio_fname);

	return 0;
}

void decode(string codec_fname, string audio_fname){
	
	soundfile_metadata sf_meta;
	Golomb gin = Golomb(codec_fname, std::fstream::in);
	gin.readSoundFileMetadata(sf_meta);

	Predictor predictor { sf_meta.type };
	bool lossy = (sf_meta.quantsize != 0); 

	cout << "Input file has:" << endl;
	cout << '\t' << sf_meta.frames  << " frames" << endl;
	cout << '\t' << sf_meta.samplerate << " samples per second" << endl;
	cout << '\t' << sf_meta.channels << " channels" << endl;
	
	cout << "Encoded with:" << endl;
	if(lossy){
		cout << '\t' << "Lossy compression" << endl; 
		cout << '\t' << sf_meta.quantsize << " quantsize" << endl;
	}
	else{
		cout << '\t' << "Lossless compression" << endl; 
		cout << '\t' << predictor.name << " predictor" << endl;
	}
	// Process by blocks
	vector<short> samples(sf_meta.FRAMES_BUFFER_SIZE * sf_meta.channels);
	vector<int> residuals(sf_meta.FRAMES_BUFFER_SIZE * sf_meta.channels);

	int order = predictor.kernel.size();
	vector<vector<int>> predictorVariables(sf_meta.channels, vector<int>(order, 0));

	SndfileHandle sndFileOut { audio_fname, SFM_WRITE, sf_meta.format, sf_meta.channels, sf_meta.samplerate };

	if(sndVerifications(sndFileOut))
		return;

	size_t nFullWrites = sf_meta.frames / sf_meta.FRAMES_BUFFER_SIZE;
	size_t finalWriteFrames = sf_meta.frames % sf_meta.FRAMES_BUFFER_SIZE;

	for(size_t nWrite=0; nWrite <= nFullWrites; nWrite++){
		
		block_metadata b_meta;

		gin.readBlockMetadata(b_meta);
		
		if(nWrite == nFullWrites){
			samples.resize(finalWriteFrames * sf_meta.channels);
			residuals.resize(finalWriteFrames * sf_meta.channels);
		}

		gin.decodeN(residuals.data(), residuals.size(), b_meta.m);

		// decoding process
		if(predictor.isTemporal)
			for(size_t i=0; i < residuals.size(); i++){
				
				int channel = i % sf_meta.channels;
				int prediction = std::inner_product(predictor.kernel.begin(), predictor.kernel.end(), predictorVariables[channel].begin(), 0.0);

				if(lossy)
					samples[i] = (residuals[i] << (sizeof(short)*8-sf_meta.quantsize)) + prediction;
				else
					samples[i] = residuals[i] + prediction;
				
				// add to prediction buffer
				std::rotate(predictorVariables[channel].begin(), predictorVariables[channel].begin()+1, predictorVariables[channel].end()); 
				predictorVariables[channel][order-1] = samples[i];
			}
		else{
			for (size_t i=0; i < residuals.size(); i += 2){
	
				int X, Y, LR;
				X = residuals[i];
				Y = residuals[i+1];

				if(lossy){
					X = X << ( sizeof(short)*8-sf_meta.quantsize );
					Y = Y << ( sizeof(short)*8-sf_meta.quantsize );
				}

				if(Y % 2 != 0)
					LR = 2*X+1;
				else
					LR = 2*X;
				
				samples[i+1] = (LR - Y)/2;
				samples[i] = Y + samples[i+1];
        	}
		}

		sndFileOut.writef(samples.data(), (nWrite == nFullWrites) ? finalWriteFrames : sf_meta.FRAMES_BUFFER_SIZE);
	}
	gin.close();
}


bool sndVerifications(SndfileHandle& sndFile){
	if(sndFile.error()) {
		cerr << "Error: invalid input file" << endl;
		return 1;
	}
	if((sndFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
		cerr << "Error: file is not in WAV format" << endl;
		return 1;
	}
	if((sndFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
		cerr << "Error: file is not in PCM_16 format" << endl;
		return 1;
	}

	return 0;
} 
