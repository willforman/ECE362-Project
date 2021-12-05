#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "stdio.h"
#include "wav.h"
#include <string.h>

FRESULT init_sdcard(FATFS* FatFs) {
    return f_mount(FatFs, "", 0); // Mount SD card
}

FRESULT openSDCardFile(FATFS *FatFs, FIL *fil, char* filename) {
    // Open file, and return it's result code
    return f_open(fil, filename, FA_READ);
}

FRESULT closeSDCardFile(FATFS *FatFs, FIL *fil) {
    return f_close(fil); // Close file
}

