#ifndef LBG_H
#define LBG_H

#include <iostream>
#include <vector>
#include <random>
#include <numeric> 
#include <stdlib.h>   
#include "binarystream.h"

struct
{
	unsigned int elSize;
	unsigned int blockSize;
	unsigned int codebookSize;
} CB_METADATA;

// not a great name
class LBG{

  private:
	std::vector<std::vector<short>> training_vectors;
	std::vector<std::vector<short>> codebook;
	std::vector<unsigned int> clusterAssignments;
	std::vector<int> assignmentSum;

	unsigned int codebookSize;
	unsigned int blockSize;

	double calculateDistortion(){
		double distortion = 0;
		for(size_t i=0; i < training_vectors.size(); ++i){
			distortion += ldistance(training_vectors[i], codebook[clusterAssignments[i]]);
		}
		return distortion;
	}
	

  public:
	LBG( unsigned int codebookSize, unsigned int blockSize): 
		codebookSize(codebookSize), blockSize(blockSize)
	{
		//reserve vars
		codebook = std::vector<std::vector<short>>(codebookSize, std::vector<short>(blockSize));

		//assignments for each centroid
		assignmentSum.reserve(blockSize);

	}

	LBG(std::vector<std::vector<short>>& codebook, unsigned int codebookSize, unsigned int blockSize): 
		codebook(codebook), codebookSize(codebookSize), blockSize(blockSize)
	{	
		//assignments for each centroid
		assignmentSum.reserve(blockSize);
	}

	// rest of samples that don't complete a full block are discarded 
	void updateTrainingVectors(const std::vector<short>& samples, int overlapFactor) {
		std::vector<short> block(blockSize);
		size_t n { };
		for(auto s : samples){
			block[n++ % blockSize] = s;
			if(n % (blockSize-overlapFactor) == 0 && n > blockSize)
				training_vectors.push_back(block);
		}
	}
	
	void chooseInitialRandomCentroids(){
		
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 eng(rd()); // seed the generator
		std::uniform_int_distribution<> distr(0, training_vectors.size()-1); // define the range
		for(unsigned int i=0; i<codebookSize; ++i){
			codebook[i] = training_vectors[distr(eng)];
		}
	}

	// runs LBG on specified training vectors
	std::vector<double> runLBG(double threshold, unsigned int max_iter, bool print_distortion = false){
		// D0 = 0
		unsigned int n = 0;
		double distortion;
			
		clusterAssign(clusterAssignments);			
		std::vector<double> d(1,0);

		while(1){
			n++;
			moveCentroids();
			clusterAssign(clusterAssignments);			
			distortion = calculateDistortion();
			d.push_back(distortion);		
			
			if(print_distortion){
				std::cout << "n: " << n << "\tdistortion: " << distortion << std::endl;
			}
			if(abs(d[n-1] - d[n]) < threshold || n >= max_iter){
				break;
			}		
		}
		return d;
	}

	std::vector<std::vector<short>>& getCodebook(){
		return codebook;
	}

	void clusterAssign(std::vector<unsigned int>& clusterAssignments){
		
		clusterAssignments.resize(training_vectors.size());
		//find closest vector
		for(size_t i=0; i < training_vectors.size(); ++i){
			
			unsigned int minCentroid = 0;
			auto assign_vector = training_vectors[i];
			double minDistance = ldistance(assign_vector, codebook[0]);

			for(size_t centroid=1; centroid < codebookSize; ++centroid){
				double distance = ldistance(assign_vector, codebook[centroid]);
				if(distance < minDistance){
					minDistance = distance;
					minCentroid = centroid;
				}
			}
			clusterAssignments[i] = minCentroid;
		}
	}

	void moveCentroids(){
		double assignments;

		std::fill(assignmentSum.begin(), assignmentSum.end(),0);

		for(size_t centroid=0; centroid < codebookSize; ++centroid){
			assignments = 0;
			//sum points assigned to centroid
			for(size_t i=0; i < training_vectors.size(); ++i){

				if(clusterAssignments[i] == centroid){
					assignments++;
					
					for(size_t idx=0; idx < blockSize; ++idx)
						assignmentSum[idx] += training_vectors[i][idx];
				}

			}
			if(assignments){
				for(size_t j=0; j < blockSize; j++){
					codebook[centroid][j] = assignmentSum[j] / assignments;
				}
			}
		}
		
	}

	// default is euclidean dist
	double ldistance(std::vector<short> block1, std::vector<short> block2, int p = 2){
		double sum = 0;
		for(size_t i = 0; i < block1.size(); ++i){
			sum+=std::pow(block1[i] - block2[i], p);
		}
		return std::pow(sum, 1.0/p);
	}

	void writeCodebook(std::string codebookName){
		CB_METADATA.elSize = sizeof(short);
		CB_METADATA.blockSize = blockSize;
		CB_METADATA.codebookSize = codebookSize;

		BinaryStream ostream { codebookName, std::fstream::out };
		ostream.write((char *)&CB_METADATA, sizeof(CB_METADATA));
		for(unsigned int i=0; i < codebookSize; ++i){
 			for(unsigned int j=0; j < blockSize; ++j){
 				ostream.writebits(codebook[i][j], sizeof(short)*8);
			}
		}
	}

};

#endif

