#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
#include "FrameParser.h"
#include "golomb.h"

using namespace cv;
using namespace std;

void encode(string video_file, string encoded_file, int blockSize, int searchArea, int periodicity);
void decode(string encoded_file, string video_file);
void printUsage();

int main(int argc, char** argv)
{

	/* check for the mandatory arguments */
	if( argc < 3 ) {
		printUsage();
		return 1;
	}

	string opt = argv[1];
	if(opt=="-e" || opt=="--encode"){
		string video_file = argv[2];
		string encoded_file = argv[3];
		int blockSize;
		int searchArea;
		int periodicity;

		if(argc < 5){
			printUsage();
			return 1;
		}
		
		try {
			blockSize = stoi(argv[4]);
			searchArea = stoi(argv[5]);
			periodicity = stoi(argv[6]);
		}
		catch (std::invalid_argument& e) {
			printUsage();
			return 1;
		}
		encode(video_file, encoded_file, blockSize, searchArea, periodicity);
	}
	else if(opt=="-d" || opt=="--decode"){
		string encoded_file = argv[2];
		string video_file = argv[3];
		decode(encoded_file, video_file);
	}
	return 0;
}

void printUsage(){
	cerr << "Usage:" 
		 << "\n\t" << "lhe -e|--encode <video_file> <encoded_file> <block_size> <search_area> <periodicity>" 
		 << "\n\t" << "lhe -d|--decode <encoded_file> <video_file>" 
		 << endl;
}

void encode(string video_file, string encoded_file, int blockSize, int searchArea, int periodicity){
	
	string line; // store the header
	int yCols, yRows; /* frame dimension */
	int ySize;
	int uvCols;
	int uvSize;
	int frameSize;
	unsigned char *imgData; // file data buffer
	string format = "420";
	Mat yImg, uImg, vImg, img;
	video_metadata video_meta;
	encoder_metadata enc_meta;
	block_metadata block_meta_I;
	block_metadata block_meta_P;
	bool estimatedM = false;
	
	/* Opening video file */
	ifstream myfile (video_file);

	/* Processing header */
	getline (myfile,line);
	FrameParser parser {line};

	yCols = parser.yCols;
	yRows = parser.yRows;
	ySize = parser.ySize;

	uvCols = parser.uvCols;
	uvSize = parser.uvSize;

	format = parser.format;
	frameSize = parser.frameSize;

	parser.setPredictiveParams(blockSize, searchArea);
	cout << "block size: " << blockSize << endl;
	cout << "search area: " << searchArea << endl;
	cout << "periodicity: " << periodicity << endl;

	/* buffer to store the frame */
	imgData = new unsigned char[frameSize];
	video_meta.header = parser.header;
	enc_meta.blockSize = blockSize;
	enc_meta.searchArea = searchArea;
	enc_meta.periodicity = periodicity;
	
	Golomb gout = Golomb(encoded_file, std::fstream::out);
	gout.writeVideoFileMetadata(video_meta);
	gout.writeEncoderMetadata(enc_meta);

	// y, u, v planar frames
	array<Mat, 3> IFrame;
	enum class FrameType {Intra, Predictive};
	FrameType frameType = FrameType::Intra; 
    // for all frames
	for(int i = 0;;){
        /* load a new frame, if possible */
		getline (myfile,line); // Skipping word FRAME
		myfile.read((char *)imgData, frameSize);
		if(myfile.gcount() == 0)
		{
			myfile.clear();
			myfile.seekg(0);
			getline (myfile,line); // read the header
			break;
		}

		// read planar info
		// get individual plane for each channel
		parser.getPlanarChannels(imgData, yImg, uImg, vImg);

		if(frameType == FrameType::Intra){
			IFrame.at(0) = yImg;
			IFrame.at(1) = uImg;
			IFrame.at(2) = vImg;
			
			vector<int> residuals;
			residuals.reserve(frameSize);

			// get residuals of each plane
			parser.encodeLS(residuals, yImg, ySize, yCols);
			parser.encodeLS(residuals, uImg, uvSize, uvCols);
			parser.encodeLS(residuals, vImg, uvSize, uvCols);

			// one m for whole video
			if(!estimatedM){
				Golomb::estimateM(residuals, block_meta_I);
				gout.writeBlockMetadata(block_meta_I);
				cout << "Golomb parameter: "<< block_meta_I.m << endl;
				estimatedM = true;
			}
			gout.encodeN(residuals.data(), residuals.size(), block_meta_I.m);
		}
		else if(frameType == FrameType::Predictive){
			
			int mbTotal = yCols * yRows / (blockSize*blockSize);
			
			for(int channel = 0; channel <3; channel++){
				vector<vector<int>> planeResiduals;
				vector<Point2i> planeVectors;
				
				Mat Pimg;
				if(channel == 0)
					Pimg = yImg;
				else if(channel == 1)
					Pimg = uImg;
				else
					Pimg = vImg;
				
				for(int mbNum = 0; mbNum < mbTotal; mbNum++){
					Mat residuals;
					Point2i motionVectors; 
					
					parser.motionCompensationEncodeSingleChannel(IFrame.at(channel), Pimg, mbNum, channel, residuals, motionVectors);
					vector<int> mbResiduals;
					
					FrameParser::mat2vec(residuals, mbResiduals);
					planeResiduals.push_back(mbResiduals);
					planeVectors.push_back(motionVectors);
				}

				Golomb::estimateM(planeResiduals, block_meta_P);
				gout.setM(block_meta_P.m);
				gout.writeBlockMetadata(block_meta_P);
				for(int mbNum = 0; mbNum < mbTotal; mbNum ++ ){
						Point2i vec = planeVectors.at(mbNum);
						gout.encode(vec.x);
						gout.encode(vec.y);
						vector<int> res = planeResiduals.at(mbNum);
						gout.encodeN(res.data(), res.size());	
				}
			}
		}
		if(i != periodicity){
			frameType = FrameType::Predictive;
			i++;
		}
		else{
			frameType = FrameType::Intra;
			i = 0;
		}
	}
}

void decode(string encoded_file, string video_file){

	string line; // store the header
	int yCols, yRows; /* frame dimension */
	int ySize;
	int uvCols;
	int uvSize;
	int frameSize;
	int *residuals; // file data buffer
	int blockSize;
	int periodicity;
	int searchArea;
	Mat yImg, uImg, vImg;
	video_metadata video_meta;
	block_metadata block_meta_I;
	block_metadata block_meta_P;
	encoder_metadata enc_meta;
	bool readM = false;
	uchar *buffer; // unsigned char pointer to the Mat data
	string format;

	Golomb gin = Golomb(encoded_file, std::fstream::in);
	gin.readVideoFileMetadata(video_meta);


	FrameParser parser { video_meta.header };
	yCols = parser.yCols;
	yRows = parser.yRows;
	ySize = parser.ySize;
	uvCols = parser.uvCols;
	uvSize = parser.uvSize;
	format = parser.format;
	frameSize = parser.frameSize;

	gin.readEncoderMetadata(enc_meta);
	blockSize = enc_meta.blockSize;
	searchArea = enc_meta.searchArea;
	periodicity = enc_meta.periodicity;
	parser.setPredictiveParams(blockSize, searchArea);
	cout << "block size: " << blockSize << endl;
	cout << "search area: " << searchArea << endl;
	cout << "periodicity: " << periodicity << endl;
	
	ofstream vout;
	vout.open(video_file, std::ofstream::out);
	vout << video_meta.header << "\n";

	array<Mat, 3> IFrame;
	enum class FrameType {Intra, Predictive};
	FrameType frameType = FrameType::Intra; 
	residuals = new int[frameSize];
	for(int i=0;;){
		if(frameType == FrameType::Intra){
			// one m for whole video
			if(!readM){
				gin.readBlockMetadata(block_meta_I);
				cout << "Golomb parameter: "<< block_meta_I.m << endl;
				readM = true;
			}

			if(!gin.decodeN(residuals, frameSize, block_meta_I.m))
				break;

			parser.getPlanarResiduals(residuals, yImg, uImg, vImg);

			vector<unsigned char> original;
			original.reserve(frameSize);
			
			parser.decodeLS(original, yImg, ySize, yCols);
			parser.decodeLS(original, uImg, uvSize, uvCols);
			parser.decodeLS(original, vImg, uvSize, uvCols);

			vout << "FRAME\n";
			for (int i = 0; i < frameSize; i++){
				vout << original.at(i);
			}
			// save for later prediction
			parser.getPlanarChannels(original.data(), IFrame.at(0), IFrame.at(1), IFrame.at(2));
		}
		else if(frameType == FrameType::Predictive){
			
			int mbTotal = yCols * yRows / (blockSize*blockSize);
			int planeSize[3] = {ySize, uvSize, uvSize};
			vout << "FRAME\n";
			for(int channel = 0; channel <3; channel++){
				gin.readBlockMetadata(block_meta_P);
				gin.setM(block_meta_P.m);
				Point2i vec;
				Mat original;
				parser.initPlanarMat(original, channel);
				int channelBlockTotal = parser.channelBlockSize(channel); 

				for(int mbNum = 0; mbNum < mbTotal; mbNum++){
					gin.decode(vec.x);
					gin.decode(vec.y);
					vector<int> res(channelBlockTotal);
					if(!gin.decodeN(res.data(), channelBlockTotal, block_meta_P.m))
						break;
					Mat RImg;
					parser.vec2channelMat(res, channel, RImg);
					parser.motionCompensationDecodeSingleChannel(IFrame.at(channel), RImg, vec, mbNum, channel, original);
				}

				for(int idx = 0; idx < planeSize[channel]; idx++){
					vout << original.at<uchar>(idx);
				}
			}
		}
		if(i != periodicity){
			frameType = FrameType::Predictive;
			i++;
		}
		else{
			frameType = FrameType::Intra;
			i = 0;
		}
	}
}