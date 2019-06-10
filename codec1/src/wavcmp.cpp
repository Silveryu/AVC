#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include <algorithm>
#include "binarystream.h"
#include "metadata.h"
#include <math.h>
#include <numeric>
#include <limits>

using namespace std;


constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames

void printUsage(void);
void computeSNR(SndfileHandle&, SndfileHandle&);

int main(int argc, char *argv[]) {
	if (argc < 3){
		printUsage();
		return -1;
	}

	string original_file = argv[1];
	string noise_file = argv[2];
	
	SndfileHandle sndFileOriginal { original_file };
	SndfileHandle sndFileNoise { noise_file };

	if(sndFileOriginal.error() || sndFileNoise.error()) {
		cerr << "Error: invalid input file" << endl;
		return 1;
	}

	if((sndFileOriginal.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
		cerr << "Error: file is not in WAV format" << endl;
		return 1;
	}
	
	if((sndFileOriginal.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
		cerr << "Error: file is not in PCM_16 format" << endl;
		return 1;
	}

	computeSNR(sndFileOriginal, sndFileNoise);

}

void printUsage(void){
	cerr << "Usage: " 
		 << "\n\twavcmp <original file> <noise file>" << endl;	
}

void computeSNR(SndfileHandle& sndFileIn, SndfileHandle& sndFileNoise){

	SF_METADATA.format = sndFileIn.format();
	SF_METADATA.channels = sndFileIn.channels();
	SF_METADATA.samplerate = sndFileIn.samplerate();

	cout << "Original format: " << SF_METADATA.format << endl;
	cout << "Original channels: "  << SF_METADATA.channels << endl;
	cout << "Original samplerate: " << SF_METADATA.samplerate << endl;

	SF_METADATA.format = sndFileNoise.format();
	SF_METADATA.channels = sndFileNoise.channels();
	SF_METADATA.samplerate = sndFileNoise.samplerate();

	cout << "Noise format: " << SF_METADATA.format << endl;
	cout << "Noise channels: "  << SF_METADATA.channels << endl;
	cout << "Noise samplerate: " << SF_METADATA.samplerate << endl;

	// write data
	size_t nFrames, nFramesNoise;

	vector<short> samples(FRAMES_BUFFER_SIZE * SF_METADATA.channels);
	vector<short> samplesNoise(FRAMES_BUFFER_SIZE * SF_METADATA.channels);

	vector<double> sum_signal(SF_METADATA.channels);
	vector<double> sum_error(SF_METADATA.channels);
	double currSnr;
	double psnr = 0;

	double error;
	double max_sample_error = 0;

	while( (nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE)) && (nFramesNoise = sndFileNoise.readf(samplesNoise.data(), FRAMES_BUFFER_SIZE)) ){		
		
		samples.resize(nFrames * SF_METADATA.channels);
		samplesNoise.resize(nFramesNoise * SF_METADATA.channels);

		size_t n { };
		for(unsigned int i=0; i<samples.size(); i++){ // calculate the sum of the powers of signal and error parts
			size_t ch_idx = n++ % SF_METADATA.channels; // channel index
			sum_signal[ch_idx] += pow ( samples[i] , 2); // signal sum
			sum_error[ch_idx]  += pow ( samples[i] - samplesNoise[i], 2); // error sum
			currSnr = 10*log10(sum_signal[ch_idx] / sum_error[ch_idx]);
			if(currSnr > psnr)
				psnr = currSnr;
			error = samples[i] - samplesNoise[i];
			if(error > max_sample_error)
				max_sample_error = error;
			
		}
	}

	double snr = 0;
	for(int i=0;i<SF_METADATA.channels;i++){
		snr = 10 * log10 (sum_signal[i] / sum_error[i]);
		cout << "SNR channel " << i << ": " << snr << endl;
	} 
	cout << "PSNR: " <<  psnr << endl;
	cout << "Max sample error: " << max_sample_error << endl;


}