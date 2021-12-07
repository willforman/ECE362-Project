#include "stm32f0xx.h"
#include "display.h"
#include "dac.h"
#include "lcd.h"
#include "ff.h"
#include "commands.h"
Dir dir;
int playingSong = 0;
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
    TIM6->PSC = 48 - 1;
    TIM6->ARR = 100 * calledEveryMs - 1;
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

FRESULT enableDisplay() {
    FRESULT res;
    f_chdir("/");
    res = updateFiles(&dir, "");
    if (res) {
        return res;
    }

    TIM6->CR1 |=  TIM_CR1_CEN;

    playingSong = 0;

    return 0;
}

void disableDisplay() {
    TIM6->CR1 &= ~TIM_CR1_CEN;
    clearDisplay();
}

void enableButtonScanning() {
    TIM7->CR1 |=  TIM_CR1_CEN;
}

void disableButtonScanning() {
    TIM7->CR1 &= ~TIM_CR1_CEN;
}

void TIM6_DAC_IRQHandler() {
    TIM6->SR &= ~TIM_SR_UIF;


    scrollDisplay(&dir);

}

void TIM7_IRQHandler() { // invokes every 1ms to read from pa0 and pb2
    TIM7->SR &= ~TIM_SR_UIF;
    int pa0 = GPIOA->IDR & 0x1;
    int pa1 = GPIOA->IDR & 0x2;
    // if playing song
    if (playingSong) {
        // first button: play pause
        if (pa0) {
            togglePlay();
            return;
        }
        // second button: end song
        else if (pa1) {
            stop();
            playingSong = 0;
            return;
        }
    } else { // song selection
        // first button: select file
        if (pa0) {
            int selectedWav;
            res = handleFileSelectButton(&dir, &selectedWav);
            if (res) {
                return;
            }
            if (selectedWav) {
                playingSong = 1;
                disableDisplay();
            }
            return;
        }
        // second button: move the file to the next
        else if (pa1) {
            handleFileNextButton(&dir);
            return;
        }
    }
}

/*void TIM7_IRQHandler() { // invokes every 1ms to read from pa0 and pb2
    TIM7->SR &= ~TIM_SR_UIF;
    int pa0 = GPIOA->IDR & 0x1;
    int pa1 = GPIOA->IDR & 0x2;

    pa0State = pa0State << 1 | pa0;
    pa1State = pa1State << 1 | pa1;

    if (pa0State == 0xff){
        pressPa0 = 1;
        releasePa0 = 0;
    }
    else if (pa0State == 0xfe && pressPa0){
        releasePa0 = 1;
        pressPa0 = 0;
    }

    if (pa1State == 0xff){
        pressPa1 = 1;
        releasePa1 = 0;
    }
    else if (pa1State == 0xfe && pressPa1){
        releasePa1 = 1;
        pressPa1 = 0;
    }

    // if playing song
    if (playingSong) {
        // first button: play pause
        if (pressPa0) {
            pressPa0 = 0;
            releasePa0 = 0;
            togglePlay();
            return;
        }
        // second button: end song
        else if (releasePa1) {
            pressPa1 = 0;
            releasePa = 0;
            stop();
            playingSong = 0;
            return;
        }
    } else { // song selection
        // first button: select file
        if (pa0) {
            if (handleFileSelectButton(&dir)) {
                playingSong = 1;
                disableDisplay();
            }
            return;
        } 
        // second button: move the file to the next
        else if (pa1) {
            handleFileNextButton(&dir);
            return;
        }
    }
}*/
