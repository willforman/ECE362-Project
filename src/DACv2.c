/**
 ******************************************************************************
 * @file    main.c
 * @author  Ac6
 * @version V1.0
 * @date    01-December-2013
 * @brief   Default main function.
 ******************************************************************************
 */

#include "stm32f0xx.h"
#include <math.h>
#include <stdint.h>
#include "wav.h"
#include "ff.h"
#include "DAC.h"
#include "sdcard.h"
int playpause_history;
int skip_history;
int playpause_button = 0; // boolean
int skip_button = 0;
int dataIdx = 0; // overall index in data array (is in bytes)
int finished = 0;

extern WavHeaders headers;
WavHeaders* header = &headers;

extern FIL fil;
FIL * file = &fil;
extern FATFS FatFs;
uint16_t dac_arr[8000];
//uint16_t array8bit[8000];





void stop() {
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    RCC->AHBENR &= ~RCC_AHBENR_DMAEN;
    closeSDCardFile(&FatFs, &fil);
    f_mount(0, "", 1);
    // logic here for pulling up next song
}

WavResult play() { // this array might need to be 16 uint16_t
    // header is a struct that has access to wav info
    //int samples = header->ChunkSize * 8 / header->BitsPerSample; // for reference
     // num of total elements
    //int dacArrSize = header->BitsPerSample == 8 ? 65535 : 65535 * 2;
    //dac_arr[dacArrSize]; // max number of elements DMA_CNDTR can take

    // this is an array which in the case there are 8 bits per sample, it splits every element into two elements

    // if there are 8 bit samples, twice the original read elements

    /**************************************************
    TIMER CONFIGURATIONS
    **************************************************/
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->ARR = 48000000 / header->SampleRate - 1; // sample_rate
    //TIM15->ARR =  48000000 /8000 -1;
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
        DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
        DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
    }

    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    DMA1_Channel5->CCR |= DMA_CCR_TCIE | DMA_CCR_HTIE; // if we need to break apart the array of data 
    DMA1_Channel5->CCR |= DMA_CCR_DIR;



    f_lseek(file, 44);

    UINT dataSize = header->Subchunk2Size;
    UINT bytesRead = 0;
    if (dataSize < 16000) {
        f_read(file,dac_arr,dataSize,&bytesRead);

        if (bytesRead != dataSize) {
            fprintf(stderr, "Data read: expected=%ld, actual=%d\n", header->Subchunk2Size, bytesRead);
            return W_ERR_READING_DATA;
        }
        if (header->BitsPerSample == 16) {
            for (int j = 0;j<dataSize;j++){
                dac_arr[j] += 32768; // adjust to unsigned
            }
        }
    }

    else {
        f_read(file,dac_arr,16000,&bytesRead);
        if (bytesRead != 16000) {
            fprintf(stderr, "Data read: expected=%ld, actual=%d\n", header->Subchunk2Size, bytesRead);
            return W_ERR_READING_DATA;
        }
        if (header->BitsPerSample == 16) {
            for (int j = 0;j<16000;j++){
                dac_arr[j] += 32768; // adjust to unsigned
            }
        }
    }
    dataIdx = 16000; // update where in the file you are
    DMA1_Channel5->CMAR = (uint32_t) dac_arr;

    if (header->BitsPerSample == 8){
        DMA1_Channel5->CPAR = (uint32_t) (&(DAC->DHR8R1));
    }
    else if (header->BitsPerSample == 16) {
        DMA1_Channel5->CPAR = (uint32_t) (&(DAC->DHR12L1));
    }
    
    if (header->BitsPerSample == 16)
        DMA1_Channel5->CNDTR = 8000; // for circular transfer
    else
        DMA1_Channel5->CNDTR = 16000;


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

//void check_buttons(void) {
//
//    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
//    TIM7->CR1 |= TIM_CR1_ARPE;
//    TIM7->ARR = 48 - 1;
//    TIM7->PSC = 100 - 1;
//    TIM7->DIER |= TIM_DIER_UIE;
//    TIM7->CR1 |= TIM_CR1_CEN;
//    NVIC->ISER[0] |= 1 << TIM7_IRQn;
//
//    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
//    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
//    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1;
//    GPIOB->PUPDR |= GPIO_PUPDR_PUPDR2_1;
//    //play(); // parameters needed
//}

//void TIM7_IRQHandler() { // invokes every 1ms to read from pa0 and pb2
//    TIM7->SR &= ~TIM_SR_UIF;
//    int pa0 = GPIOA->IDR & 0x1;
//    int pb2 = GPIOB->IDR & 0x4;
//    //playpause_history = playpause_history << pa0;
//    //skip_history = skip_history << pb2;
//    if (pa0 == 1) {
//        stop(); // call function
//    }
//    if (pb2 == 1) {
//        skip_button = 1; // call function
//    }
//}


void DMA1_CH4_5_6_7_DMA2_CH3_4_5_IRQHandler	() {

    if (finished){
       stop();
       return;
    }
    // if the transfer is half complete, we starting writing the elements in the first half of the array.
    // if the transfer is fully complete, we starting writing the elements in the second half of the array.
    int * startAddr;
    // full transfer
    //int res = DMA1->ISR & DMA_ISR_TCIF5;
    if (DMA1->ISR & DMA_ISR_TCIF5) {
        DMA1->IFCR = DMA_IFCR_CTCIF5; // acknowledge interrupt
        startAddr = (int*) (&dac_arr[4000]);
    }
    // half transfer
    else if (DMA1->ISR & DMA_ISR_HTIF5){
        DMA1->IFCR = DMA_IFCR_CHTIF5; // acknowledge interrupt
        startAddr = (int*) dac_arr;

    }
    UINT dataSize = header->Subchunk2Size;
    UINT bytesRead = 0;
//    if ((dataIdx + 8000) > dataSize){ // when you're done
//
//        f_read(file,startAddr, dataSize-dataIdx, &bytesRead);
//        for (int i = dataSize-dataIdx;i<dataSize;i++) {
//            dac_arr[i] = 0;
//        }
//        stop();
//    }
//    else {

        f_read(file,startAddr, 8000, &bytesRead);
        dataIdx += bytesRead;
        if (bytesRead != 8000){ // end of file; nothing more to read
            finished = 1;
//            for (int i=((bytesRead / 2) + 1);i <4000;i++){
//                startAddr[i] = 0;
//            }
//
//
//            stop();
        }


  //      }


//        for (int j = start; j < end; j++) // half of dac_array
//        {
//            if ((j + dataIdx) > dataSize) {
//            // not sure if i should break here or set to 0
//                dac_arr[j] = 0; // array has run out of elements to copy over
//                stop();
//
//                break;
//            }
//            else {
////                if (header->BitsPerSample == 8) {
////                    dac_arr[j] = array8bit[j + dataIdx];
////                    dataIdx++;
////                    j++;
////                }
////                else {
////                    dac_arr[j] = array[j+dataIdx];`
////                    dataIdx++;
////                    j++;
////                }
//                f_read(file, dac_arr, 8000, )
//            }
            if (header->BitsPerSample == 16) {
                for (int j = 0;j<4000;j++){
                    startAddr[j] += 32768; // adjust to unsigned
                }
            }


}


 
