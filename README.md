# AVC
Three codecs develped for Audio Video Coding Classes

* Codec 1

Audio codec capable of uniform scalar quantization and vector quantization, the vector quantization codebook is derived 
through the [Linde–Buzo–Gray algorithm](https://en.wikipedia.org/wiki/Linde%E2%80%93Buzo%E2%80%93Gray_algorithm).

* Codec 2

Audio codec with support for Entropy coding, specifically [Golomb coding](https://en.wikipedia.org/wiki/Golomb_coding). Explores both temporal and channel redundancy, with various temporal predictors available. Lossy compression is also available through residual quantization.

* Codec 3

Video codec with support for lossless and lossy intra-frame and inter-frame predictive coding, intra-frame coding is based on the [JPEG-LS predictor](https://en.wikipedia.org/wiki/Lossless_JPEG) 
and inter-frame on Block [Motion Compensation](https://en.wikipedia.org/wiki/Motion_compensation).

## Usage

Usage for each codec, along with their Signal-to-Noise Ratio, PSNR and timing benchmarks can be found in their respective report file.

