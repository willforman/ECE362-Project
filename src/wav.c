#include "wav.h"
#include "sdcard.h"
#include "ff.h"

// Two approaches:
// 


uint32_t be(uint32_t num) {
    return ((num >> 24) & 0xff) 
            | ((num << 8) & 0xff0000 )
            | ((num >> 8) & 0xff00)
            | ((num << 24) & 0xff000000);
}

WavResult verifyWavFile(FIL *file, WavHeaders *headers) {
    FRESULT res;
    int bytesRead;

    res = f_read(file, headers, sizeof *headers, &bytesRead);

    // if the read resulted in an error
    if (res) {
        return W_INVALID_PATH;
    }

    if (bytesRead == 0) {
        return W_INVALID_HEADERS;
    }

    // Make sure all the headers that must equal a certain value are valid
    int notValid = (
        headers->ChunkID != be(0x52494646) | // ChunkID must equal "RIFF"
        headers->Format != be(0x57415645) | // Format must equal "WAVE"
        headers->Subchunk1ID != be(0x64617461) // Subchunk1ID must equal fmt
    );

    if (notValid) {
        return W_INVALID_HEADERS;
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

WavResult getWavData(FIL *file, WavHeaders *headers, void* data) {
    int bytesRead;
    data = malloc(8 * headers->Subchunk2Size);

    if (data == NULL) {
        return W_MEM_ERR;
    }
    
    f_read(file, data, headers->BitsPerSample, &bytesRead);

    if (bytesRead != headers->Subchunk2Size) {
        free(data);
        fprintf(stderr, "Data read: expected=%d, actual=%d\n", headers->Subchunk2Size, bytesRead);
        return W_ERR_READING_DATA;
    }

    return W_OK;
}