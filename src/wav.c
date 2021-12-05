#include "wav.h"
#include "sdcard.h"
#include "ff.h"
#include "DAC.h"

uint16_t* array;
WavHeaders headers;
WavHeaders * headerAdd = &headers;
FIL fil;
extern FATFS FatFs;
uint32_t be(uint32_t num) {
    int ans = ((num >> 24) & 0xff)
            | ((num << 8) & 0xff0000 )
            | ((num >> 8) & 0xff00)
            | ((num << 24) & 0xff000000);
    return ans;
}

WavResult verifyWavFile(FIL *file, WavHeaders *headers) {
    FRESULT res;
    UINT bytesRead;

    res = f_read(file, headers, sizeof *headers, &bytesRead); // fills out each attribute of WavHeaders

    // if the read resulted in an error
    if (res) {
        return W_INVALID_PATH;
    }

    if (bytesRead == 0) {
        return W_INVALID_HEADERS;
    }

    // Make sure all the headers that must equal a certain value are valid
    int notValid = (
        (headers->ChunkID != be(0x52494646)) || // ChunkID must equal "RIFF"
        (headers->Format != be(0x57415645)) || // Format must equal "WAVE"
        (headers->Subchunk1ID != be(0x666d7420)) // Subchunk1ID must equal "fmt "
    );

    if (notValid) {
        return W_INVALID_HEADERS;
    }

    if (headers->NumChannels != 1) {
        return W_NOT_MONO;
    }

    // Some .wav files may include extra params in the fmt block.
    // We need to advance the Subchunk2ID past these
    while (headers->Subchunk2ID != be(0x64617461)) {
        f_lseek(file, 4);
        f_read(file, &(headers->Subchunk2ID), 4, &bytesRead);
    }
    f_read(file, &(headers->Subchunk2Size), 4, &bytesRead);

    return W_OK;
}

FRESULT playSDCardWavfile(char* filename) {
    FRESULT res;
    res = openSDCardFile(&FatFs, &fil, filename);

    if (res) {
        return res;
    }

    res = verifyWavFile(&fil, &headers);

    if (res) {
        return 1;
    }

    return play();
}
