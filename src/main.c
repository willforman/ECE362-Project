#include "ff.h"
#include "wav.h"

FRESULT play() {
    FATFS FatFs;
    FRESULT fr;
    WavHeaders headers;
    WavResult wr;
    FIL file;

    fr = openSDCardFile(&FatFs, &file);

    if (fr) {
        return fr;
    }
    
    wr = verifyWavFile(&file, &headers);
    uint8_t* data;
    getWavData(&file, &headers);

    return closeSDCardFile(&FatFs, &file);
}

int main() {
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    command_shell();
}