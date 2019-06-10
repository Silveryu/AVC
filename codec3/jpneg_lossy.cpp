#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
#include "FrameParser.h"
#include "golomb.h"

using namespace cv;
using namespace std;

void encode(string video_file, string encoded_file, int y_quant, int u_quant, int v_quant);
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
        if(argc < 7){
            printUsage();
            return -1;
        }
		string video_file = argv[2];
		string encoded_file = argv[3];
        int y_quant, u_quant, v_quant;
		try {
			y_quant = stoi(argv[4]);
			u_quant = stoi(argv[5]);
			v_quant = stoi(argv[6]);
		}
		catch (std::invalid_argument& e) {
			printUsage();
			return 1;
		}
		encode(video_file, encoded_file, y_quant, u_quant, v_quant);
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
		 << "\n\t" << "jpneg_lossy -e|--encode <video_file> <encoded_file> <y_quant> <u_quant> <v_quant>" 
		 << "\n\t" << "jpneg_lossy -d|--decode <encoded_file> <video_file>" 
		 << endl;
}

void encode(string video_file, string encoded_file, int y_quant, int u_quant, int v_quant){
	
	string line; // store the header
	int yCols; /* frame dimension */
	int ySize;
	int uvCols;
	int uvSize;
	int frameSize;
	unsigned char *imgData; // file data buffer
	string format = "420";
	Mat yImg, uImg, vImg, img;
	video_metadata video_meta;
	block_metadata block_meta;
    lossy_metadata loss_meta;
	bool estimatedM = false;
	
	/* Opening video file */
	ifstream myfile (video_file);

	/* Processing header */
	getline (myfile,line);
	FrameParser parser {line};

	yCols = parser.yCols;
	ySize = parser.ySize;

	uvCols = parser.uvCols;
	uvSize = parser.uvSize;

	format = parser.format;
	frameSize = parser.frameSize;

	/* buffer to store the frame */
	imgData = new unsigned char[frameSize];
	
	video_meta.header = parser.header;

	// by default putting golomb param 15
	Golomb gout = Golomb(encoded_file, std::fstream::out);
	gout.writeVideoFileMetadata(video_meta);

    loss_meta.c1_quant = y_quant;
    loss_meta.c2_quant = u_quant;
    loss_meta.c3_quant = v_quant;
    gout.writeLossyMetadata(loss_meta);
	
	//namedWindow("flow", 1);
	while(true){

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

		vector<int> residuals;
		residuals.reserve(frameSize);

		// get residuals of each plane
		parser.encodeLossy(residuals, yImg, ySize, yCols, y_quant);
		parser.encodeLossy(residuals, uImg, uvSize, uvCols, u_quant);
		parser.encodeLossy(residuals, vImg, uvSize, uvCols, v_quant);


		// one m for whole video
		if(!estimatedM){
			Golomb::estimateM(residuals, block_meta);
			gout.writeBlockMetadata(block_meta);
			cout << "Golomb parameter: "<< block_meta.m << endl;
			estimatedM = true;
		}
		gout.encodeN(residuals.data(), residuals.size(), block_meta.m);
	}
	gout.close();
}

void decode(string encoded_file, string video_file){

	string line; // store the header
	int yCols; /* frame dimension */
	int ySize;
	int uvCols;
	int uvSize;
	int frameSize;
	int *residuals; // file data buffer
    int y_quant;
    int u_quant;
    int v_quant;
	Mat yImg, uImg, vImg;
	video_metadata video_meta;
	block_metadata block_meta;
    lossy_metadata lossy_meta;


	string format;

	Golomb gin = Golomb(encoded_file, std::fstream::in);
	gin.readVideoFileMetadata(video_meta);

	FrameParser parser { video_meta.header };
	yCols = parser.yCols;
	ySize = parser.ySize;
	uvCols = parser.uvCols;
	uvSize = parser.uvSize;
	format = parser.format;
	frameSize = parser.frameSize;
    
    gin.readLossyMetadata(lossy_meta);

    y_quant = lossy_meta.c1_quant;
    u_quant = lossy_meta.c2_quant;
    v_quant = lossy_meta.c3_quant;
	
	gin.readBlockMetadata(block_meta);
	cout << "Golomb parameter: "<< block_meta.m << endl;
	
	residuals = new int[frameSize];
	
	ofstream vout;
	vout.open(video_file, std::ofstream::out);
	vout << video_meta.header << "\n";
	
	while(true){
		if(!gin.decodeN(residuals, frameSize, block_meta.m)){
			break;
		}
		parser.getPlanarResiduals(residuals, yImg, uImg, vImg);
		
		vector<unsigned char> original;
		original.reserve(frameSize);
		
		parser.decodeLossy(original, yImg, ySize, yCols, y_quant);
		parser.decodeLossy(original, uImg, uvSize, uvCols, u_quant);
		parser.decodeLossy(original, vImg, uvSize, uvCols, v_quant);

		vout << "FRAME\n";
		for (int i = 0; i < frameSize; i++){
			vout << original.at(i);
		}
	}
}

