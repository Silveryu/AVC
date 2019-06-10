#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include <math.h>
#include <algorithm>
#include "binarystream.h"
#include "lbg.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames


void printUsage(void);
void encode(std::string inFile, std::string outFile, std::vector<std::vector<short>>& codebook, unsigned int codebookSize, unsigned int blockSize);
void decode(std::string vqFile, std::string outFile,  std::vector<std::vector<short>>& codebook, unsigned int codebookSize, unsigned int blockSize);
void readCodebook(std::vector<std::vector<short>>& codebook, unsigned int& codebookSize,  unsigned int&  blockSize, std::string codebookName);

struct{
	unsigned int elSize;
	unsigned int codebookSize;
	int format;
	int channels;
	int samplerate;	
} VQ_METADATA;

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

	if(argc < 3) {
		printUsage();
		return 1;
	}

	string infile = argv[2];
	
	if(encodeFlag){	

		//calculate codebook
		std::vector<std::vector<short>> codebook;
		unsigned int blockSize;
		unsigned int codebookSize;
		std::string outFile = argv[3];
		
		if(argc > 5){
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

			string codebookName = argv[4];
			codebookSize = stoi(argv[5]);
			blockSize = stoi(argv[6]);
			int overlapFactor = stoi(argv[7]);
			double threshold = stod(argv[8]);
			unsigned int max_iter = stoi(argv[9]);

			if(codebookSize <= 0) {
				cerr << "Error: codebookSize must be greater than 0" << endl;
				return 1;
			}

			if(blockSize <= 0) {
				cerr << "Error: blockSize must be greater than 0" << endl;
				return 1;
			}

			if( overlapFactor >= (int)blockSize ) {
				cerr << "Error: blocksize must be greater than overlapFactor" << endl;
				return 1;
			}

			size_t nFrames;
			vector<short> samples(FRAMES_BUFFER_SIZE * sndFileIn.channels());

			LBG lbg {codebookSize, blockSize};

			// doing batch k-means/LBG for now, mini-batch might be more advisable
			while((nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE))) {
				samples.resize(nFrames * sndFileIn.channels());
				lbg.updateTrainingVectors(samples, overlapFactor);
			}
			
			lbg.chooseInitialRandomCentroids();
			lbg.runLBG(threshold, max_iter, false);
			codebook = lbg.getCodebook();
			lbg.writeCodebook(codebookName);

		}
		else{
			std::string codebookName = argv[4];
			readCodebook(codebook, codebookSize, blockSize, codebookName);
		}
		encode( infile, outFile, codebook, codebookSize, blockSize);		

	}
	else{
		if(argc > 5 ){
			printUsage();
			return 1;
		}
		std::string vqFile = argv[2];
		std::string outFile = argv[3];
		std::vector<std::vector<short>> codebook;
		unsigned int blockSize;
		unsigned int codebookSize;
		std::string codebookName = argv[4];
		readCodebook(codebook, codebookSize, blockSize, codebookName);
		decode(vqFile, outFile, codebook, codebookSize, blockSize);
	}

}

void printUsage(void){
	cerr << "Usage: " 
			<< "\n\twavvq -e|--encode <input file> <output file> <codebook>"
			<< "\n\twavvq -e|--encode <input file> <output file> <codebook name> <codebook size> <block size> <overlap factor> <threshold> <max iterations>"
			<< "\n\twavvq -d|--decode <input file> <output file> <codebook>" << endl;	
}

void encode(std::string inFile, std::string outFile, std::vector<std::vector<short>>& codebook, unsigned int codebookSize, unsigned int blockSize){

	SndfileHandle sndFileIn { inFile };
	if(sndFileIn.error()) {
		cerr << "Error: invalid input file" << endl;
		return;
	}

	if((sndFileIn.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
		cerr << "Error: file is not in WAV format" << endl;
		return;
	}
	
	if((sndFileIn.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
		cerr << "Error: file is not in PCM_16 format" << endl;
		return;
	}

	// equivalent to just the assignment step of LBG alg
	LBG lbg {codebook, codebookSize, blockSize};

	size_t nFrames;
	vector<short> samples(FRAMES_BUFFER_SIZE * sndFileIn.channels());
	while((nFrames = sndFileIn.readf(samples.data(), FRAMES_BUFFER_SIZE))) {
		samples.resize(nFrames * sndFileIn.channels());
		lbg.updateTrainingVectors(samples, 0);
	}
	std::vector<unsigned int> clusterAssignments;
	lbg.clusterAssign(clusterAssignments);

	BinaryStream ostream { outFile, std::fstream::out };
	
	//nbits needed to represent indexes
	unsigned int nbits = ceil(log2(codebookSize));

	VQ_METADATA.elSize = nbits;
	VQ_METADATA.codebookSize = codebookSize;
	VQ_METADATA.format = sndFileIn.format();
	VQ_METADATA.channels = sndFileIn.channels();
	VQ_METADATA.samplerate = sndFileIn.samplerate();

	ostream.write((char *)&VQ_METADATA, sizeof(VQ_METADATA));
	for(size_t i=0; i < clusterAssignments.size(); ++i){
		ostream.writebits(clusterAssignments[i], nbits);
	}
} 

void decode(std::string vqFile, std::string outFile,  std::vector<std::vector<short>>& codebook, unsigned int codebookSize, unsigned int blockSize){
	
	BinaryStream istream { vqFile, std::fstream::in};
	istream.read((char *)&VQ_METADATA, sizeof(VQ_METADATA));
	
	// read data
	SndfileHandle sndFileOut { outFile, SFM_WRITE, VQ_METADATA.format,
	  VQ_METADATA.channels, VQ_METADATA.samplerate };
	if(sndFileOut.error()) {
		cerr << "Error: invalid output file" << endl;
		return;
	}

	if(codebookSize != VQ_METADATA.codebookSize){
		cerr << "Error: encoded file doesn't match codebook" << endl;
		return;
	}

	// The number of indices we retrieve
	size_t nIndices = FRAMES_BUFFER_SIZE * VQ_METADATA.channels / blockSize;
	// so we can write full frames
	nIndices += (nIndices*blockSize) % VQ_METADATA.channels;

	vector<unsigned int> indices(nIndices);
	vector<short> samples(nIndices * blockSize);

	while((nIndices = istream.readvq(indices.data(), nIndices, VQ_METADATA.elSize))){

		samples.resize(nIndices * blockSize);
		indices.resize(nIndices);

		for(size_t i = 0; i < nIndices; ++i){
			for(unsigned int j = 0; j < blockSize; ++j){
				samples[i*blockSize + j] = codebook[indices[i]][j];
			}
		}

		size_t nFrames = nIndices * blockSize / VQ_METADATA.channels;
		sndFileOut.writef(samples.data(), nFrames);
	}
}


void readCodebook(std::vector<std::vector<short>>& codebook, unsigned int& codebookSize,  unsigned int&  blockSize, std::string codebookName){
	// read codebookName
	BinaryStream istream {codebookName, std::fstream::in};
	istream.read((char *)&CB_METADATA, sizeof(CB_METADATA));

	codebookSize = CB_METADATA.codebookSize;
	blockSize = CB_METADATA.blockSize;

	codebook = std::vector<std::vector<short>>(CB_METADATA.codebookSize, std::vector<short>(CB_METADATA.blockSize));		
	// read data
	for(unsigned int i=0; i < CB_METADATA.codebookSize; ++i){
		for(unsigned int j=0; j < CB_METADATA.blockSize; ++j){
			codebook[i][j] = istream.readbits(sizeof(short)*8);
		}
	}

}


