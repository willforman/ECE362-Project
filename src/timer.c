#include "stm32f0xx.h"
#include "display.h"
#include "dac.h"
#include "lcd.h"
#include "ff.h"
#include "commands.h"
Dir dir;
int mode = 0;
uint8_t pa1State = 0;
uint8_t pa0State = 0;
int pressPa0 = 0;
int releasePa0 = 0;
int pressPa1 = 0;
int releasePa1 = 0;
FRESULT res;

// Is for scrolling the display
void initDisplay(int calledEveryMs) {
    // initialize timer 6
    RCC->APB1ENR = RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 480 - 1;
    TIM6->ARR = calledEveryMs - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] = 1 << TIM6_DAC_IRQn;

    LCD_Setup();
    LCD_Clear(0);
}

void initButtonScanning(int calledEveryMs) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->CR1 |= TIM_CR1_ARPE;
    TIM7->ARR = 48 - 1;
    TIM7->PSC = 100 * calledEveryMs - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] = 1 << TIM7_IRQn;

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1;
}

void enableTimer6() {
    TIM6->CR1 |=  TIM_CR1_CEN;
}

void disableDisplay() {
    TIM6->CR1 &= ~TIM_CR1_CEN;
    LCD_Clear(0);
}

void enableButtonScanning() {
    TIM7->CR1 |=  TIM_CR1_CEN;
}

void disableButtonScanning() {
    TIM7->CR1 &= ~TIM_CR1_CEN;
}

FRESULT enableDisplay() {
    FRESULT res;

    LCD_Clear(0);
    mode = 0;

    f_chdir("/");
    res = updateFiles(&dir, "");
    if (res) {
        return res;
    }

    return 0;
}

void enableErrorMode(char* error) {
    mode = 2;
    disableDisplay();
}

void TIM6_DAC_IRQHandler() {
    TIM6->SR &= ~TIM_SR_UIF;

    if (mode) {
        updatePlayingDisplay();
    }
    else {
        updateFilesDisplay(&dir);
    }
}

#define DEBOUNCING // Uncommented, run with debouncing. Commented, run without
#ifdef DEBOUNCING


void TIM7_IRQHandler() { // invokes every 1ms to read from pa0 and pb2
    TIM7->SR = ~TIM_SR_UIF;
    int pa0 = GPIOA->IDR & 0x1;
    int pa1 = (GPIOA->IDR & 0x2) >> 1;
    pa0State = (pa0State << 1) | pa0;
    pa0State &= 0xf;
    pa1State = (pa1State << 1) | pa1;
    pa1State &= 0xf;
    if (pa0State == 0xf){
        pressPa0 = 1;
        releasePa0 = 0;
    }
    else if (pa0State == 0xe && pressPa0){
        releasePa0 = 1;
        pressPa0 = 0;
        // clear the history byte
        pa0State = 0;
    }
    if (pa1State == 0xf){
        pressPa1 = 1;
        releasePa1 = 0;
    }
    else if (pa1State == 0xe && pressPa1){
        releasePa1 = 1;
        pressPa1 = 0;
        pa1State = 0;
    }
    // if playing song
    if (mode == 1) {
            // first button: play pause
            if (releasePa0) {
                releasePa0 = 0;
                togglePlay();
                return;
            }
            // second button: end song
            else if (releasePa1) {
                releasePa1 = 0;
                stop();
                mode = 0;
                return;
            }
        }
        // second button: end song
    else if (mode == 0) { // song selection
            // first button: select file
        if (releasePa0) {
            releasePa0 = 0;
            int selectedWav;
            res = handleFileSelectButton(&dir, &selectedWav);
            if (res) {
                if (res == 25) {
                    return;
                }
                disableButtonScanning();
                disableDisplay();
                printEndError(res);
                return;
            }
            if (selectedWav) {
                mode = 1;
                LCD_Clear(0);
                initPlayingDisplay();
            }
            return;
        }
        // second button: move the file to the next
        else if (releasePa1) {
            releasePa1 = 0;
            handleFileNextButton(&dir);
            return;
        }
    } else { // mode: error displayed on screen
        if (releasePa0) {
            mode = 0;
            enableDisplay();
            enableTimer6();
            releasePa0 = 0;
        }
    }
}
#endif

#ifndef DEBOUNCING

void TIM7_IRQHandler() { // invokes every 1ms to read from pa0 and pb2
    TIM7->SR &= ~TIM_SR_UIF;
    int pa0 = GPIOA->IDR & 0x1;
    int pa1 = GPIOA->IDR & 0x2;
    // if playing song
    if (mode == 1) {
        // first button: play pause
        if (pa0) {
            releasePa0 = 0;
            togglePlay();
            return;
        }
        // second button: end song
        else if (pa1) {
            releasePa1 = 0;
            stop();
            mode = 0;
            return;
        }
    }
    // second button: end song
    else if (mode == 0) { // song selection
        // first button: select file
        if (pa0) {
            releasePa0 = 0;
            int selectedWav;
            res = handleFileSelectButton(&dir, &selectedWav);
        if (res) {
            return;
        }
        if (selectedWav) {
            mode = 1;
            LCD_Clear(0);
            initPlayingDisplay();
        }
        return;
        }
        // second button: move the file to the next
        else if (pa1) {
            releasePa1 = 0;
            handleFileNextButton(&dir);
            return;
        }
    } else { // mode: error displayed on screen
        if (pa0) {
            mode = 0;
            enableDisplay();
            enableTimer6();
            releasePa0 = 0;
        }
    }


}
#endif

