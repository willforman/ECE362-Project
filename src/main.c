#include "ff.h"
#include "timer.h"

FATFS FatFs;
int main() {
    FRESULT res;
    res = f_mount(&FatFs, "", 0);
    if (res) {
        return res;
    }
    initDisplay(2000000);
    initButtonScanning(1);

    res = enableDisplay();
    if (res) {
         return res;
    }
    enableButtonScanning();

    /*for (;;) {
        asm("wfi");
    }*/

    // Unmount SD card
    return 0;
}
