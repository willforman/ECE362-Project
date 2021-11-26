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

int playpause_history;
int skip_history;
int playpause_button = 0; // boolean
int skip_button = 0;
int dataIdx = 0; // overall index in data array
int dataSize;
void* dac_arr;


void play(WavHeaders *header, void *array) { // this array might need to be 16 uint16_t
    // header is a struct that has access to wav info

    int samples = header->ChunkSize * 8 / header->BitsPerSample; // for reference
    dataSize = sizeof(array) / sizeof(array[0]); // num of total elements
    int dacArrSize = header->BitsPerSample == 8 ? 65535 : 65535 * 2;
    dac_arr[dacArrSize]; // max number of elements DMA_CNDTR can take

    /**************************************************
    TIMER CONFIGURATIONS
    **************************************************/
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->ARR = 48000000 / header->SampleRate - 1; // sample_rate
    TIM15->PSC = 0; 

    TIM15->CR2 |= TIM_CR2_MMS_1;
    TIM15->DIER |= TIM_DIER_UDE;
    TIM15->CR1 |= TIM_CR1_CEN;


    /**************************************************
    DMA CONFIGURATIONS
    **************************************************/
    RCC->AHBENR |= RCC_AHBENR_DMAEN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;

    if (header->BitsPerSample == 8){
        DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
    }
    else if (header->BitsPerSample == 16) {
        DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
        DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0; 
    }
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0; 
    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    DMA1_Channel5->CCR |= DMA_CCR_TCIE | DMA_CCR_HTIE; // if we need to break apart the array of data 
    DMA1_Channel5->CCR |= DMA_CCR_DIR;

    int bytesPerSample =  header->BitsPerSample / 8;
    // initalize dac_arr (copies over the first 65535 elements)
    int i = 0;
    while (i < 65535) {
        if (i > dataSize) {
            dac_arr[i] = 0; // array has run out of elements to copy over
			stop();
			break;
		}
        else {
            if (header->BitsPerSample == 8) {
                dac_arr[i] = array[i];
                i++;
            }
            else {
                dac_arr[i] = (array[i] >> 8) & array[i+1];
                i += 2;
            }
        }
    }
    dataIdx = 65535; // update where in the array 
    DMA1_Channel5->CMAR = &dac_arr;

    if (header->BitsPerSample == 8){
        DMA1_Channel5->CPAR = (&(DAC->DHR8R1));
    }
    else if (header->BitsPerSample == 16) {
        DMA1_Channel5->CPAR = (&(DAC->DHR12L1));
    }
    
    DMA1_Channel5->CNDTR = 65535; // for circular transfer
    DMA1_Channel5->CCR |= DMA_CCR_EN;
    NVIC->ISER[0] |= 1 << DMA1_Channel4_5_IRQn

    /**************************************************
    DAC CONFIGURATIONS
    **************************************************/
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODER4_0;
    DAC->CR |= DAC_CR_TSEL1_1 | DAC_CR_TSEL1_0;
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_EN1;
}

int check_buttons(void) {

    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->CR1 |= TIM_CR1_ARPE;
    TIM7->ARR = 48 - 1;
    TIM7->PSC = 100 - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM7_IRQn;

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1;
    GPIOB->PUPDR |= GPIO_PUPDR_PUPDR2_1;
    //play(); // parameters needed 
}

void TIM7_IRQHandler() { // invokes every 1ms to read from pa0 and pb2
    TIM7->SR &= ~TIM_SR_UIF;
    int pa0 = GPIOA->IDR & 0x1;
    int pb2 = GPIOB->IDR & 0x4;
    //playpause_history = playpause_history << pa0;
    //skip_history = skip_history << pb2;
    if (pa0 == 1) {
        stop(); // call function
    }
    if (pb2 == 1) {
        skip_button = 1; // call function
    }
}


void DMA1_CH4_5_6_7_DMA2_CH3_4_5_IRQHandler	() {
    if (DMA1->ISR | DMA_ISR_TCIF5 || DMA1->ISR | DMA_ISR_HTIF5) {
        // if the transfer is half complete, we starting writing the elements in the first half of the array.
        // if the transfer is fully complete, we starting writing the elements in the second half of the array.
        int start, end;
        // full transfer
        if (DMA1->ISR | DMA_ISR_TCIF5) {
            DMA1->IFCR |= DMA_IFCR_CTCIF5; // acknowledge interrupt
            start = 0;
            end = 32768;
        }
        // half transfer
        else {
            DMA1->IFCR |= DMA_IFCR_CHTIF5; // acknowledge interrupt
            start = 32768;
            end = 65535;
        }
        for (int j = start; j < end; j++) // half of dac_array
        {
            if ((j + dataIdx) > dataSize) {
            // not sure if i should break here or set to 0
                dac_arr[j] = 0; // array has run out of elements to copy over
                stop();
                break;
            }
            else {
                if (header->BitsPerSample == 8) {
                    dac_arr[j] = array[j + dataIdx];
                    dataIdx++;
                    j++;
                }
                else {
                    dac_arr[j] = (array[j + dataIdx] >> 8) & array[j + dataIdx + 1];
                    dataIdx += 2;
                    j += 2;
                }
            }
            if (header->BitsPerSample == 16) {
                dac_arr[j] += 32768; // adjust to unsigned
            }
        }
    }
}

void stop() {
	DMA1_Channel5->CCR &= ~DMA_CCR_EN;
	// logic here for pulling up next song
}
 
