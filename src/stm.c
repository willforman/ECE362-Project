#include "stm32f0xx.h"
#include "stm.h"

void init_usart5() {
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC->AHBENR |= RCC_AHBENR_GPIODEN;

    // Alternate: PC12, PD2
    GPIOC->MODER |= GPIO_MODER_MODER12_1;
    GPIOC->MODER &= ~GPIO_MODER_MODER12_0;

    GPIOD->MODER |= GPIO_MODER_MODER2_1;
    GPIOD->MODER &= ~GPIO_MODER_MODER2_0;

    // PC12 -> USART5_TX (AF2)
    GPIOC->AFR[1] &= ~0xf0000;
    GPIOC->AFR[1] |= 0x20000;

    // PD2 -> USART5_RX (AF2)
    GPIOD->AFR[0] &= ~0xf00;
    GPIOD->AFR[0] |= 0x200;

    RCC->APB1ENR |= RCC_APB1ENR_USART5EN; // Enable RCC Clock
    USART5->CR1 &= ~USART_CR1_UE; // USART EN: Off
    USART5->CR1 &= ~0x10001000; // Word size: 8 bit
    USART5->CR2 &= ~USART_CR2_STOP; // Stop bits: 1 bit
    USART5->CR1 &= ~USART_CR1_PCE; // Parity: Off
    USART5->CR1 &= ~USART_CR1_OVER8; // Oversampling: 16
    USART5->BRR =  0x1a1; // Baud rate: 115.2 kbaud
    USART5->CR1 |= USART_CR1_TE; // Transmitter: On
    USART5->CR1 |= USART_CR1_RE; // Receiver: On
    USART5->CR1 |= USART_CR1_UE; // USART EN: On

    while (!(USART5->ISR & USART_ISR_REACK)); // Wait for RE to be acked
    while(!(USART5->ISR & USART_ISR_TEACK)); // Wait for TE to be acked
}

void enable_tty_interrupt() {
    USART5->CR1 |= USART_CR1_RXNEIE;

    NVIC->ISER[0] |= 1 << USART3_8_IRQn;
    USART5->CR3 |= USART_CR3_DMAT;
    USART5->CR3 |= USART_CR3_DMAR;

    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->RMPCR |= DMA_RMPCR2_CH2_USART5_RX; // Remap register set to allow it
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;  // EN: Off
    DMA2_Channel2->CMAR = (uint32_t) &serfifo;
    DMA2_Channel2->CPAR = (uint32_t) &USART5->RDR;
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR; // Peripheral -> memory
    DMA2_Channel2->CCR &= ~DMA_CCR_HTIE; // Half transfer interrupt: Off
    DMA2_Channel2->CCR &= ~DMA_CCR_TCIE; // Full transfer interrupt: Off
    DMA2_Channel2->CCR &= ~DMA_CCR_MSIZE; // MSIZE: 8 bits
    DMA2_Channel2->CCR &= ~DMA_CCR_PSIZE; // PSIZE: 8 bits
    DMA2_Channel2->CCR |=  DMA_CCR_MINC; // Increment CMAR
    DMA2_Channel2->CCR &= ~DMA_CCR_PINC; // CPAR always points to USART5_RDR
    DMA2_Channel2->CCR |=  DMA_CCR_CIRC; // Circular transfers
    DMA2_Channel2->CCR &= ~DMA_CCR_MEM2MEM; // Do not enable MEM2MEM
    DMA2_Channel2->CCR |=  DMA_CCR_PL;
    DMA2_Channel2->CCR |= DMA_CCR_EN; // EN: On
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
    SPI1->CR1 |= SPI_CR1_BR; // Set baud rate to 256
    SPI1->CR1 |= SPI_CR1_MSTR;  // Master mode
    
    // DATA SIZE MAY NEED TO CHANGE
    SPI1->CR2 |= SPI_CR2_DS; // Set data size to 8 bit
    SPI1->CR2 &= ~SPI_CR2_DS_3;
    
    SPI1->CR1 |= SPI_CR1_SSM;// Set software slave management
    SPI1->CR1 |= SPI_CR1_SSI;// Set internal slave select
    SP1->CR2 |= SPI_CR2_FRXTH;// Set FIFO reception threshold
    SPI1->CR1 |= SPI_CR1_SPE; // Enable SPI1 channel
}

void enable_sdcard() {
    GPIOB->BSRR |= (1<<(2+ 16))); // set PB2 low
}

void disable_sdcard(){
    GPIOB->BSRR |= (1<<(2))); // set PB2 high
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