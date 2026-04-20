/*
*
*   uart.c
*
*
*
*   @author
*   @date
*/

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include "uart.h"
#include <cyBot_uart.h>
#include <cyBot_Scan.h>
void init_cybot_manual_uart();
//void init_cybot_manual_uart(){
//    cyBot_uart_init_clean();  // Clean UART1 initialization, before running your UART1 GPIO init code
//
//    // Complete this code for configuring the GPIO PORTB part of UART1 initialization (your UART1 GPIO init code)
//    SYSCTL_RCGCGPIO_R |= 0x02;// This will enable port B
//    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {}; //This is to check if port B is set or not
//    GPIO_PORTB_DEN_R |= 0x03;//Enable the digital function of the Pins
//    GPIO_PORTB_AFSEL_R |= 0x03;//Tell the pins that the GPIO will be driven by alternative perihperal mode
//    /*This part below here is to tell the system the hardware is UART
//     */
//    GPIO_PORTB_PCTL_R = GPIO_PORTB_PCTL_R & (~0x000000FF); //Reset the entire resistor to make sure the set value is correct
//    GPIO_PORTB_PCTL_R |= 0x11; //Make sure the sum of 4 bits at the byte for PINS 0 and 1 are 1
//    cyBot_uart_init_last_half();  // Complete the UART device configuration
//}
void uart_init(void);
void uart_init(void){
	//TODO
  //enable clock to GPIO port B
  SYSCTL_RCGCGPIO_R |= 0x02;

  //enable clock to UART1
  SYSCTL_RCGCUART_R |= 0x02;

  //wait for GPIOB and UART1 peripherals to be ready
  while ((SYSCTL_PRGPIO_R & 0x02) == 0) {};
  while ((SYSCTL_PRUART_R & 0x02) == 0) {};

  //enable alternate functions on port B pins
  GPIO_PORTB_AFSEL_R |= 0x03;

  //enable digital functionality on port B pins
  GPIO_PORTB_DEN_R |= 0x03;

  //enable UART1  port B pins
  GPIO_PORTB_PCTL_R = GPIO_PORTB_PCTL_R & (~0x000000FF); //Reset the entire resistor to make sure the set value is correct
  GPIO_PORTB_PCTL_R |= 0x11;

  //calculate baud rate
  uint16_t iBRD = (16000000.0) / (16.0 * 115200.0);

  float dum_cal = (16000000.0) / (16.0 * 115200.0) - iBRD;

  uint16_t fBRD = dum_cal * 64.0 + 0.5;

  //turn off UART1 while setting it up
  UART1_CTL_R &= ~0x01;

  //set baud rate
  //note: to take effect, there must be a write to LCRH after these assignments
  UART1_IBRD_R = iBRD;
  UART1_FBRD_R = fBRD;

  //set frame, 8 data bits, 1 stop bit, no parity, no FIFO
  //note: this write to LCRH must be after the BRD assignments
  UART1_LCRH_R = 0x0000060;

  //use system clock as source
  //note from the datasheet UARTCCC register description:
  //field is 0 (system clock) by default on reset
  //Good to be explicit in your code
  UART1_CC_R = 0x0;

  //re-enable UART1 and also enable RX, TX (three bits)
  //note from the datasheet UARTCTL register description:
  //RX and TX are enabled by default on reset
  //Good to be explicit in your code
  //Be careful to not clear RX and TX enable bits
  //(either preserve if already set or set them)
  UART1_CTL_R |= 0x301;
}
//
void uart_blocking_sendChar(char data){
    while (UART1_FR_R & 0x20);
    UART1_DR_R |= data;
}

char uart_blocking_receive(void){
    while (UART1_FR_R & 0x10);  // Bit 4 = RXFE (Receive FIFO Empty)
    // Read the first 8 bits from Data Register
    return (char)(UART1_DR_R & 0xFF);
}
//
void uart_blocking_sendStr(const char *data){
	while (*data != '\0'){
	    uart_sendChar(*data);
	    data++;
	}
}
char uart_receive_nonblocking(void){
    //Check if the FIFO is not empty
    char a = '0';
    if (!(UART1_FR_R & 0x10)){
        //Check if the FiFio is not transmitting data
            a = (char)(UART1_DR_R & 0xFF); //assign the byte received to the character , the 0XFF is not crucial , I just add it for sure
    }  // Bit 4 = RXFE (Receive FIFO Empty)
    // Read the first 8 bits from Data Register
    return a;
}
