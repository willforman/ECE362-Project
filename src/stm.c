#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "tty.h"
#include "commands.h"
#include <stdio.h>
#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

void init_usart5(){
    RCC->AHBENR |= RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOCEN;

    GPIOC->MODER &= ~GPIO_MODER_MODER12;
    GPIOC->MODER |= GPIO_MODER_MODER12_1;
    GPIOD->MODER &= ~GPIO_MODER_MODER2;
    GPIOD->MODER |= GPIO_MODER_MODER2_1;

    //not sure if need
    GPIOC->AFR[1] &= ~(0xf<<(4*4));
    GPIOC->AFR[1] |= 2 << (4*4);
    GPIOD->AFR[0] &= ~((0xf) << (4*2));
    GPIOD->AFR[0] |= 2 << (4*2);


    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;


    USART5->CR1 &= ~USART_CR1_UE;
    USART5->CR1 &= ~USART_CR1_M;
    USART5->CR1 &= ~(1<<28);
    USART5->CR2 &= ~USART_CR2_STOP_0;
    USART5->CR2 &= ~USART_CR2_STOP_1;
    USART5->CR1 &= ~USART_CR1_PCE;
    USART5->CR1 &= ~USART_CR1_OVER8;
    USART5->BRR = 0x1a1;
    USART5->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART5->CR1 |= USART_CR1_UE;
    while((!(USART5->ISR & USART_ISR_TEACK) && (!(USART5->ISR & USART_ISR_REACK))));




}

void enable_tty_interrupt() {
    USART5->CR1 |= USART_CR1_RXNEIE;
    NVIC->ISER[0] |= 1 << USART3_8_IRQn;
    USART5->CR3 |= USART_CR3_DMAR;

    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->RMPCR |= DMA_RMPCR2_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;  // First make sure DMA is turned off


    DMA2_Channel2->CMAR = (int) serfifo;
    DMA2_Channel2->CPAR = (int) (&(USART5->RDR));
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;
    DMA2_Channel2->CCR &= ~DMA_CCR_TCIE;
    DMA2_Channel2->CCR &= ~DMA_CCR_HTIE;
    DMA2_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_CIRC;
    DMA2_Channel2->CCR &= ~DMA_CCR_PSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_MSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_PINC;
    DMA2_Channel2->CCR &= ~DMA_CCR_MEM2MEM;
    DMA2_Channel2->CCR |= DMA_CCR_PL;



    DMA2_Channel2->CCR |= DMA_CCR_EN;

}


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

/*
void play() {
    FATFS FatFs;
    FRESULT res;
    FR

    FIL fil;
    uint32_t total, free;

    res = f_mount(&FatFs, "", 0);

    if (res != FR_OK) {
        return;
    }

}
*/

int __io_putchar(int c) {
    if (c == '\n') {
        while(!(USART5->ISR & USART_ISR_TXE)) { }
        USART5->TDR = '\r';
    }
    while(!(USART5->ISR & USART_ISR_TXE)) { }
    USART5->TDR = c;
    return c;
}
/*
int __io_getchar(void) {
//     while (!(USART5->ISR & USART_ISR_RXNE)) { }
//     char c = USART5->RDR;
//     if (c == '\r'){
//         c = '\n';
//     }
     return line_buffer_getchar();
     //return x;
}

int interrupt_getchar(void) {
    USART_TypeDef *u = USART5;
        // If we missed reading some characters, clear the overrun flag.
//    if (u->ISR & USART_ISR_ORE)
//        u->ICR |= USART_ICR_ORECF;
    // Wait for a newline to complete the buffer.
    while(fifo_newline(&input_fifo) == 0) {
//        while (!(u->ISR & USART_ISR_RXNE))
//            ;
        asm volatile ("wfi");
        insert_echo_char(u->RDR);
    }
    // Return a character from the line buffer.
    char ch = fifo_remove(&input_fifo);
    return ch;



}
*/


int interrupt_getchar(void) {
    while(fifo_newline(&input_fifo) == 0) {
    //        while (!(u->ISR & USART_ISR_RXNE))
    //            ;
            asm volatile ("wfi");
        }
    char ch = fifo_remove(&input_fifo);
    return ch;
}

int __io_getchar(void) {
    return interrupt_getchar();
}


void USART3_4_5_6_7_8_IRQHandler(void){
    while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) {
                    if (!fifo_full(&input_fifo))
                        insert_echo_char(serfifo[seroffset]);
                    seroffset = (seroffset + 1) % sizeof serfifo;
                }

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


int main() {
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    command_shell();
}
