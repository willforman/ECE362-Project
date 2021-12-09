#include "ff.h"
#include "timer.h"
#include "internal_clock.h"
FATFS FatFs;
int main() {
    FRESULT res;
    internal_clock(); // for the pcb
    res = f_mount(&FatFs, "", 0);
    if (res) {
        return res;
    }
    initDisplay(20000);
    initButtonScanning(1);

    res = enableDisplay();
    if (res) {
         return res;
    }

    enableButtonScanning();

    // Unmount SD card
    return 0;
}
