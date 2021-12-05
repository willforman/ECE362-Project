#ifndef __SDCARD__
#define __SDCARD__

FRESULT init_sdcard(FATFS* FatFs);

FRESULT openSDCardFile(FATFS *FatFs, FIL *fil, char* filename);

FRESULT closeSDCardFile(FATFS *FatFs, FIL *fil);

#endif