#ifndef __TIMER__
#define __TIMER__

void initDisplay(int calledEveryMs);
FRESULT enableDisplay();
void disableDisplay();

void initButtonScanning(int calledEveryMs);
void enableButtonScanning();
void disableButtonScanning();

#endif
