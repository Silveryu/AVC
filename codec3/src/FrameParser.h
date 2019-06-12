#ifndef FRAMEPARSER_H
#define FRAMEPARSER_H

#include <vector>
#include <array>
#include <stdio.h>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <float.h>
using namespace cv;
using namespace std;

class FrameParser {
    private:
        int y, u, v, b, g, r;

    public:
        string format;
        double fps; /* frames per second */
        int yCols, yRows; /* y frame dimension */
        int ySize;
        int uvCols, uvRows; /* uv frame dimension */
        int uvSize;
        int frameSize;
        string header;
        int yBlockSize;
        int uvBlockSizeWidth;
        int uvBlockSizeHeight;
        int searchArea;
        int c1_quant;
        int c2_quant;
        int c3_quant;

        FrameParser(string header) : header(header){
            parseHeader(header, yCols, yRows, fps, format, true);
            ySize = yCols * yRows;
            if(format == "444"){
                frameSize = ySize * 3;
                uvCols = yCols;
                uvRows = yRows;
            }
            else if(format == "422"){
                frameSize = ySize * 2;
                uvCols = yCols / 2;
                uvRows = yRows;
            }
            else{
                frameSize = ySize * 3/2;
                uvCols = yCols / 2;
                uvRows = yRows / 2;
            }
            uvSize = uvCols * uvRows;
        }

        void planarToPackedRGB(unsigned char *frame,int bufferPos, int& r, int& g, int& b){
            /* Accessing to planar info */
            planarToPackedYUV(frame, bufferPos, y, u, v);
            yuv2rgb(y, u, v, r, g, b);
        }

        void planarToPackedYUV(unsigned char *frame, int bufferPos, int& y, int& u, int& v){
            /* Accessing to planar info */
            if(format == "444"){
                y = frame[bufferPos / 3]; 
                u = frame[bufferPos / 3 + ySize]; 
                v = frame[bufferPos / 3 + ySize*2]; 
            }
            else if(format == "422"){
                y = frame[bufferPos / 3]; 
                u = frame[(bufferPos / 6) + ySize]; 
                v = frame[(bufferPos / 6) + ySize*3/2]; 
            }
            else if(format == "420"){
                int idx = ( (bufferPos / 6) % (yCols / 2)) + (yCols / 2) * ( bufferPos / (yCols * 6) );
                y = frame[bufferPos / 3];
                u = frame[idx + ySize];
                v = frame[idx + ySize*5/4];
            }
        }

        void getPlanarChannels(unsigned char *frame, Mat& yImg, Mat& uImg, Mat& vImg){
            
            yImg = Mat(Size(yCols, yRows), CV_8UC1);
            uImg = Mat(Size(uvCols, uvRows), CV_8UC1);
            vImg = Mat(Size(uvCols, uvRows), CV_8UC1);
            
            uchar *buffer; // unsigned char pointer to the Mat data

            buffer = (uchar*)yImg.ptr();
            for(int i = 0; i < ySize; i++)
                buffer[i] = frame[i];

            buffer = (uchar*)uImg.ptr();
            for(int i = 0; i < uvSize; i++)
                buffer[i] = frame[ySize + i];
            
            buffer = (uchar*)vImg.ptr();
            for(int i = 0; i < uvSize; i++)
                buffer[i] = frame[ySize + uvSize + i];
        }

        void getPlanarResiduals(int *residuals, Mat& yImg, Mat& uImg, Mat& vImg){
            
            yImg = Mat(Size(yCols, yRows), CV_16SC1);
            uImg = Mat(Size(uvCols, uvRows), CV_16SC1);
            vImg = Mat(Size(uvCols, uvRows), CV_16SC1);
            
            short* buffer; //  pointer to the Mat data

            buffer = (short*)yImg.ptr();
            for(int i = 0; i < ySize; i++)
                buffer[i] = residuals[i];

            buffer = (short*)uImg.ptr();
            for(int i = 0; i < uvSize; i++)
                buffer[i] = residuals[ySize + i];
            
            buffer = (short*)vImg.ptr();
            for(int i = 0; i < uvSize; i++)
                buffer[i] = residuals[ySize + uvSize + i];
        }

        static void yuv2rgb(int y, int u, int v, int& r, int& g, int& b){
            b = (int)(1.164*(y - 16) + 2.018*(u-128));
            g = (int)(1.164*(y - 16) - 0.813*(u-128) - 0.391*(v-128));
            r = (int)(1.164*(y - 16) + 1.596*(v-128));

            /* clipping to [0 ... 255] */
            if(r < 0) r = 0;
            if(g < 0) g = 0;
            if(b < 0) b = 0;
            if(r > 255) r = 255;
            if(g > 255) g = 255;
            if(b > 255) b = 255;
        }
        
        static void rgb2yuv(int r, int g, int b, int& y, int& u, int& v){
            /* if you need the inverse formulas */
            y = r *  .299 + g *  .587 + b *  .114 ;
            u = r * -.169 + g * -.332 + b *  .500  + 128.;
            v = r *  .500 + g * -.419 + b * -.0813 + 128.;
            
            /* clipping to [0 ... 255] */
            if(y < 0) y = 0;
            if(u < 0) u = 0;
            if(v < 0) v = 0;
            if(y > 255) y = 255;
            if(u > 255) u = 255;
            if(v > 255) v = 255;
        }

        static void parseHeader(string header, int& yCols, int& yRows, double& fps, string& format, bool print = true){
            
            yCols = stoi(header.substr(header.find(" W") + 2, header.find(" H") - header.find(" W") - 2));
            yRows = stoi(header.substr(header.find(" H") + 2, header.find(" F") - header.find(" H") - 2));
            int pos = header.find(" F");
            int fpsDelimPos = header.find(":", pos);
            fps = stoi(header.substr(pos + 2, fpsDelimPos - pos - 1)) /
                stod(header.substr(fpsDelimPos + 1, header.find(" I") - fpsDelimPos - 1));
            if(header.find(" C") != string::npos)
                format = header.substr(header.find(" C") + 2, header.find("\n") - header.find(" C") - 2);
            else
                format = "420";
            
            if(print){
                cout << header << endl;
                cout << "dim: " << yRows << ", " << yCols << endl;
                cout << "format: " << "YUV"+format << endl;
                cout << "fps: " << fps << endl;
            }
        }

        // calculate residuals according to jpegLS
        // intra coding
        void encodeLS(vector<int>& residuals, Mat img, int size, int nCols){
            unsigned char a, b, c, p, x;
            for(int i = 0; i < size; i++){
                bool firstCol = (i / nCols == 0);
                bool firstRow = (i < nCols);

                // padding with 0s 
                a = firstCol ? 0 : img.at<uchar>(i-1);
                b = firstRow ? 0 : img.at<uchar>(i-nCols);
                c = ( firstCol || firstRow ) ? 0 : img.at<uchar>(i-nCols-1); // = erro 
                x = img.at<uchar>(i);

                // JPEG-LS predictor
                p = jpegLs(a, b, c);
                // e = x - p;
                //  e >>= k;
                // img.at<uchar>(i) = p + e << k;
                residuals.push_back(x - p); // e
		    }
        }


        // intra coding
        void decodeLS(vector<unsigned char>& original, Mat img, int size, int nCols){
            unsigned char a, b, c, p;

            for(int i = 0; i < size; i++){
                bool firstCol = (i / nCols == 0);
                bool firstRow = (i < nCols);

                // padding with 0s 
                a = firstCol ? 0 : img.at<short>(i-1);
                b = firstRow ? 0 : img.at<short>(i-nCols);
                c = ( firstCol || firstRow ) ? 0 : img.at<short>(i-nCols-1);

                // JPEG-LS predictor
                p = jpegLs(a, b, c);
                img.at<short>(i) += p;

                original.push_back( img.at<short>(i) );
            }
        }
                // quant is how many bits we want to remove this time
        void encodeLossy(vector<int>& residuals, Mat img, int size, int nCols, int quant){
            unsigned char a, b, c, p, x;
            int r, e;
            for(int i = 0; i < size; i++){
                bool firstCol = (i / nCols == 0);
                bool firstRow = (i < nCols);

                // padding with 0s 
                a = firstCol ? 0 : img.at<uchar>(i-1);
                b = firstRow ? 0 : img.at<uchar>(i-nCols);
                c = ( firstCol || firstRow ) ? 0 : img.at<uchar>(i-nCols-1); // = erro 
                x = img.at<uchar>(i);

                // JPEG-LS predictor
                p = jpegLs(a, b, c);
                r = x - p;
                r >>= quant;
                e = r << quant;
                img.at<uchar>(i) = e + p;
                residuals.push_back(r);
		    }
        }

        void decodeLossy(vector<unsigned char>& original, Mat img, int size, int nCols, int quant){
            unsigned char a, b, c, p;

            for(int i = 0; i < size; i++){
                bool firstCol = (i / nCols == 0);
                bool firstRow = (i < nCols);

                // padding with 0s 
                a = firstCol ? 0 : img.at<short>(i-1);
                b = firstRow ? 0 : img.at<short>(i-nCols);
                c = ( firstCol || firstRow ) ? 0 : img.at<short>(i-nCols-1);

                // JPEG-LS predictor
                p = jpegLs(a, b, c);
                img.at<short>(i) =  (img.at<short>(i) << quant) + p  ;
                original.push_back( img.at<short>(i) );
            }
        }
        
        static unsigned char jpegLs(unsigned char a, unsigned char b, unsigned char c){       
            if(c >= max(a, b)){
                return min(a, b);
            }
            else if(c <= min(a, b)){
                return max(a, b);
            }
            else{
                return a + b - c;
            }
        }

        void setPredictiveParams(int blockSize, int searchArea){

            this->yBlockSize = blockSize;
            if(format == "444"){
                this->uvBlockSizeWidth = blockSize;
                this->uvBlockSizeHeight = blockSize;
            }
            else if(format == "422"){
                this->uvBlockSizeWidth = blockSize / 2;
                this->uvBlockSizeHeight = blockSize;
            }
            else{
                this->uvBlockSizeWidth = blockSize / 2;
                this->uvBlockSizeHeight = blockSize / 2;
            }
            this->searchArea = searchArea;
        }

        void setLossyParams(int c1_quant, int c2_quant, int c3_quant){
            this->c1_quant = c1_quant;
            this-> c2_quant = c2_quant;
            this-> c3_quant = c3_quant;
        }

        double motionCompensationEncodeSingleChannel(Mat IImg, Mat PImg, int mbNum, int channel, Mat& bestResiduals, Point2i& bestMotionVector){
            
            // channel 0 -> y
            int channelBlockSizeHeight;
            int channelBlockSizeWidth;

            if(channel == 0){
                channelBlockSizeWidth = yBlockSize;
                channelBlockSizeHeight = yBlockSize;
            }
            else{
                channelBlockSizeWidth = uvBlockSizeWidth;
                channelBlockSizeHeight = uvBlockSizeHeight;
            }

            // convert to signed to subtract directly
            IImg.convertTo(IImg, CV_16SC1);
            PImg.convertTo(PImg, CV_16SC1);

            Point2i topLeftPoint;
            Mat PMacroblock = getMacroblock(PImg, channelBlockSizeWidth, channelBlockSizeHeight, mbNum, topLeftPoint);
            vector<Point2i> motionVectors = mbMotionVectors(PImg.cols, PImg.rows, topLeftPoint, channelBlockSizeWidth, channelBlockSizeHeight, searchArea);
            
            double bestError = std::numeric_limits<double>::max();
            for(auto motionVector : motionVectors){
                Mat IMatch = Mat(IImg, cv::Rect(topLeftPoint + motionVector, Size(channelBlockSizeWidth, channelBlockSizeHeight)));                
                Mat res = PMacroblock - IMatch;
                double error = sum(abs(res))[0];
                if(error < bestError){
                    bestResiduals = res;
                    bestError = error;
                    bestMotionVector = motionVector;
                }
            }
            return bestError;
        }

        double motionCompensationEncodeSingleChannelLossy(Mat IImg, Mat PImg, int mbNum, int channel, Mat& bestResiduals, Point2i& bestMotionVector){
            
            // channel 0 -> y
            int channelBlockSizeHeight;
            int channelBlockSizeWidth;

            if(channel == 0){
                channelBlockSizeWidth = yBlockSize;
                channelBlockSizeHeight = yBlockSize;
            }
            else{
                channelBlockSizeWidth = uvBlockSizeWidth;
                channelBlockSizeHeight = uvBlockSizeHeight;
            }

            // convert to signed to subtract directly
            IImg.convertTo(IImg, CV_16SC1);
            PImg.convertTo(PImg, CV_16SC1);

            Point2i topLeftPoint;
            Mat PMacroblock = getMacroblock(PImg, channelBlockSizeWidth, channelBlockSizeHeight, mbNum, topLeftPoint);
            vector<Point2i> motionVectors = mbMotionVectors(PImg.cols, PImg.rows, topLeftPoint, channelBlockSizeWidth, channelBlockSizeHeight, searchArea);
            
            double bestError = std::numeric_limits<double>::max();
            for(auto motionVector : motionVectors){
                Mat IMatch = Mat(IImg, cv::Rect(topLeftPoint + motionVector, Size(channelBlockSizeWidth, channelBlockSizeHeight)));                
                Mat res = PMacroblock - IMatch;
                double error = sum(abs(res))[0];
                if(error < bestError){
                    bestResiduals = res;
                    bestError = error;
                    bestMotionVector = motionVector;
                }
            }
            
            
            return bestError;



        }

        void motionCompensationDecodeSingleChannel(Mat IImg, Mat RImg, Point2i motionVector, int mbNum, int channel, Mat& result){
            // channel 0 -> y
            int channelBlockSizeHeight;
            int channelBlockSizeWidth;

            if(channel == 0){
                channelBlockSizeWidth = yBlockSize;
                channelBlockSizeHeight = yBlockSize;
            }
            else{
                channelBlockSizeWidth = uvBlockSizeWidth;
                channelBlockSizeHeight = uvBlockSizeHeight;
            }

            IImg.convertTo(IImg, CV_16SC1);
            Point2i topLeftPoint;            
            getTopLeftPoint(IImg.cols, channelBlockSizeWidth, channelBlockSizeHeight, mbNum, topLeftPoint);
            Mat IMatch = Mat(IImg, cv::Rect(topLeftPoint + motionVector, Size(channelBlockSizeWidth, channelBlockSizeHeight)));
            RImg = RImg + IMatch;

            // copy to original image
            Mat tmp = result(Rect(topLeftPoint, Size(channelBlockSizeWidth, channelBlockSizeHeight)));
            RImg.convertTo(RImg, CV_8UC1);
            RImg.copyTo(tmp);
        }
        
        // top left point is (col, row) indexed
        // assume mbNum is never greater than what is possible 
        static Mat getMacroblock(Mat img, int blockSizeWidth, int blockSizeHeight, int mbNum, Point2i& topLeftPoint){
            getTopLeftPoint(img.cols, blockSizeWidth, blockSizeHeight, mbNum, topLeftPoint );
            return Mat(img, cv::Rect(topLeftPoint, Size(blockSizeWidth, blockSizeHeight)));
        }

        static void getTopLeftPoint(int nCols, int blockSizeWidth, int blockSizeHeight, int mbNum, Point2i& topLeftPoint){
            int mbCols = nCols / blockSizeWidth;
            topLeftPoint = Point2i(blockSizeWidth * (mbNum % mbCols), blockSizeHeight * (mbNum / mbCols));
        }

        // get possible motion vectors according to position
        // recursive boi
        static vector<Point2i> mbMotionVectors(int nCols, int nRows, Point2i topLeftPoint, int blockSizeWidth, int blockSizeHeight, int searchArea){
            vector<Point2i> possVec;

            if(searchArea == 0){
                possVec.push_back(Point2i(0, 0));
                return possVec;
            }

            // rec add smaller borders
            bool Ncond = topLeftPoint.y - searchArea >= 0;
            bool Scond = topLeftPoint.y + blockSizeHeight + searchArea <= nRows;
            bool Wcond = topLeftPoint.x - searchArea >= 0;
            bool Econd = topLeftPoint.x + blockSizeWidth + searchArea <= nCols;
            
            // N
            if(Ncond){
                possVec.push_back(Point2i(0, -searchArea));
                // NE
                if(Econd){
                    possVec.push_back(Point2i(searchArea, -searchArea));
                    for(int i=1; i < searchArea;i++){
                        possVec.push_back(Point2i(i, -searchArea));
                        possVec.push_back(Point2i(searchArea, -i));
                    }
                }
                // NW
                if(Wcond){
                    possVec.push_back(Point2i(-searchArea, -searchArea)); 
                    for(int i=1; i < searchArea;i++){
                        possVec.push_back(Point2i(-i, -searchArea));
                        possVec.push_back(Point2i(-searchArea, -i));
                    }
                }
            }
            
            // S
            if(Scond){
                possVec.push_back(Point2i(0, searchArea));
                // SE
                if(Econd){
                    possVec.push_back(Point2i(searchArea, searchArea));
                    for(int i=1; i < searchArea;i++){
                        possVec.push_back(Point2i(searchArea, i));
                        possVec.push_back(Point2i(i, searchArea));
                    }
                }
                // SW
                if(Wcond){
                    possVec.push_back(Point2i(-searchArea, searchArea));
                    for(int i=1; i < searchArea;i++){
                        possVec.push_back(Point2i(-searchArea, i));
                        possVec.push_back(Point2i(-i, searchArea));
                    }
                }
            }

            // W
            if(Wcond)
                possVec.push_back(Point2i(-searchArea, 0));

            // E 
            if(Econd)
                possVec.push_back(Point2i(searchArea, 0));
            
            vector<Point2i> insideVectors = mbMotionVectors(nCols, nRows, topLeftPoint, blockSizeWidth, blockSizeHeight, searchArea-1);
            possVec.insert(possVec.end(), insideVectors.begin(), insideVectors.end());
            return possVec;
        }

        static void mat2vec(Mat mat, vector<int>& vec){
            if (mat.isContinuous()) {
                vec.assign((short*)mat.datastart, (short*)mat.dataend);
            } else {
                for (short i = 0; i < mat.rows; ++i) 
                    vec.insert(vec.end(), mat.ptr<short>(i), mat.ptr<short>(i)+mat.cols);        
            }
        } 

        static void vec2mat(vector<int> vec, Mat& mat){
            for(size_t i=0; i < vec.size(); i++){
                mat.at<short>(i) = vec.at(i);
            }
        }

        void vec2channelMat(vector<int> vec, int channel, Mat& mat){
            
            if(channel == 0){
               mat = Mat(Size(yBlockSize, yBlockSize), CV_16SC1);
            }
            else{
                mat = Mat(Size(uvBlockSizeWidth, uvBlockSizeHeight), CV_16SC1);
            }
            vec2mat(vec, mat);
        }
        int channelBlockSize(int channel){
            if(channel == 0){
                return yBlockSize*yBlockSize;
            }
            else{
                return uvBlockSizeHeight * uvBlockSizeWidth;
            }
        }
        void initPlanarMat(Mat& mat, int channel){
            if(channel == 0){
                mat = Mat(Size(yCols, yRows), CV_8UC1);
            }
            else{
                mat = Mat(Size(uvCols, uvRows), CV_8UC1);
            }
        }
};


#endif