#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <algorithm>
//#include "golomb.h"

using namespace cv;
using namespace std;

int main( int argc, char** argv )
{
    if( argc != 2)
    {
     cout <<" Usage: test ImageToLoadAndDisplay" << endl;
     return -1;
    }

    Mat img;
    img = imread(argv[1], CV_LOAD_IMAGE_COLOR);   // Read the file

    if(! img.data ) // Check for invalid input
    {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }

    cout << "COLUMNS: " << img.cols << endl;
    cout << "ROWS: " <<img.rows << endl;

    const int MAX_VAL = 255;
    const int MAX_C = 127;
    const int MIN_C = -128;

    const int alpha = 255;

    const int NEAR = 0; // lossless coding
    const int RESET = 64; // default value
    int SIGN;

    /* RANGE parameter computation */
    int RANGE;
    if(NEAR > 0)
        RANGE = abs((MAX_VAL + 2 * NEAR)/(2*NEAR +1)) + 1;
    else
        RANGE = MAX_VAL + 1;

    /*  qpbpp and bpp parameters computation */
    int qbpp = std::log2(RANGE);
    int tmp = ceil(std::log2(MAX_VAL + 1));
    int bpp = max(2,tmp);

    /* LIMIT parameter computation */
    int LIMIT = 2 * (bpp + max(8,bpp));

    /*
    int tmp = ceil(std::log2(alpha));
    int beta_max = max(2,tmp);
    int L_max = 2 * (beta_max  + max(8,beta_max));
    */

    int A[367], N[367], B[365], C[365]; // context counters

    /* Initialize context counters */
    for(int i=0; i < 365; i++){
        B[i] = 0;
        C[i] = 0;
    }

    tmp = floor((RANGE+32)/64);
    int valA = max(2,tmp); 

    for(int i=0 ; i<367 ; i++){
        N[i] = 1;
        A[i] = valA;
    }

    /* Initialize run mode variables */
    int RUNindex = 0;
    int J[32] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3
                ,4, 4, 5, 5, 6, 6, 7, 7, 8, 9,10,11,12,13,14,15};

    /* Intialize run interruption variables */
    int Nn[366];
    // Initialize interruption variables
    Nn[366] = 0;
    Nn[365] = 0;

    /** Set gradients of first sample **/
    Vec3b Ra;
    Vec3b Rb;
    Vec3b Rc;
    Vec3b Rd;

    Vec3b D1;
    Vec3b D2;
    Vec3b D3;

    std::vector<cv::Vec3b> D = { D1, D2, D3};

    Vec3b Q1;
    Vec3b Q2;
    Vec3b Q3;
    std::vector<cv::Vec3b> Q = { Q1, Q2, Q3};

    /* Default threshold values */
    const int BASIC_T1 = 3;
    const int BASIC_T2 = 7;
    const int BASIC_T3 = 21;

    Vec3b current_sample;

    /* Set first sample as current sample */ 
    /* current_sample represents the sample to be encoded in each iteration */
    //current_sample = img.at<cv::Vec3b>(0,0);

    for(int i=0; i<img.rows; i++){

        for(int j=0; j<img.cols; j++){

            current_sample = img.at<cv::Vec3b>(j,i);

            if(i==0){
                Rc = { 0,0,0 };
                Rb = { 0,0,0 };
                Rd = { 0,0,0 };
                if(j==0){
                    Ra = { 0,0,0 };
                }else{
                    Ra = img.at<cv::Vec3b>(j-1,i);
                }
            }else{
                Rb = img.at<cv::Vec3b>(j,i-1);
                if(j==0){
                    Rc = { 0,0,0 };
                    Ra = { 0,0,0 };
                }else if (j == img.cols-1){
                    Rd = { 0,0,0} ;
                }else{
                    Ra = img.at<cv::Vec3b>(j-1,i);
                    Rc = img.at<cv::Vec3b>(j-1,i-1); 
                    Rd = img.at<cv::Vec3b>(j+1,i-1);
                } 
            }

            /* compute local gradients D1, D2, D3 */ 
            for(int k=0;k<3;k++){
                D1[k] = Rd[k] - Rb[k];
                D2[k] = Rb[k] - Rc[k];
                D3[k] = Rc[k] - Rb[k];
            }

            /* Check if current sample will be encoded regularly or in run mode */
            /* Mode selection for lossless coding */
            if((cv::sum(D1)[0]) == 0 && (cv::sum(D2)[0]) == 0 && (cv::sum(D3)[0]) == 0){ // RUN mode
                
                /* RUN mode steps
                * RunLengthScanning();
                * RunLengthCoding();
                * RunInterruptionCoding();
                */

                /* Run-length determination for run mode*/
                /*
                Vec3b RUNval = Ra;
                Vec3b Rx;
                int RUNcnt = 0;
                while(abs(current_sample - RUNval) <= NEAR){
                    RUNcnt += 1;
                    Rx = RUNval;
                    if(EOLine == 1);
                }else{
                    GetNextSample();
                }

                while(RUNcnt >= (1 << J[RUNindex])){
                    AppendToBitStream(1,1);
                    RUNcnt -= ( 1 << J[RUNindex]);
                    if(RUNindex < 31)
                        RUNindex += 1;
                }

                if(abs(Ix - RUNval) > NEAR){
                    AppendToBitStream(0,1);
                    AppendToBitStream(RUNcnt, J[RUNindex]);
                    if(RUNindex > 0)
                        RUNindex -= 1;
                }else if(RUNcnt > 0)
                    AppendToBitStream(1,1);
                */

                /* Run interruption sample encoding */ 
                /** Index computation **/
                /*
                if(abs(Ra - Rb) <= NEAR)
                    RItype = 1;
                else
                    RItype = 0;                
                */

                /** Prediction error computation **/ 
                /*
                if(RItype == 1)
                    Px = Ra;
                else
                    Px = Rb;

                Errval = Ix - Px;    
                */

                /** Error computation for a run interruption sample **/ 
                /*
                if((RItype == 0) && (Ra > Rb)){
                    Errval = - Errval;
                    SIGN = -1;
                }else
                    SIGN = 1;
                if(NEAR > 0){
                    Errval = Quantize(Errval);
                    Rx = ComputeRx();
                }else{
                    Rx = Ix;
                }
                Errval = ModRange(Errval, RANGE);
                */

                /** Compute aux variable TEMP to help Golomb k value calculation **/
                /*
                if(RItype == 0)
                    TEMP = A[365];
                else
                    TEMP = A[366] + (N[366] >> 1);
                */

                /* Set Q value */
                //Q = RItype + 365;

                /** compute golomb_k variable **/
                /*
                int golomb_k;
                for(int k=0; k<3; k++){
                    for(golomb_k=0; (N[sample_context[k]] << golomb_k) < TEMP; golomb_k++);
                }
                */

                /** Computation of map for Errval mapping **/ 
                /*
                if((golomb_k==0) && (Errval > 0) && (2*Nn[Q] < N[Q]))
                    map = 1;
                else if((Errval < 0) && (2 * Nn[Q] >= N[Q]))
                    map = 1;
                else if((Errval <0) && (golomb_k != 0))
                    map = 1;
                else
                    map = 0;
                */

                /** Errval mapping for run interruption sample **/ 
                // EMErrval  = 2 * abs(Errval) - RItype - map;

                /** Encode EMErrval using function LG(golomb_k, glimit) */
                 // glimit = LIMIT - J[RUNindex] - 1;                

                /** Update of variables for run interruption sample **/
                /*
                if(Errval < 0)
                    Nn[Q] += 1;
                A[Q] = A[Q] + ((EMErrval + RItype) >> 1);
                if(N[Q] == RESET){
                    A[Q] = A[Q] >> 1;
                    N[Q] =  N[Q] >> 1;
                    Nn[Q] = Nn[Q] >> 1;
                }
                N[Q] += 1;
                */

            }else{ // REGULAR mode

                /* quantize the local gradients */
                for(int k=0;k<3;k++){
                    for(int l; l<3;l++){
                         if(D[k][l] <= -BASIC_T3) Q[k][l] = -4;
                         else if(D[k][l] <= -BASIC_T2) Q[k][l] = -3;
                         else if(D[k][l] <= -BASIC_T1) Q[k][l] = -2;
                         else if(D[k][l] <  -NEAR) Q[k][l] = -1;
                         else if(D[k][l] <=  NEAR) Q[k][l] = 0;
                         else if(D[k][l] < BASIC_T1) Q[k][l] = 1;
                         else if(D[k][l] < BASIC_T2) Q[k][l] = 2;
                         else if(D[k][l] < BASIC_T3) Q[k][l] = 3;
                         else D[k][l] = 4;
                    }
                }

                /* Quantized gradient merging */
                /* If the first non-zero element of Q is negative
                   then all signs of Q are reversed */
                /* TODO: Optimize */
                for(int k=0; k<3; k++){
                    if(Q[k][0] != 0 && Q[k][0] < 0){
                        for(int k=0; k<3;k++){
                            for(int l=0; l<3; l++){
                                Q[k][l] *= -1;
                            }
                        }
                        SIGN = -1;
                    }else{
                        SIGN = 1;
                    }   
                }  

                /*  Map Q into an integer value representing the context of the sample
                    Q values are in [-4,4] range 
                    They become in range [0,8] by interleave */
                int max_q = 8;
                /* The one-to-one mapping formula is as follows
                        k = (x * (max(y)+1) * (max(z) + 1)) + (y * (max(z) + 1)) + z
                   In our case x,y,z correspond to the B,G,R components
                   and max(x)=max(y)=max(z)=max(q) = 8
                */
            
                Vec3b sample_context;
                for(int k=0; k<3; k++){
                    sample_context[k] = (Q[k][0] * (max_q+1) * (max_q+1)) + (Q[k][1] * (max_q + 1)) + Q[k][2];
                }

                /* Prediction step */

                /** Edge detecting predictor **/
                Vec3b Px; /* Estimate of current sample */
                for(int k=0;k<3;k++){
                    if(Rc[k] >= max(Ra[k],Rb[k])) 
                        Px[k] = min(Ra[k],Rb[k]);
                    else{
                        if(Rc[k] <= min(Ra[k],Rb[k])) 
                            Px[k] = max(Ra[k],Rb[k]);
                        else 
                            Px[k] = Ra[k] + Rb[k] - Rc[k];
                    }
                }

                /** Prediction correction **/ 
                for(int k=0;k<3;k++){
                    if(SIGN == +1) 
                        Px[k] = Px[k] + C[sample_context[k]];
                    else 
                        Px[k] = Px[k] - C[sample_context[k]];
                    // clamp Px values
                    if(Px[k] > MAX_VAL)
                        Px[k] = MAX_VAL;
                    else if(Px[k] < 0)
                        Px[k] = 0; 
                }

                /** Computation of prediction error **/
                Vec3b Errval = current_sample - Px;
                if(SIGN == -1) Errval *= -1;
            
                /* Error quantization  */
                Vec3b Rx; // reconstructed value
                if(NEAR == 0){ // lossless coding
                    Rx = current_sample;
                }else{ // near-lossless coding
                    /* Error quantization and computaton of reconstructed value */
                    for(int k=0;k<3;k++){
                        if(Errval[k] > 0)
                            Errval[k] = (Errval[k] + NEAR) / (2 * NEAR + 1);
                        else
                            Errval[k] = - (NEAR - Errval[k]) / (2 * NEAR +1);

                        Rx[k] = Px[k] + SIGN * Errval[k] * (2 * NEAR + 1);
                        
                        if(Rx[k] < 0)
                            Rx[k] = 0;
                        else if(Rx[k] > MAX_VAL)
                            Rx[k] = MAX_VAL;
                    }
                }

                /** Modulo reduction of predictor error **/
                
                /*
                for(int k=0; i<3; k++){
                    if(Errval[k] < 0) 
                        Errval[k] += RANGE;
                    if(Errval[k] <= ((RANGE+1)/2))
                        Errval[k] -= RANGE;  
                }
                */
                   
                /**  Compute golomb coding variable **/
                /* Variable k for function LG(k,LIMIT) */
                int golomb_k;
                /*
                for(int k=0; k<3; k++){
                    for(golomb_k=0; (N[sample_context[k]] << golomb_k) < A[sample_context[k]]; golomb_k++);
                }
                */
                
                Vec3b MErrval;
                /** Error mapping no non-negative values  **/
                /*
                for(int k=0; k < 3; k++){
                    if((NEAR == 0) && (golomb_k==0) && (2*B[sample_context[k]] <= - N[sample_context[k]])){
                        if(Errval[k] >= 0) MErrval[k] = 2 * Errval[k] + 1;
                        else MErrval[k] = - 2 * (Errval[k] + 1);
                    }else{
                        if(Errval[k]>=0) MErrval[k] = 2 * Errval[k];
                        else MErrval[k] = (-2 * Errval[k]) -1; 
                    }
                }
                */

                /** Encode MErrval with Golomb**/
                /*
                    TODO
                */

                /** Update context variables **/ 
                /*
                for(int k=0; k<3; k++){
                    B[sample_context[k]] += Errval[k]*(2*NEAR+1);
                    A[sample_context[k]] += abs(Errval[k]);
                    
                    if(N[sample_context[k]] == RESET){
                        A[sample_context[k]] == A[sample_context[k]] << 1;
                        if(B[sample_context[k]] >= 0)
                            B[sample_context[k]] = B[sample_context[k]] >> 1;
                        else
                            B[sample_context[k]] = -((1-B[sample_context[k]]) >> 1);
                        N[sample_context[k]] = N[sample_context[k]] >> 1;
                    }
                    N[sample_context[k]] += 1;
                }
                */

                

                /** Bias update **/ 
                /*
                for(int k=0; k<3; k++){
                    if(B[sample_context[k]] <= -N[sample_context[k]]){
                        B[sample_context[k]] += N[sample_context[k]];
                        if(C[sample_context[k]] > MIN_C)
                            C[sample_context[k]] = C[sample_context[k]] - 1;
                        if(B[sample_context[k]] <= -N[sample_context[k]])
                            B[sample_context[k]] = -N[sample_context[k]] + 1; 
                    }else if(B[sample_context[k]] > 0){
                        B[sample_context[k]] = B[sample_context[k]] - N[sample_context[k]];
                        if(C[sample_context[k]] < MAX_C)
                            C[sample_context[k]] = C[sample_context[k]] + 1;
                        if(B[sample_context[k]] > 0)
                            B[sample_context[k]] = 0;
                    }
                }
                */

            }
    
        }

    }

    //namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    //imshow( "Display window", img );                   // Show our image inside it.

    // waitKey(0);   // Wait for a keystroke in the window
    return 0;
}