#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "tty.h"
#include "commands.h"
#include "sdcard.h"
#include "wav.h"
#include "stm.h"
#include <stdio.h>


FATFS FatFs;
int main() {
    printf("hello world\n");

    init_sdcard(&FatFs);
    playSDCardWavfile();

     // Unmount SD card
    return 0;
}
