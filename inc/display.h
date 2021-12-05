#ifndef __DISPLAY__
#define __DISPLAY__

typedef struct Dir {
   char* path;
   char** fileNames;
   int currSelection;
   int numFiles;
} Dir;

void handleFileNextButton(Dir* dir);

int handleFileSelectButton(Dir* dir);

void scrollDisplay(Dir* dir);

#endif
