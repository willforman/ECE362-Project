#ifndef __SDCARD__
#define __SDCARD__

FRESULT openSDCardFile(FATFS *FatFs, FIL *fil);

FRESULT closeSDCardFile(FATFS *FatFs, FIL *fil);

FRESULT printSDCardTextFile();

#endif