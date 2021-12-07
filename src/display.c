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

DIR f_dir;
extern FATFS FatFs;
int isWav(char* filename) {
  int len = strlen(filename);
  return filename[len - 4] == '.' && filename[len - 3] == 'w' && filename[len - 2] == 'a' && filename[len - 1] == 'v';
}

// Accepts a dir with an updated path, and updates all the filenames inside it
FRESULT updateFiles(Dir* dir, const TCHAR * dest) {
    FRESULT res;
    FILINFO fno;

    dir->numFiles = 0;
    dir->currSelection = 0;

    res = f_chdir(dest);
    res = f_opendir(&f_dir, "");
    if (res) {
        return res;
    }

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
  return 0;
}

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


