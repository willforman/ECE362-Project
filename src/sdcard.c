#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "commands.h"
#include "sdcard.h"
#include "stdio.h"
#include "wav.h"

FRESULT openSDCardFile(FATFS *FatFs, FIL *fil) {
    // Open file, and return it's result code
    return f_open(fil, "kanye.wav", FA_READ);
}

FRESULT closeSDCardFile(FATFS *FatFs, FIL *fil) {
    return f_close(fil); // Close file
}

FRESULT printSDCardTextFile() {
    FATFS FatFs;
    FRESULT res;
    FIL fil;
    char line[100];
    //uint32_t total, free;

    res = openSDCardFile(&FatFs, &fil);

    if (res) {
        return res;
    }

    while (f_gets(line, sizeof line, &fil)) {
        printf(line);
    }

    return closeSDCardFile(&FatFs, &fil);
}
