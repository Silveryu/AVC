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
void decode(string codec_fname, string audio_fname, vector<int> predictor);

int main(int argc, char *argv[]) {
	
	// predictive decoding process 
	if(argc < 3) {
		cerr << "Usage: losslessPredictiveAudio <input file> <output file>" << endl;
		return 1;
	}

    // temporal predictor, predictor[0] has x[n-(len-1)] 
    vector<int> predictor = TemporalPredictors::temporalOrder2;
	
	string codec_fname = argv[1];
	string audio_fname = argv[2];
	
    decode(codec_fname, audio_fname, predictor);

	return 0;
}

void decode(string codec_fname, string audio_fname, vector<int> predictor){
    soundfile_metadata sf_meta;
	Golomb gin = Golomb(codec_fname, std::fstream::in);
    gin.readMetadata(sf_meta);

    cout << "Input file has:" << endl;
	cout << '\t' << sf_meta.frames  << " frames" << endl;
	cout << '\t' << sf_meta.samplerate << " samples per second" << endl;
	cout << '\t' << sf_meta.channels << " channels" << endl;
	cout << '\t' << sf_meta.m << " golomb paramater" << endl;
    
    gin.setM(sf_meta.m);

	// Process by blocks
	vector<short> samples(FRAMES_BUFFER_SIZE * sf_meta.channels);

    int order = predictor.size();
    //vector<vector<int>> predictorVariables(sf_meta.channels, vector<int>(order, 0)); 
    int index = order - 1;

    SndfileHandle sndFileOut { audio_fname, SFM_WRITE, sf_meta.format, sf_meta.channels, sf_meta.samplerate };

    size_t nFullWrites = sf_meta.frames / FRAMES_BUFFER_SIZE;
    size_t finalWriteFrames = sf_meta.frames % FRAMES_BUFFER_SIZE;
    for (size_t nWrite=0; nWrite <= nFullWrites; nWrite++){ 
        if (nWrite == nFullWrites)    
            samples.resize(finalWriteFrames * sf_meta.channels);

        gin.decodeN(samples.data(), FRAMES_BUFFER_SIZE * sf_meta.channels);
        
        for (size_t i=0; i < samples.size(); i += 2){
            //int channel = i % sf_meta.channels;         
            //int prediction = std::inner_product(predictor.begin(), predictor.end(), predictorVariables[channel].begin(), 0.0);
            int R = 0, L = 0;
            if (( samples[i+1] % 2) != 0){
                R = ( (2 * samples[i]) + 1 - samples[i+1] ) / 2;
                L = samples[i+1] + R;
            }
            else{
                R = ( samples[i] - samples[i+1] ) / 2;
                L = samples[i + 1] + R;
            }
            
            samples[i] = L;
            samples[i + 1] = R;
            //predictorVariables[channel][index] = samples[i];
            //if (channel == sf_meta.channels-1)
				//index = (index+1) % sf_meta.channels;
        }
        if (nWrite == nFullWrites)    
            sndFileOut.writef(samples.data(), finalWriteFrames);
        else
            sndFileOut.writef(samples.data(), FRAMES_BUFFER_SIZE);
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
