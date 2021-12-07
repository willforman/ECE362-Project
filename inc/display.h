#ifndef __DISPLAY__
#define __DISPLAY__

#include "ff.h"

typedef struct Dir {
   //char* path;
   char** fileNames;
   int currSelection;
   int numFiles;
} Dir;

void handleFileNextButton(Dir* dir);

FRESULT handleFileSelectButton(Dir* dir, int* selectedWav);

void scrollDisplay(Dir* dir);

FRESULT updateFiles(Dir* dir, const TCHAR *);


#endif
