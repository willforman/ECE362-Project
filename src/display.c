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

char bitsPerSampleStr[20];
char sampleRateStr[19];
char numChannelsStr[] = "Number of Channels: 1";
char progBarStr[] = "[                        ]";

extern int dataIdx;
int numBars = 25;
int barIdx;
int progDiv;
int progCurr;

void printError(char* error) {
    enableErrorMode();
    LCD_DrawString(x1, y, fc1, bc1, error, size1, mode1);
    LCD_DrawString(x1, 0x20, fc1, bc1, "Press right button to return", size1, mode1);
}

void printEndError(FRESULT res) {
    const char *f_errs[] = {
            [FR_OK] = "Success",
            [FR_DISK_ERR] = "Hard err in disk I/O layer",
            [FR_INT_ERR] = "Assertion failed",
            [FR_NOT_READY] = "Physical drive cannot work",
            [FR_NO_FILE] = "File not found",
            [FR_NO_PATH] = "Path not found",
            [FR_INVALID_NAME] = "Path name format invalid",
            [FR_DENIED] = "Permission denied",
            [FR_EXIST] = "Prohibited access",
            [FR_INVALID_OBJECT] = "File or dir object invalid",
            [FR_WRITE_PROTECTED] = "Phys drive is write-protected",
            [FR_INVALID_DRIVE] = "Logical drive num is invalid",
            [FR_NOT_ENABLED] = "Volume has no work area",
            [FR_NO_FILESYSTEM] = "Not a valid FAT volume",
            [FR_MKFS_ABORTED] = "f_mkfs aborted",
            [FR_TIMEOUT] = "Unable to obtain grant for object",
            [FR_LOCKED] = "File locked",
            [FR_NOT_ENOUGH_CORE] = "File name is too large",
            [FR_TOO_MANY_OPEN_FILES] = "Too many open files",
            [FR_INVALID_PARAMETER] = "Either err, or root is empty",
    };

    LCD_DrawString(x1, 0, fc1, bc1, f_errs[res], size1, mode1);
    LCD_DrawString(x1, 0x20, fc1, bc1, "==========================", size1, mode1);
    LCD_DrawString(x1, 0x40, fc1, bc1, "Remove SD card and reinsert,", size1, mode1);
    LCD_DrawString(x1, 0x60, fc1, bc1, "then reset with left button.", size1, mode1);
}

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
    
    if (dir->numFiles == 0) {
        return FR_INVALID_PARAMETER;
    }

    dir->fileNames[i] = "..";
    dir->numFiles++;

    f_closedir(&f_dir);
    return 0;
}

FRESULT getSelectedFile(Dir* dir, FILINFO* fno) {
  FRESULT res;


  res = f_opendir(&f_dir, "");
  if (res) {
      return res;
  }

  for (int i = 0; i < dir->currSelection + 1; i++) {
    res = f_readdir(&f_dir, fno);
    if (res != FR_OK || fno->fname[0] == 0) {
      f_closedir(&f_dir);
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
            disableDisplay();
            disableButtonScanning();
            printEndError(res);
            return res;
        }
    } else {
        *selectedWav = 1;
        if (playingSongName[0] != '\0') {
            free(playingSongName);
        }
        playingSongName = strdup(selectedFile.fname);
        res = playSDCardWavfile(selectedFile.fname);
        if (res) {
            if (res == 25) {
                return res;
            }
            return res;
        }
    }
    return 0;
}

void handleFileNextButton(Dir* dir) {
    if (dir->currSelection == dir->numFiles - 1) {
        dir->currSelection = 0;
        LCD_Clear(0);
    } else {
        dir->currSelection++;
        if (dir->currSelection % 10 == 0) {
            LCD_Clear(0);
        }
    }
}

void initPlayingDisplay() {

    //sprintf(bitsPerSampleStr, "Bits Per Sample: %d", headers.BitsPerSample);
    //sprintf(sampleRateStr, "Sample Rate: %d", headers.SampleRate);
    char sampleStr[12];
    sprintf(sampleStr, "%d x %d", headers.SampleRate, headers.BitsPerSample);

    barIdx = 1;
    progDiv = headers.Subchunk2Size / numBars;
    progCurr = progDiv;

    playingSongNameLen = strlen(playingSongName);

    for (int i = 1; i < numBars; i++) {
        progBarStr[i] = ' ';
    }

    //LCD_DrawString(x1, y + 0x110, fc1, bc1, progBarStr, size1, mode1);
    //LCD_DrawString(x1, y + 0x10, fc1, bc1, "==========================", size1, mode1);
    LCD_DrawString(x1, y + 0x10, fc1, bc1, progBarStr, size1, mode1);
    LCD_DrawString(x1, y + 0x20, fc1, bc1, sampleStr, size1, mode1);



}

void updatePlayingDisplay() {
    // file name, header
    if (dataIdx > progCurr) {
        do {
            progBarStr[barIdx] = '=';
            barIdx++;
            progCurr += progDiv;
        } while (dataIdx > progCurr);
        LCD_DrawString(x1, y + 0x10, fc1, bc1, progBarStr, size1, mode1);
    }

    if (playingSongNameLen > 28) {
        char temp = playingSongName[0];
        for(int j = 0; j < playingSongNameLen - 1; j++) {
            playingSongName[j] = playingSongName[j + 1];
        }
        playingSongName[playingSongNameLen - 1] = temp;
        strncpy(fileNameShort, playingSongName, 28);
        LCD_DrawString(x1, y, fc1, bc1, fileNameShort, size1, mode1);
    } else {
        LCD_DrawString(x1, y, fc1, bc1, playingSongName, size1, mode1);
    }

    // List info
    int offset = 0;
    for (int i = 0; i < headers.infoListIdx; i++) {
        char* str = headers.infoList[i];
        int len = strlen(str);

        if (len > 28) {
            char temp = str[0];
            for(int j = 0; j < len - 1; j++) {
                str[j] = str[j + 1];
            }
            str[len - 1] = temp;
            strncpy(fileNameShort, str, 28);
            LCD_DrawString(x1, 0x40 + offset, fc1, bc1, fileNameShort, size1, mode1);
        } else {
            LCD_DrawString(x1, 0x40 + offset, fc1, bc1, str, size1, mode1);
        }

        offset += 0x10;
    }
}


// Display current files in current path
void updateFilesDisplay(Dir* dir)
{
    int sel = dir -> currSelection;
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


