#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "tty.h"
#include "sdcard.h"
#include <stdio.h>

void init_spi1_slow() {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GPIOB->MODER &= ~GPIO_MODER_MODER3;
    GPIOB->MODER &= ~GPIO_MODER_MODER4;
    GPIOB->MODER &= ~GPIO_MODER_MODER5;
    GPIOB->MODER |= GPIO_MODER_MODER3_1;
    GPIOB->MODER |= GPIO_MODER_MODER4_1;
    GPIOB->MODER |= GPIO_MODER_MODER5_1;

    // set all 3 (PB3, PB4, PB5) to alternate function 0000
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFR3);
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFR4);
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFR5);

    SPI1->CR1 &= ~SPI_CR1_SPE; // Disable SPI1 channel
    SPI1->CR1 |= SPI_CR1_BR; // Set baud rate to divide by 256
    SPI1->CR1 |= SPI_CR1_MSTR;  // Master mode

    // DATA SIZE MAY NEED TO CHANGE
    SPI1->CR2 |= SPI_CR2_DS; // Set data size to 8 bit
    SPI1->CR2 &= ~SPI_CR2_DS_3;

    SPI1->CR1 |= SPI_CR1_SSM;// Set software slave management
    SPI1->CR1 |= SPI_CR1_SSI;// Set internal slave select
    SPI1->CR2 |= SPI_CR2_FRXTH;// Set FIFO reception threshold
    SPI1->CR1 |= SPI_CR1_SPE; // Enable SPI1 channel
}

void enable_sdcard() {
    GPIOB->BSRR |= (1<<(2+ 16)); // set PB2 low
}

void disable_sdcard(){
    GPIOB->BSRR |= (1<<(2)); // set PB2 high
}

void init_sdcard_io() {
    init_spi1_slow();
    // Configure pb2 as an output
    GPIOB->MODER &= ~GPIO_MODER_MODER2;
    GPIOB->MODER |= GPIO_MODER_MODER2_0;
    disable_sdcard();
}

void sdcard_io_high_speed() {
    SPI1->CR1 &= ~SPI_CR1_SPE; // Disable SPI1 channel
    SPI1->CR1 &= ~SPI_CR1_BR;
    SPI1->CR1 |=SPI_CR1_BR_0; // baud rate of 12 MHz
    SPI1->CR1 |= SPI_CR1_SPE;
}

void init_lcd_spi() {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~GPIO_MODER_MODER8;
    GPIOB->MODER |= GPIO_MODER_MODER8_0;
    GPIOB->MODER &= ~GPIO_MODER_MODER14;
    GPIOB->MODER |= GPIO_MODER_MODER14_0;
    GPIOB->MODER &= ~GPIO_MODER_MODER11;
    GPIOB->MODER |= GPIO_MODER_MODER11_0;
    init_spi1_slow();
    sdcard_io_high_speed();
}
