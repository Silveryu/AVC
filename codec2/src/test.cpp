#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.hh>
#include <algorithm>
#include <array>
#include "bitstream.h"
#include "golomb.h"
#include <bitset>

using namespace std;


int main() {
	
	Golomb gin = Golomb("sample.bin", std::fstream::in);

	soundfile_metadata sf_meta;
	gin.readSoundFileMetadata(sf_meta);
	
	vector<short> samples(sf_meta.FRAMES_BUFFER_SIZE * sf_meta.channels);
	
	size_t nFullWrites = sf_meta.frames / sf_meta.FRAMES_BUFFER_SIZE;
    size_t finalWriteFrames = sf_meta.frames % sf_meta.FRAMES_BUFFER_SIZE;
    cout << "finalWrite: " << finalWriteFrames << endl;
	cout << "nFull: " << nFullWrites << endl;
	for(size_t nWrite=0; nWrite <= nFullWrites; nWrite++){   

		block_metadata b_meta;
        
		gin.readBlockMetadata(b_meta);
        
		cout << "m:" <<  b_meta.m << endl;

		if(nWrite == nFullWrites)
			samples.resize(finalWriteFrames * sf_meta.channels);
		else		
			samples.resize(sf_meta.FRAMES_BUFFER_SIZE * sf_meta.channels);

		
		gin.setM(b_meta.m);
		cout << "samples size: "<< samples.size() << endl;
		for(size_t i = 0; i < samples.size(); i++)
		gin.decode();
		 //cout << "nW: " << nWrite << " samples[" << i << "] = " << gin.decode() << endl;
		
	}




/* 	cout << "size: " << samples.size() << endl;
	gin.decodeN(samples, samples.size(), b_meta.m);

	for(int i= 0; i< 100; i++){
		cout << "samples["<<i<<"] = " << samples[i]<< endl;
	}
 */

	// for(int i = 0; i < 1; ++i){	
	// }

	// BitStream ostreamwbits {"testfilewbits", fstream::out};
	// BitStream istreamwbits {"testfilewbits", fstream::in};
	
	// // ostreamwbits.close();
	// uint64_t tst = 0xDEF;

	// for(int i = 0; i < 12; ++i){
	// 	ostreamwbits.writebit((tst&0x800) >> 11);
	// 	tst = tst << 1;
	// }
	// ostreamwbits.close();

	// uint64_t val = 0;
	// while(istreamwbits.readbits(val, 12)){
	// 	std::cout << std::bitset<15>(val) << std::endl;
	// }


	// uint8_t bit;
	// while(istreamwbits.readbit(bit)){
	// 	std::cout << (bit ? "1" : "0") << std::endl;
	// }

	// printf("\n%x", val);



	// ostreamwbits.close();

	// int64_t val = 0;
	// while(istreamwbits.readbits(val,30)){
	// 	std::cout << "val: " << val << std::endl;
	// }


	return 0;
}