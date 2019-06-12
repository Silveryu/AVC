#ifndef METADATA_H
#define METADATA_H


struct video_metadata : metadata
{
	std::string header;
};
struct block_metadata : metadata
{
	unsigned int m;
};
struct encoder_metadata : metadata
{
	int blockSize;
	int searchArea;
	int periodicity;
};
struct lossy_metadata : metadata
{
	int c1_quant;
	int c2_quant;
	int c3_quant;
};

#endif