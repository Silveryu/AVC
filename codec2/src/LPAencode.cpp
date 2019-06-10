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
#include "metadata.h"
#include "wavhist.h"



using namespace std;

bool sndVerifications(SndfileHandle& sndFile);
block_metadata estimateM(vector<int> residuals);
void encode(string audio_fname, string codec_fname, Predictor predictor, bool lossy, unsigned int quantsize);
void printUsage();

int main(int argc, char *argv[]) {
	
	// predictive coding process
	if(argc < 5) {
		printUsage();
		return 1;
	}
	
	bool lossy;
	unsigned int quantsize;

	string opt = argv[1];
	if(opt=="--lossless")
		lossy = false;
	else if(opt=="--lossy" && argc > 5){
		lossy = true;
	}
	else
		return 1;
	

	Predictor predictor { PredictorType(stoi(argv[2])) };

	string audio_fname = argv[3];
	string codec_fname = argv[4];

	if(lossy){
		quantsize = stoi(argv[5]);
		if(quantsize <= 0 || quantsize >= 16){
			cerr << "quantsize must be in interval [1,15]" << endl;
			return 1;
		}
	}
	else{
		quantsize = 0;
	}
	
	encode(audio_fname, codec_fname, predictor, lossy, quantsize);

	return 0;
}

void encode(string audio_fname, string codec_fname, Predictor predictor, bool lossy, unsigned int quantsize){
	SndfileHandle sndFileIn { audio_fname };
	if(sndVerifications(sndFileIn)){
		return;
	}

	soundfile_metadata sf_meta;
	sf_meta.type = predictor.type;

	// read metadata
	sf_meta.format = sndFileIn.format();
	sf_meta.frames = sndFileIn.frames();
	sf_meta.samplerate = sndFileIn.samplerate();
	sf_meta.channels = sndFileIn.channels();

	cout << "Input file has:" << endl;
	cout << '\t' << sf_meta.frames  << " frames" << endl;
	cout << '\t' << sf_meta.samplerate << " samples per second" << endl;
	cout << '\t' << sf_meta.channels << " channels" << endl;
	cout << "Using: " << endl;
	cout << '\t' << sf_meta.FRAMES_BUFFER_SIZE << " buffer size" << endl;

	// output histogram
	WAVHist hist { sf_meta.channels };
	
	if(lossy){
		sf_meta.quantsize = quantsize;
		cout << '\t' << "Lossy compression with " << sf_meta.quantsize << " quantsize" << endl;
	}
	else{
		cout << '\t' << "Lossless compression" << endl;
		sf_meta.quantsize = 0;
	}
	cout << '\t' << predictor.name << " predictor" << endl;
	// Process by blocks
	size_t nFrames;
	vector<short> samples(sf_meta.FRAMES_BUFFER_SIZE * sf_meta.channels);
	vector<int> residuals(sf_meta.FRAMES_BUFFER_SIZE * sf_meta.channels);

	int order = predictor.kernel.size();
	vector<vector<int>> predictorVariables(sf_meta.channels, vector<int>(order, 0));

	Golomb gout = Golomb(codec_fname, std::fstream::out);
	gout.writeSoundFileMetadata(sf_meta);

	// write golomb encoded residuals
	int nWrite = 0;
	while((nFrames = sndFileIn.readf(samples.data(), sf_meta.FRAMES_BUFFER_SIZE))){
		samples.resize(nFrames * sf_meta.channels);
		residuals.resize(nFrames * sf_meta.channels);

		// calculate residuals
		if(predictor.isTemporal)
			for(size_t i=0; i < samples.size(); i++){
				
				int channel = i % sf_meta.channels;
				int prediction = std::inner_product(predictor.kernel.begin(), predictor.kernel.end(), predictorVariables[channel].begin(), 0.0);

				if(lossy){
					residuals[i] = samples[i] - prediction;
					residuals[i] = residuals[i] >> ( sizeof(short)*8-sf_meta.quantsize );
					// add to prediction buffer
					std::rotate(predictorVariables[channel].begin(), predictorVariables[channel].begin()+1, predictorVariables[channel].end()); 
					predictorVariables[channel][order-1] = ( residuals[i] << ( sizeof(short)*8-sf_meta.quantsize ) ) + prediction;
				}
				else{				
					// add to prediction buffer
					std::rotate(predictorVariables[channel].begin(), predictorVariables[channel].begin()+1, predictorVariables[channel].end()); 
					predictorVariables[channel][order-1] = samples[i];	
					residuals[i] = samples[i] - prediction;
				}
			}
		else{
			for (size_t i=0; i < samples.size(); i += 2){
	
				int X, Y;

				X = (samples[i] + samples[i+1])/2;

				Y = samples[i] - samples[i+1];

				if(lossy){
					X = X >> ( sizeof(short)*8-sf_meta.quantsize );
					Y = Y >> ( sizeof(short)*8-sf_meta.quantsize );
				}

				residuals[i] = X;
				residuals[i+1] = Y;
        	}
		}

		hist.update(residuals);
		block_metadata b_meta = estimateM(residuals);
		gout.writeBlockMetadata(b_meta);
		gout.encodeN(residuals.data(), residuals.size(), b_meta.m);
		nWrite++;
	}
	// mono by default

	const int channel = -1;
	hist.dump(channel, audio_fname + "_" + predictor.name + "_" + ( (channel == -1) ? "mono" : "channel"+to_string(channel)));
	gout.close();

}
block_metadata estimateM(vector<int> residuals){	
	block_metadata b_meta;
	size_t residual_sums = 0;
	for(size_t i = 0; i < residuals.size(); ++i){
		residual_sums+= Golomb::interleave(residuals[i]);
	}
	int nSamples = residuals.size();
	double p =  nSamples/(double)residual_sums;

	b_meta.m = ceil(-1/log2(1-p));

	// safeguard
	if(b_meta.m == 0)
		b_meta.m = 1;
	return b_meta;
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

void printUsage(){
	cerr << "Usage:" << endl;
	cerr << '\t' << "LPAencode --lossless <predictor type> <input file> <output file> " << endl;
	cerr << '\t' << "LPAencode --lossy <predictor type> <input file> <output file> <quantsize>" << endl;
	cerr << "Predictor types:" << endl;
	cerr << '\t' << "0 - InterChannel" << endl;
	cerr << '\t' << "1 - TemporalOrder1" << endl;
	cerr << '\t' << "2 - TemporalOrder2" << endl;
	cerr << '\t' << "3 - TemporalOrder3" << endl;
}