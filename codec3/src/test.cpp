#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include "FrameParser.h"
#include "golomb.h"

using namespace cv;
using namespace std;

void encode(string video_file, string encoded_file);
void decode(string encoded_file, string video_file);
void printUsage();

int main()
{   
    std::vector<int> v;
    Mat img = Mat(Size(10, 10), CV_8UC1);

    for(int i = 0; i < 100; i++)
        img.at<uchar>(i) = i;
    

    cout << "Vector of floats via Mat = " << img << endl;

    Point2i topLeftPoint;


    Mat macro = FrameParser::getMacroblock(img, 2, 1, 2, topLeftPoint);
    cout << "topLeftPoint: " << topLeftPoint << endl;
    
    //Mat(img, cv::Rect(topLeftPoint, Size(blockSize, blockSize)));
    cout << macro << endl;

    vector<Point2i> motionVectors = FrameParser::mbMotionVectors(img.cols, img.rows, topLeftPoint, 2, 1, 2);
    for(auto motionVector : motionVectors)
        cout << motionVector << endl;

}
    

