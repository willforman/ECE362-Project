#include "stm32f0xx.h"
#include <math.h>
#include <stdint.h>
#include "wav.h"
#include "ff.h"
#include "DAC.h"
#include "sdcard.h"
#include "display.h"
#include "timer.h"

int playpause_history;
int skip_history;
int playpause_button = 0; // boolean
int skip_button = 0;
int dataIdx = 0; // overall index in data array (is in bytes)
int finished = 0;
int shortF = 0; // file shorter than 16000 bytes, assume not
extern WavHeaders headers;
WavHeaders* header = &headers;

extern FIL fil;
FIL * file = &fil;
extern FATFS FatFs;
uint16_t dac_arr[8000];

void togglePlay() {
    if (DMA1_Channel5->CCR & DMA_CCR_EN){ // dma enabled
        DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    }
    else {
        DMA1_Channel5->CCR |= DMA_CCR_EN;
    }
}

void stop() {
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    closeSDCardFile(&FatFs, &fil);
    enableDisplay();
}

int play() { // this array might need to be 16 uint16_t
    /**************************************************
    TIMER CONFIGURATIONS
    **************************************************/


    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->ARR = 48000000 / header->SampleRate - 1; // sample_rate
    TIM15->PSC = 0; 

    TIM15->CR2 |= TIM_CR2_MMS_1;
    TIM15->DIER |= TIM_DIER_UDE;

    /**************************************************
    DMA CONFIGURATIONS
    **************************************************/
    RCC->AHBENR |= RCC_AHBENR_DMAEN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;

    if (header->BitsPerSample == 8){
        DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
        DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;
    }
    else if (header->BitsPerSample == 16) {
        DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
        DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;
        DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
        DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
    }

    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    DMA1_Channel5->CCR |= DMA_CCR_TCIE | DMA_CCR_HTIE; // if we need to break apart the array of data 
    DMA1_Channel5->CCR |= DMA_CCR_DIR;

    f_lseek(file, 44);

    UINT bytesRead = 0;
    f_read(file,dac_arr,16000,&bytesRead);
    if (bytesRead != 16000) { // this is if file is shorter than 16000 bytes
       // need to test
       // add addition for 16 bit
        if (header->BitsPerSample == 16) {
           for (int j = 0; j< bytesRead;j++){
               dac_arr[j] += 0x8000;
               //dac_arr[j] = dac_arr[j] << 4;
               }
            }
        shortF = 1;
    }
    if (!shortF && header->BitsPerSample == 16 ) {
       for (int j = 0;j<8000;j++){
           //dac_arr[j]  = ((dacArr[j] + 0x8000) >> 8) & 0xff; // adjust to unsigned

           dac_arr[j] += 0x8000;

       }
    }

    if(shortF)
        dataIdx = bytesRead;
    else
        dataIdx = 16000; // update where in the file you are
    DMA1_Channel5->CMAR = (uint32_t) dac_arr;

    if (header->BitsPerSample == 8){
        DMA1_Channel5->CPAR = (uint32_t) (&(DAC->DHR8R1));
    }
    else if (header->BitsPerSample == 16) {
        DMA1_Channel5->CPAR = (uint32_t) (&(DAC->DHR12L1));
    }
    
    if (shortF){
        if (header->BitsPerSample == 16)
            DMA1_Channel5->CNDTR = bytesRead/2; // for circular transfer
        else
            DMA1_Channel5->CNDTR = bytesRead;
    }
    else {
        if (header->BitsPerSample == 16)
            DMA1_Channel5->CNDTR = 8000; // for circular transfer
        else
            DMA1_Channel5->CNDTR = 16000;
    }

    NVIC->ISER[0] |= (1 << DMA1_Channel4_5_IRQn);

    /**************************************************
    DAC CONFIGURATIONS
    **************************************************/
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODER4_0;
    DAC->CR |= DAC_CR_TSEL1_1 | DAC_CR_TSEL1_0;
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_EN1;

    DMA1_Channel5->CCR |= DMA_CCR_EN;
    TIM15->CR1 |= TIM_CR1_CEN;
    return W_OK;
}

void init_buttons(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->ARR = 48 - 1;
    TIM7->PSC = 100 - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;

    NVIC_SetPriority(TIM7_IRQn,0);
    NVIC->ISER[0] |= 1 << TIM7_IRQn;

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1 | GPIO_PUPDR_PUPDR1_1;
}

void DMA1_CH4_5_6_7_DMA2_CH3_4_5_IRQHandler () {

    // if the transfer is half complete, we starting writing the elements in the first half of the array.
    // if the transfer is fully complete, we starting writing the elements in the second half of the array.
    uint16_t * startAddr;
    // full transfer
    //int res = DMA1->ISR & DMA_ISR_TCIF5;
    if (DMA1->ISR & DMA_ISR_TCIF5) {
        DMA1->IFCR = DMA_IFCR_CTCIF5; // acknowledge interrupt
        startAddr = (uint16_t*) (&dac_arr[4000]);
        if (finished){
               finished = 0;
               stop();
               return;
            }
    }
    // half transfer
    else if (DMA1->ISR & DMA_ISR_HTIF5){
        DMA1->IFCR = DMA_IFCR_CHTIF5; // acknowledge interrupt
        startAddr = (uint16_t*) dac_arr;
        if (finished){
               finished = 0;
               stop();
               return;
            }
    }
    UINT bytesRead = 0;
    f_read(file,startAddr, 8000, &bytesRead);
    dataIdx += bytesRead;
    if (bytesRead != 8000){ // end of file; nothing more to read
        finished = 1;
    }
    if (header->BitsPerSample == 16) {
        for (int j = 0;j<4000;j++){

            startAddr[j] += 0x8000; // adjust to unsigned
        }
    }
}

