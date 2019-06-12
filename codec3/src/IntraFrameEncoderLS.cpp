#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <algorithm>
#include "golomb.h"
#include <regex>
#include <ctime>

using namespace cv;
using namespace std;

int main( int argc, char** argv )
{

    /* check for the mandatory arguments */
    if( argc < 2 ) {
        cerr << "Usage: IntraFrameEncoderLS filename" << endl;
        return 1;
    }

    /* Opening video file */
    ifstream myfile (argv[1]);

    string line; // store the header
    unsigned char *imgData; // file data buffer

    /* Processing header */
    getline (myfile,line);
    cout << line << endl;
    std::regex word_regex("(\\S+)");
    string format = "420"; // default

    int yCols = stoi(line.substr(line.find(" W") + 2, line.find(" H") - line.find(" W") - 2));
    int yRows = stoi(line.substr(line.find(" H") + 2, line.find(" F") - line.find(" H") - 2));
    
    int pos = line.find(" F");
    int fpsDelimPos = line.find(":", pos);

    double fps =    stoi(line.substr(pos + 2, fpsDelimPos - pos - 1)) /
    stod(line.substr(fpsDelimPos + 1, line.find(" I") - fpsDelimPos - 1));

    cout << "dim: " << yRows << ", " << yCols << endl;
    cout << "format: " << "YUV"+format << endl;
    cout << "fps: " << fps << endl;

    int frameSize;
    if(format == "444")
        frameSize = yCols * yRows * 3;
    else if(format == "422")
        frameSize = yCols * yRows * 2;
    else
        frameSize = yCols * yRows * 3/2;
    
    cout << "frame size: " <<  frameSize << endl;

    int end = 0;

    /* buffer to store the frame */
    imgData = new unsigned char[frameSize];
    
    int y, u, v;
    Mat mat_y, mat_u, mat_v;

    /* read file */
    while(!end){
        getline (myfile,line); // Skipping word FRAME
        myfile.read((char *)imgData, frameSize); // read frame
        if(myfile.gcount() == 0){ // eof
            end = 1;
            break;
        }

        if(format == "444"){
            mat_y = Mat(yRows,yCols,CV_8UC1);
            mat_u = Mat(yRows,yCols, CV_8UC1);
            mat_v = Mat(yRows,yCols, CV_8UC1);
        }else if(format == "422"){
            mat_y = Mat(yRows,yCols,CV_8UC1);
            mat_u = Mat(yRows,yCols/2, CV_8UC1);
            mat_v = Mat(yRows, yCols/2, CV_8UC1);
        }else{
            mat_y = Mat(yRows,yCols,CV_8UC1);
            mat_u = Mat(yRows/2,yCols/2, CV_8UC1);
            mat_v = Mat(yRows/2, yCols/2, CV_8UC1);         
        }

        int y_idx = 0;
        int uv_idx = 0;
        
        /*
        cout << "Y rows " << mat_y.rows << ", cols " << mat_y.cols <<  endl;
        cout << "U rows " << mat_u.rows << ", cols " << mat_u.cols <<  endl;
        cout << "V rows " << mat_v.rows << ", cols " << mat_v.cols <<  endl;
        */

        for(int i = 0 ; i < yRows * yCols * 3; i += 3){ 
        
            if(format == "444"){
                y = imgData[i / 3]; 
                u = imgData[i / 3 + (yRows * yCols)]; 
                v = imgData[i / 3 + (yRows * yCols)*2]; 

                if(i % yCols == 0 && i != 0)
                    y_idx++;

                mat_y.at<int>(y_idx, i % yCols) = y;
                mat_u.at<int>(y_idx, i % yCols) = u;
                mat_v.at<int>(y_idx, i % yCols) = v;

            }
            else if(format == "422"){
                y = imgData[i / 3]; 
                u = imgData[(i / 6) + (yRows * yCols)]; 
                v = imgData[(i / 6) + (yRows * yCols)*3/2]; 
                
                if(i % yCols == 0 && i != 0)
                    y_idx++;

                if(i % yCols/2 == 0 && i!=0)
                    uv_idx++;

                mat_y.at<int>(y_idx, i % yCols) = y;
                mat_u.at<int>(uv_idx, i % yCols/2) = u;
                mat_v.at<int>(uv_idx, i % yCols/2) = v;

            }
            else if(format == "420"){
                int idx = ( (i / 6) % (yCols / 2)) + 3 * ( i / (yCols * 6) );
                y = imgData[i / 3];
                u = imgData[idx + (yRows * yCols)];
                v = imgData[idx + (yRows * yCols)*5/4];
    

                if(i/3 % yCols == 0 && i != 0){ 
                    y_idx++;
                }

                mat_y.at<uchar>(y_idx, (i/3) % yCols) = y;
                
                if(i/3 % yCols/2 == 0 && i!=0)
                    uv_idx++;

                if(uv_idx != yRows/2){
                    mat_u.at<uchar>(uv_idx,  (i/3) % (yCols/2) ) = u;
                    mat_v.at<uchar>(uv_idx,  (i/3) % (yCols/2) ) = v;
                }           
                
            }

        }

        // prediction calc
        /*
        std::vector<cv::Mat> vec_yuv = { mat_y, mat_u, mat_v};

        uchar Ra, Rb, Rc;
        uchar prediction;
        uchar current_sample;
        uchar estimate;

        // for every component
        for(int k = 0; k<3; k++){
            for(int i=0; i < vec_yuv[k].rows ; i++ ){
                for(int j=0; j < vec_yuv[k].cols ; j++ ){

                    current_sample = vec_yuv[k].at<uchar>(i,j);

                    cout << current_sample << endl;

                    // no neighbor handling
                    if(i==0){
                        Rc = 0;
                        Rb = 0;
                    }else{
                        Rc = vec_yuv[k].at<uchar>(i-1,j-1);
                        Rb = vec_yuv[k].at<uchar>(i-1,j);
                    }
                    if(j==0){
                        Ra = 0;
                    }else{
                        Ra = vec_yuv[k].at<uchar>(i,j-1);
                    }

                    if(Rc >= max(Ra, Rb))
                        estimate = min(Ra, Rb);
                    else if(Rc <= min(Ra, Rb))
                        estimate = max(Ra, Rb);
                    else
                        estimate = Ra + Rb - Rc;

                prediction = current_sample - estimate;
                
                //cout << prediction << endl;

                }
            }
        }
        
        break;

        // Golomb here 
        */
    }

    return 0;
}
