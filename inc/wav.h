#ifndef __WAV__
#define __WAV__

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "ff.h"

typedef struct WavHeaders {
    uint32_t ChunkID;
    uint32_t ChunkSize;
    uint32_t Format;
    uint32_t Subchunk1ID;
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    uint32_t Subchunk2ID;
    uint32_t Subchunk2Size;
    char* infoList[30];
    int infoListIdx;
} WavHeaders;

typedef enum WavResult {
    W_OK = 0,
    W_INVALID_PATH,
    W_INVALID_HEADERS,
    W_NOT_MONO,
    W_ERR_READING_DATA,
    W_MEM_ERR,
} WavResult;



WavResult verifyWavFile(FIL *file, WavHeaders* headers);

WavResult getWavData(FIL *file);
FRESULT playSDCardWavfile();
#endif
