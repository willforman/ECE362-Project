#include "ff.h"
#include "sdcard.h"
#include "timer.h"

FATFS FatFs;
int main() {
    if (init_sdcard(&FatFs)) {
        return 0;
    }

    initDisplay(2000);
    initButtonScanning(1);

    enableDisplay();
    enableButtonScanning();

    for (;;) {
        asm("wfi");
    }

    // Unmount SD card
    return 0;
}
