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
#include "predictors.h"


using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames

bool sndVerifications(SndfileHandle& sndFile);
bool estimateM( string audio_fname, vector<int> predictor, soundfile_metadata& sf_meta);
void encode(string audio_fname, string codec_fname, vector<int> predictor, soundfile_metadata meta);

int main(int argc, char *argv[]) {
	
	// predictive coding process
	if(argc < 3) {
		cerr << "Usage: losslessPredictiveAudio <input file> <output file>" << endl;
		return 1;
	}
	
	string audio_fname = argv[1];
	string codec_fname = argv[2];
	
	vector<int> predictor = TemporalPredictors::temporalOrder1;

	soundfile_metadata sf_meta;
	// estimates M and fills metadata
	if(!estimateM(audio_fname, predictor, sf_meta))
		return 1;
	
	cout << "Input file has:" << endl;
	cout << '\t' << sf_meta.frames  << " frames" << endl;
	cout << '\t' << sf_meta.samplerate << " samples per second" << endl;
	cout << '\t' << sf_meta.channels << " channels" << endl;
	cout << "Estimated m:" << endl;
	cout << '\t' << sf_meta.m << " golomb parameter" << endl;

	encode(audio_fname, codec_fname, predictor, sf_meta);

	return 0;
}

void encode(string audio_fname, string codec_fname, vector<int> predictor, soundfile_metadata sf_meta){
	SndfileHandle sndFileIn { audio_fname };
	
	if(sndVerifications(sndFileIn)){
		return;
	}
	// Process by blocks
	size_t nFrames;
	vector<short> samples(FRAMES_BUFFER_SIZE * sndFileIn.channels());
	
	int order = predictor.size();
	//vector<vector<int>> predictorVariables(sf_meta.channels,vector<int>(order, 0));
	int index = order-1;

	Golomb gout = Golomb(codec_fname, std::fstream::out, sf_meta.m);
	gout.writeMetadata(sf_meta);

	// write golomb encoded residuals
	int nWrite = 0;
	while ((nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE))){
		samples.resize(nFrames * sndFileIn.channels());
		for (size_t i = 0; i < samples.size(); i += 2){
			
			//short sample = samples[i];
			int channel = i % sf_meta.channels;
			
			// simple predictor 
			int Y = samples[i] - samples[i + 1];
			//std::inner_product(predictor.begin(), predictor.end(), predictorVariables[channel].begin(), 0.0);
			int X = (samples[i] + samples[ i + 1] )/ 2;
			
			if (nWrite == 0 && i < 100)
				cout << "encoding " << i << " -> " << X-Y  << endl;
			
			gout.encode(X);
			gout.encode(Y);
			
			if (channel == sf_meta.channels-1)
				index = (index+1) % sf_meta.channels;
		}
		nWrite++;
	}
	gout.close();
}

bool estimateM(string audio_fname, vector<int> predictor, soundfile_metadata& sf_meta){
	SndfileHandle sndFileIn { audio_fname };
	
	if(sndVerifications(sndFileIn)){
		return false;
	}

	sf_meta.format = sndFileIn.format();
	sf_meta.frames = sndFileIn.frames();
	sf_meta.samplerate = sndFileIn.samplerate();
	sf_meta.channels = sndFileIn.channels();

	// Process by blocks
	size_t nFrames;
	
	vector<short> samples(FRAMES_BUFFER_SIZE * sndFileIn.channels());

	int order = predictor.size();
	// find m for linear predictor 
	// buffer needed for predictions
	vector<vector<int>> predictorVariables( sf_meta.channels, vector<int>(order, 0)); 
	//vector<size_t> residual_sums(sf_meta.channels);
	size_t residual_sums = 0;
	int index = order-1;

	// get E[X] for residuals
	// parameter estimation of p [X ~ Geometric(p), P(k) = (1-p)^k * p]
	// best m is M = floor(-1/log2(1-p))
	// estimate best m and find fileSize
	// probably not best way to estimate m 
	while ((nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE))){
		samples.resize(nFrames * sndFileIn.channels());
		
		for(size_t i=0; i < samples.size(); i++){
			short sample = samples[i];
			int channel = i % sf_meta.channels;
			
			int prediction = std::inner_product(predictor.begin(), predictor.end(), predictorVariables[channel].begin(), 0.0);
			residual_sums += Golomb::interleave(sample - prediction);

			// update predictor variables
			predictorVariables[channel][index] = sample; 
			if(channel == sf_meta.channels-1)
				index = (index+1) % sf_meta.channels;
		}
	}

	int nSamples = sf_meta.frames * sf_meta.channels;
	double p =  nSamples/(double)residual_sums;
	sf_meta.m = floor(-1/log2(1-p));

	return true;
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

