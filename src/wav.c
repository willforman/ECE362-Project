#include "wav.h"
#include "sdcard.h"
#include "ff.h"
#include "DAC.h"
#include "display.h"
#include <stdlib.h>
#include <string.h>

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

    res = f_read(file, headers, sizeof *headers - (12 + sizeof(headers->infoList)), &bytesRead); // fills out each attribute of WavHeaders

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

    uint32_t id;
    f_read(file, &id, sizeof(id), &bytesRead);

    // List chunk
    headers->infoListIdx = 0;
    if (id == 0x5453494c) {
        uint32_t listSize;
        f_read(file, &listSize, sizeof(listSize), &bytesRead);

        uint32_t typeId;
        f_read(file, &typeId, sizeof(typeId), &bytesRead);

        if (typeId != 0x4f464e49) {
            return W_INVALID_HEADERS;
        }



        while (1) {
            uint32_t label;
            f_read(file, &label, 4, &bytesRead);

            if (label == 0x61746164) {
                break;
            }

            uint8_t size;
            f_read(file, &size, sizeof(size), &bytesRead);
            f_lseek(file, f_tell(file) + 3);

            int strSize = 6 + size + 1; // "IART: Mathematics and Nature\0"
            char* str = malloc(strSize);

            strcpy(str, (char*) &label);
            str[4] = ':';
            str[5] = ' ';
            str[strSize - 1] = '\0';

            f_read(file, str + 6, size, &bytesRead);

            for (int i = 6; i < strSize - 1; i++) {
                if (str[i] < 20) {
                    str[i] = ' ';
                }
            }

            headers->infoList[headers->infoListIdx] = str;
            headers->infoListIdx++;

            uint8_t advance;
            do {
                f_read(file, &advance, 1, &bytesRead);
            } while (advance == 0);
            f_lseek(file, f_tell(file) - 1);
        }
    }
    f_read(file, &(headers->Subchunk2Size), 4, &bytesRead);

    return W_OK;
}

const char *w_errs[] = {
        [W_OK] = "Success",
        [W_INVALID_PATH] = "Invalid path",
        [W_INVALID_HEADERS] = "Invalid headers",
        [W_NOT_MONO] = "File is not Mono",
};

FRESULT playSDCardWavfile(char* filename) {
    FRESULT res;

    res = openSDCardFile(&FatFs, &fil, filename);

    if (res) {
        return res;
    }
    WavResult r;
    r = verifyWavFile(&fil, &headers);

    if (r) {
        printError(w_errs[r]);
        return 25;
    }

    return play();
}
