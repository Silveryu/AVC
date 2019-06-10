#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include <algorithm>
#include <array>
#include "binarystream.h"
#include "lbg.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames

void printUsage(void);

int main(int argc, char *argv[]) {

	if(argc < 7) {
		printUsage();
		return 1;
	}

	SndfileHandle sndFile { argv[1] };
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

	string codebookName = argv[2];
	unsigned int codebookSize = stoi(argv[3]);
	unsigned int blockSize = stoi(argv[4]);
	int overlapFactor = stoi(argv[5]);
	double threshold = stod(argv[6]);
	unsigned int max_iter = stoi(argv[7]);

	if(codebookSize <= 0) {
		cerr << "Error: codebookSize must be greater than 0" << endl;
		return 1;
	}

	if(blockSize <= 0) {
		cerr << "Error: blockSize must be greater than 0" << endl;
		return 1;
	}

	if( overlapFactor >= (int)blockSize ) {
		cerr << "Error: overlap factor must be greater than blocksize" << endl;
		return 1;
	}

	size_t nFrames;
	vector<short> samples(FRAMES_BUFFER_SIZE * sndFile.channels());

	LBG lbg {codebookSize, blockSize};

	// doing batch k-means/LBG for now, mini-batch might be more advisable
	while((nFrames = sndFile.readf(samples.data(), FRAMES_BUFFER_SIZE))) {
		samples.resize(nFrames * sndFile.channels());
		lbg.updateTrainingVectors(samples, overlapFactor);
	}

	std::vector<double> distortions;
	std::vector<std::vector<short>> codebook; 
	
	lbg.chooseInitialRandomCentroids();
	distortions = lbg.runLBG(threshold, max_iter, false);
	codebook = lbg.getCodebook();
	lbg.writeCodebook(codebookName);

	return 0;
}

void printUsage(void){
	cerr << "Usage:\n\twavcb <input file> <codebook name> <codebook size> <block size> <overlap factor> <threshold> <max iterations>"<< endl;	
}