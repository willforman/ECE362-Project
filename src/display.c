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
char fileNameShort[29];
extern int playingSong;
extern WavHeaders headers;

// Printing constants
u16 x1 = 0x10;
u16 y = 0x00;
u16 fc1 = 0xffff; // Foreground color (0xffff = white) (text color)
u8 size1 = 16;
u8 mode1 = 0; // Transparent background = 1
u16 bc1 = 0x0000;

char* playingSongName;
int playingSongNameLen;
char playingSongNameShort[29];

char bitsPerSampleStr[20];
char sampleRateStr[19];
char numChannelsStr[] = "Number of Channels: 1";
char progBarStr[] = "[                        ]";

extern int dataIdx;
int numBars = 25;
int barIdx = 1;
int progDiv;
int progCurr;

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
    dir->fileNames = malloc(sizeof(char*) * (dir->numFiles + 1));

    // fill the filenames
    res = f_opendir(&f_dir, "");
    if (res) {
       // f_mount(0, "", 1);
        return res;
    }
    int i;
    for (i = 0; i < dir->numFiles; i++) {
      res = f_readdir(&f_dir, &fno);
      dir->fileNames[i] = strdup(fno.fname);
    }
    
    dir->fileNames[i] = "..";
    dir->numFiles++;

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

    if (dir->currSelection == dir->numFiles - 1) {
        *selectedWav = 0;
        res = updateFiles(dir, "..");
        LCD_Clear(0);
        if (res) {
            return res;
        }
        return 0;
    }

    res = getSelectedFile(dir, &selectedFile);
    if (res) {
        return res;
    }

    // is directory
    if (selectedFile.fattrib & AM_DIR) {
        *selectedWav = 0;
        res = updateFiles(dir, selectedFile.fname);
        LCD_Clear(0);
        if (res) {
            return res;
        }
    } else {
        *selectedWav = 1;
        playingSongName = selectedFile.fname;
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

void initPlayingDisplay() {
    sprintf(bitsPerSampleStr, "Bits Per Sample: %d", headers.BitsPerSample);
    sprintf(sampleRateStr, "Sample Rate: %d", headers.SampleRate);

    progDiv = headers.Subchunk2Size / numBars;
    progCurr = progDiv;

    playingSongNameLen = strlen(playingSongName);
    playingSongNameShort[28] = '\0';
}

void updatePlayingDisplay(Dir* dir) {
    // file name, header
    if (dataIdx > progCurr) {
        do {
            progBarStr[barIdx] = '=';
            barIdx++;
            progCurr += progDiv;
        } while (dataIdx > progCurr);
        LCD_DrawString(x1, y + 0x110, fc1, bc1, progBarStr, size1, mode1);
    }

    if (playingSongNameLen > 28) {
        char temp = playingSongName[0];
        for(int j = 0; j < playingSongNameLen - 1; j++) {
            playingSongName[j] = playingSongName[j + 1];
        }
        playingSongName[playingSongNameLen - 1] = temp;
        strncpy(playingSongNameShort, playingSongName, 28);
        LCD_DrawString(x1, y, fc1, bc1, playingSongNameShort, size1, mode1);
    } else {
        LCD_DrawString(x1, y, fc1, bc1, playingSongName, size1, mode1);
    }

    LCD_DrawString(x1, y + 0x10, fc1, bc1, "==========================", size1, mode1);
    LCD_DrawString(x1, y + 0x30, fc1, bc1, bitsPerSampleStr, size1, mode1);
    LCD_DrawString(x1, y + 0x50, fc1, bc1, sampleRateStr, size1, mode1);
    LCD_DrawString(x1, y + 0x70, fc1, bc1, numChannelsStr, size1, mode1);
}


// Display current files in current path
void updateFilesDisplay(Dir* dir)
{
    int sel = dir -> currSelection;
    /*if (sel % 10 == 0)
    {
        LCD_Clear(0);
    }*/
    int offsetY = 0x00;


    for(int i = 0; i < dir -> numFiles; i++)
    {
        // choose the background color of selected line only
        u16 bc1 = sel == i ? 0x7BEF : 0x0000;

        int fileNameLen = strlen(dir -> fileNames[i]);
        char* fileName = dir -> fileNames[i];

        strncpy(fileNameShort, fileName, 28);

        if (i >= sel - (sel % 10) && i < sel + 10 - (sel % 10))
        {
            LCD_DrawString(x1, y + offsetY, fc1, bc1, fileNameShort, size1, mode1);
            offsetY += 0x20;

            if (fileNameLen > 28)
            {
                // Scroll filename by one character
                char temp = fileName[0];
                for(int j = 0; j < fileNameLen - 1; j++)
                {
                    fileName[j] = fileName[j + 1];
                }
                fileName[fileNameLen - 1] = temp;
            }
        }
    }
}

