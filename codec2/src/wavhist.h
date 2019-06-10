#ifndef WAVHIST_H
#define WAVHIST_H

#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <sndfile.hh>
#include <math.h>


class WAVHist {
  private:
	std::vector<std::map<int, size_t>> counts;
	std::map<int, size_t> mono;


  public:
	WAVHist(int nChannels) {
		counts.resize(nChannels);
	}

	void update(const std::vector<int>& samples) {
		size_t n { };
		for(auto s : samples){
			
			counts[n % counts.size()][s]++;

			int sample_buffer[counts.size()] { };			
			// save the sample for each channel temporarily 
			sample_buffer[n % counts.size()] = s;
			
			if(++n % counts.size() == 0){
				int sum = 0;
				for(auto sample : sample_buffer)
					sum += sample;
				int average = round((float)sum / counts.size());
				mono[average]++;
			}
		}

	}
	// channel == -1 for mono 
	void dump(const int channel, std::string filename) const {
		std::ofstream ofile;
		std::replace(filename.begin(), filename.end(), '/', '_');
  		ofile.open(filename);
		
		std::map<int, size_t> hist_map;

		hist_map = (channel >= 0) ? counts[channel] : mono;

		for(auto [value, counter] : hist_map){
			ofile << value << '\t' << counter << '\n';
		}

		ofile.close();
	}
	
};

#endif

