#ifndef WAVHIST_H
#define WAVHIST_H

#include <iostream>
#include <vector>
#include <map>
#include <sndfile.hh>
#include <math.h>


class WAVHist {
  private:
	std::vector<std::map<short, size_t>> counts;
	std::map<short, size_t> mono;


  public:
	WAVHist(const SndfileHandle& sfh) {
		counts.resize(sfh.channels());
	}

	void update(const std::vector<short>& samples) {
		size_t n { };
		for(auto s : samples){
			
			counts[n % counts.size()][s]++;

			short sample_buffer[counts.size()] { };			
			// save the sample for each channel temporarily 
			sample_buffer[n % counts.size()] = s;
			
			if(++n % counts.size() == 0){
				int sum = 0;
				for(auto sample : sample_buffer)
					sum += sample;
				short average = round((float)sum / counts.size());
				mono[average]++;
			}
		}

	}
	// channel == -1 for mono 
	void dump(const int channel) const {
	
		std::map<short, size_t> hist_map;

		hist_map = (channel >= 0) ? counts[channel] : mono;

		for(auto [value, counter] : hist_map){
			std::cout << value << '\t' << counter << '\n';
		}
				
	}
	
};

#endif

