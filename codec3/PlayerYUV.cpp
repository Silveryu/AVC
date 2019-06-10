#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
#include "FrameParser.h"

using namespace cv;
using namespace std;

/*static void drawOptFlowMap(const Mat& flow, Mat& cflowmap, int step,
                    double, const Scalar& color)
{
    for(int y = 0; y < cflowmap.rows; y += step)
        for(int x = 0; x < cflowmap.cols; x += step)
        {
            const Point2f& fxy = flow.at<Point2f>(y, x);
            line(cflowmap, Point(x,y), Point(cvRound(x+fxy.x), cvRound(y+fxy.y)), color);
            circle(cflowmap, Point(x,y), 2, color, -1);
        }
}*/

int main(int argc, char** argv)
{
	string line; // store the header
	int yCols, yRows; /* frame dimension */
	//int y, u, v;
	int r, g, b;
	double fps; /* frames per second */
	int frameSize;
	unsigned char *imgData; // file data buffer
	uchar *buffer; // unsigned char pointer to the Mat data
	char inputKey = '?'; /* parse the pressed key */
	int end = 0, playing = 1, loop = 0; /* control variables */
	string format = "420";

	/* check for the mandatory arguments */
	if( argc < 2 ) {
		cerr << "Usage: PlayerYUV filename" << endl;
		return 1;
	}

	/* Opening video file */
	ifstream myfile (argv[1]);

	/* Processing header */
	getline (myfile,line);
	FrameParser parser {line};

	yCols = parser.yCols;
	yRows = parser.yRows;
	fps = parser.fps;
	format = parser.format;
	frameSize = parser.frameSize;

	/* Parse other command line arguments */
	for(int n = 1 ; n < argc ; n++)
	{
		if(!strcmp("-fps", argv[n]))
		{
			fps = atof(argv[n+1]);
			n++;
		}

		if(!strcmp("-wait", argv[n]))
		{
			playing = 0;
		}

		if(!strcmp("-loop", argv[n]))
		{
			loop = 1;
		}
	}

	/* data structure for the OpenCv image */
	// 8 bit unsigned mat
	Mat img = Mat(Size(yCols, yRows), CV_8UC3);

	/* buffer to store the frame */
	imgData = new unsigned char[frameSize];

	/* create a window */
	namedWindow( "rgb", CV_GUI_NORMAL);
	// Optical Flow stuff
	Mat flow, cflow, frame;
	UMat gray, prevgray, uflow;
	//namedWindow("flow", 1);
	while(!end)
	{
		/* load a new frame, if possible */
		getline (myfile,line); // Skipping word FRAME
		myfile.read((char *)imgData, frameSize);
		if(myfile.gcount() == 0)
		{
			if(loop)
			{
				myfile.clear();
				myfile.seekg(0);
				getline (myfile,line); // read th+e header
				continue; 
			}
			else
			{
				end = 1;
				break;
			}
		}

		/* The video is stored in YUV planar mode but OpenCv uses packed modes*/
		buffer = (uchar*)img.ptr();
		
		for(int i = 0 ; i < yRows * yCols * 3; i += 3)
		{ 
			parser.planarToPackedRGB(imgData, i, r, g, b);
			/* Fill the OpenCV buffer - packed mode: BGRBGR...BGR */
			buffer[i] = b;
			buffer[i + 1] = g;
			buffer[i + 2] = r;
		}
		
		/* display the image */
		imshow( "rgb", img );
		if(playing)
		{
			/* wait according to the frame rate */
			inputKey = waitKey(1.0 / fps * 1000);
		}
		else
		{
			/* wait until user press a key */
			inputKey = waitKey(0);
		}
	
		/* parse the pressed keys, if any */
		switch((char)inputKey)
		{
			case 'q':
				end = 1;
				break;
			
			case 'p':
				playing = playing ? 0 : 1;
				break;
		}
	}
	
	return 0;
}
