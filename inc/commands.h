#ifndef __COMMANDS_H__
#define __COMMANDS_H__
#include "display.h"

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

void scrollDisplay(Dir* dir);

void clearDisplay();

#endif /* __COMMANDS_H_ */
