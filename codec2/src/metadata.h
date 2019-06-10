#ifndef METADATA_H
#define METADATA_H

#include "bitstream.h"
#include "predictors.h"

struct soundfile_metadata : metadata
{
	int format;
	int frames;
	int channels;
	int samplerate;
	unsigned int m;
	unsigned int quantsize;
	unsigned int FRAMES_BUFFER_SIZE = 65536; // Buffer for reading/writing frames
	PredictorType type;
};

struct block_metadata : metadata
{
	unsigned int m;
};

#endif