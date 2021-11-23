#ifndef __WAV__
#define __WAV__

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "ff.h"

typedef struct WavHeaders {
    uint_32_t ChunkID;
    uint_32_t ChuckSize;
    uint_32_t Format;
    uint_32_t Subchunk1ID;
    uint_32_t Subchunk1Size;
    uint_16_t AudioFormat;
    uint_16_t NumChannels;
    uint_32_t SampleRate;
    uint_32_t ByteRate;
    uint_16_t BlockAlign;
    uint_16_t BitsPerSample;
    uint_32_t Subchunk2ID;
    uint_32_t Subchunk2Size;
} WavHeaders;

typedef enum WavResult {
    W_OK = 0,
    W_INVALID_PATH,
    W_INVALID_HEADERS,
    W_ERR_READING_DATA,
    W_MEM_ERR,
} WavResult;

WavResult verifyWavFile(FIL *file, WavHeaders* headers);

WavResult getWavData(FIL *file, WavHeaders* headers);

#endif