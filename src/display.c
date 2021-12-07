#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ff.h"
#include "display.h"
#include "timer.h"
#include "dac.h"
#include "lcd.h"
#include "string.h"
#include "wav.h"


extern FATFS FatFs;
int isWav(char* filename) {
  int len = strlen(filename);
  return filename[len - 4] == '.' && filename[len - 3] == 'w' && filename[len - 2] == 'a' && filename[len - 1] == 'v';
}

// Accepts a dir with an updated path, and updates all the filenames inside it
FRESULT updateFiles(Dir* dir, const TCHAR * dest) {
    FRESULT res;
    DIR f_dir;
    FILINFO fno;

    // free previous filenames
    for (int i = 0; i < dir->numFiles; i++) {
      free(dir->fileNames[i]);
    }

    dir->numFiles = 0;
    dir->currSelection = 0;



    // this for loop counts up the number of files in the directory
    //res = f_mount(&FatFs, "", 0);
//    if (res) {
//        return res;
//    }
    res = f_chdir(dest);
    res = f_opendir(&f_dir, "");
    if (res) {
        //f_mount(0, "", 1);
        return res;
    }

    // change into directory


//    if (res) {
//        f_closedir(&f_dir);
//        //f_mount(0, "", 1);
//        return res;
//    }

    for (;;) {
      res = f_readdir(&f_dir, &fno);
      if (res != FR_OK) {
          f_closedir(&f_dir);
          //f_mount(0, "", 1);
          return res;
      }
      if (fno.fname[0] == 0) {
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
    res = f_opendir(&f_dir, "");
    if (res) {
       // f_mount(0, "", 1);
        return res;
    }
    for (int i = 0; i < dir->numFiles; i++) {
      res = f_readdir(&f_dir, &fno);
      dir->fileNames[i] = strdup(fno.fname);
    }
    
    f_closedir(&f_dir);
    //f_mount(0, "", 1);
    return 0;
}

FRESULT getSelectedFile(Dir* dir, FILINFO* fno) {
  FRESULT res;
  DIR f_dir;
//  res = f_mount(&FatFs, "", 0);
//  if (res) {
//      return res;
//  }
  res = f_opendir(&f_dir, "");
  if (res) {
      //f_mount(0, "", 1);
      return res;
  }

  for (int i = 0; i < dir->currSelection + 1; i++) {
    res = f_readdir(&f_dir, fno);
    if (res != FR_OK || fno->fname[0] == 0) {
      f_closedir(&f_dir);
      //f_mount(0, "", 1);
      return res;
    }
  }

  f_closedir(&f_dir);
  //f_mount(0, "", 1);
  return 0;
}
/*
// this function adds current selection to path
void appendFilename(Dir* dir, char* selectedFilename) {
  // current path, '/', selectedFilename, '\0'
  int newPathLen = strlen(dir->path) + strlen(selectedFilename) + 2; //     /william/\0
  char* newPath = malloc(newPathLen);

  // copy over the current path

  char curr = dir->path[0];

  int i;
  for (i = 0; curr != '\0'; i++) {
    newPath[i] = curr;
    curr = dir->path[i + 1];
  }

  // add the slash after
  newPath[i] = '/';
  i++;

  // add the selectedFilename
  int j;
  for (j = 0; i + j < newPathLen - 1; j++) {
    newPath[i + j] = selectedFilename[j];
  }

  newPath[i + j] = '\0';

  free(dir->path);
  dir->path = newPath;
  int strlenNew = strlen(newPath);
  for (int k = 0; k<strlenNew;k++){
      char c = newPath[k];
      int kjkh  =0;
  }

}
*/
FRESULT handleFileSelectButton(Dir* dir, int* selectedWav) {
    FILINFO selectedFile;
    FRESULT res;

    res = getSelectedFile(dir, &selectedFile);
    if (res) {
        return res;
    }

    // is directory
    if (selectedFile.fattrib & AM_DIR) {
        *selectedWav = 0;
        //appendFilename(dir, selectedFile.fname);
        res = updateFiles(dir, selectedFile.fname);
        if (res) {
            return res;
        }
    } else {
        *selectedWav = 1;
        playSDCardWavfile(selectedFile.fname);
    }
    return 0;
}

void handleFileNextButton(Dir* dir) {
    if (dir->currSelection == dir->numFiles - 1) {
        dir->currSelection = 0;
    } else {
        dir->currSelection++;
    }
}


