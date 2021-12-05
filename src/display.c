#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ff.h"
#include "display.h"
#include "timer.h"
#include "dac.h"
#include "lcd.h"
#include "string.h"



int isWav(char* filename) {
  int len = strlen(filename);
  return filename[len - 4] == '.' && filename[len - 3] == 'w' && filename[len - 2] == 'a' && filename[len - 1] == 'v';
}

// Accepts a dir with an updated path, and updates all the filenames inside it
void updateFiles(Dir* dir)
{
    FRESULT res;
    DIR f_dir;
    FILINFO fno;

    // free previous filenames
    for (int i = 0; i < dir->numFiles; i++) {
      free(dir->fileNames[i]);
    }
    free(dir->fileNames);

    dir->numFiles = 0;
    dir->currSelection = 0;

    // this for loop counts up the number of files in the directory
    res = f_opendir(&f_dir, dir->path);
    for (;;) {
      res = f_readdir(&f_dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) {
        f_closedir(&f_dir);
        break;
      }
      // verify that it's either a directory or .wav file
      //if (!((fno.fattrib & AM_DIR) || isWav(fno.fname))) {
        //return;
      //}
      dir->numFiles++;
    }

    // close the directory, and allocate memory for the array of strings
    f_closedir(&f_dir);
    dir->fileNames = malloc(sizeof(char*) * dir->numFiles);

    // fill the filenames
    res = f_opendir(&f_dir, dir->path);
    for (int i = 0; ; i++) {
      res = f_readdir(&f_dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) {
        f_closedir(&f_dir);
        return;
      }
      dir->fileNames[i] = strdup(fno.fname);
    }
    f_closedir(&f_dir);
    
    return;
}

int getSelectedFile(Dir* dir, FILINFO* fno) {
  FRESULT res;
  DIR f_dir;
  res = f_opendir(&f_dir, dir->path);

  for (int i = 0; i < dir->currSelection + 1; i++) {
    res = f_readdir(&f_dir, fno);
    if (res != FR_OK || fno->fname[0] == 0) {
      f_closedir(&f_dir);
      return 1;
    }
  }

  return 0;
}

// this function adds current selection to path
void appendFilename(Dir* dir, char* selectedFilename) {
  // current path, '/', selectedFilename, '\0'
  int newPathLen = strlen(dir->path) + strlen(selectedFilename) + 2;
  char* newPath = malloc(newPathLen);

  // copy over the current path
  int i;
  char curr;
  for (;;) {
    curr = dir->path[i];
    if (curr == '\0') {
      break;
    }
    newPath[i] = curr;
    i++;
  }

  // add the slash after
  newPath[i] = '/';
  i++;

  // add the selectedFilename
  for (int j = 0; i + j < newPathLen; j++) {
    newPath[i + j] = selectedFilename[j];
  }
  free(dir->path);
  dir->path = newPath;
}

int handleFileSelectButton(Dir* dir) {
    FILINFO selectedFile;
    int res = getSelectedFile(dir, &selectedFile);
    if (res) {
        return res;
    }

    // is directory
    if (selectedFile.fattrib & AM_DIR) {
        appendFilename(dir, selectedFile.fname);
        updateFiles(dir);   
    } else {
        playSDCardWavfile(selectedFile.fname);
        return 1;
    }
    return 0;
}

void handleFileNextButton(Dir* dir) {
    if (dir->currSelection == dir->numFiles) {
        dir->currSelection = 0;
    } else {
        dir->currSelection++;
    }
}


