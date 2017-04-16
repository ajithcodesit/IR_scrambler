/*
 * NEC infrared code sender program for Atmel ATtiny85.
 * This code is based on Dave Jones's tutorial on IR Remote Control
 * protocol using an Arduino found here https://www.eevblog.com.
 * The Arduino code is adapted to work on an ATtiny85 with some extra 
 * functions to control the sending of IR codes.
 * Written by Ajith Thomas
 */

#ifndef F_CPU
#define F_CPU 8000000UL 
#endif

#include<avr/io.h>
#include<util/delay.h>
#include<avr/power.h>
#include<avr/sleep.h>
#include<avr/interrupt.h>

/*
*NEC code for remote
*Power : 0x80BF3BC4
*Channel + : 0x80BFA15E
*Channel - : 0x80BF619E
*Mute : 0x80BF39C6
*/

#define LED_DDR DDRB
#define LED_PORT PORTB
#define IR_LED PB3 /*IR LED is PB4*/
#define IND_LED PB4 /*Indicatror LED is PB3*/

#define BUTTON_DDR DDRB
#define BUTTON_PIN PINB
#define BUTTON PB2 /*This button is used to wake up from power off state*/

const uint16_t BIT_TIME=562; /*Bit time to differentiate the logic level (Refer NEC protocol)*/

enum codes /*32-Bit IR codes for NEC protocol*/
{
	POWER=0x80BF3BC4,
	CHA_P=0x80BFA15E,
	CHA_N=0x80BF619E,
	MUTE=0x80BF39C6,
	DEF=0x00000000
};

uint32_t ir_code=DEF; /*Code to be send*/
uint32_t mask=0x80000000; /*Mask fo the MSB*/

void initInterrupt0(void) /*Initializes INT0*/
{
	MCUCR&=~((1<<ISC01)|(1<<ISC00)); /*Low level is used to trigger the interrupt (Compound AND is used to clear the BITS without interfering with other BITS in the register)*/
	GIMSK|=(1<<INT0); /*Enabling INT0 in the General Interrupt Mask register*/
	/*sei() should be called after this function to enable global interrupts*/
}

void uninitInterrupt0(void)
{
	GIMSK&=~(1<<INT0); /*Disables INT0*/
}

void sleep()
{
	initInterrupt0(); /*Initialize the interrupt pin INT0*/
	set_sleep_mode(SLEEP_MODE_PWR_DOWN); /*Sleep mode is set to power the chip down*/
	sleep_enable(); /*Enable sleep capability*/
	sei(); /*Global interrupt is enabled to wake the chip up again through INT0 (LOW level is detected)*/
	sleep_cpu(); /*CPU is put to sleep*/
	
	cli(); /*When woken up clear global intrrupts*/
	uninitInterrupt0(); /*Disable INT0 as interrupt pin*/
	sleep_disable(); /*Sleep capability is disabled*/
	sei(); /*Global interrupt is turned on again*/
}

ISR(INT0_vect)
{
	if((!(BUTTON_PIN & (1<<BUTTON))) && (MCUCR & (1<<ISC01))) /*If button is pressed and falling edge interrupt is enabled then go to sleep*/
	{
		sleep(); /*Cancels the scramble operation and goes to sleep immediately*/
	}
}

void initCancelInterrupt0(void) /*Initializes the start button to act as a cancel button or go to sleep button*/
{
	MCUCR|=(1<<ISC01); /*Falling edge interrupt is used (Compound OR is used to set the BIT without interfering other BITS in the register)*/
	GIMSK|=(1<<INT0); /*Enabling INT0 in the General Interrupt Mask register*/
}

void IR_carrier(uint16_t ir_time_us) /*Function that produces 38kHz carrier wave*/
{
	uint16_t i;
	for(i=0; i<(ir_time_us/26); i++) /*Toggles the pin ON and OFF (Order is important)*/
	{
		LED_PORT|=(1<<IR_LED); /*ON*/
		_delay_us(13);
		LED_PORT&=~(1<<IR_LED); /*OFF*/
		_delay_us(13);
	}
}

void IR_send_code(uint32_t code) /*Function to send the 32 bit code*/
{
	/*Sending the leader code*/
	IR_carrier(9000); /*For 9 ms*/
	_delay_us(4500); /*Silence for 4.5 ms*/
	
	uint8_t i;
	for(i=0; i<32; i++) /*All of the 4 bytes is sent starting with the MS*/
	{
		IR_carrier(BIT_TIME); /*Carrier is turned on for one bit time*/
		if(code & mask)
		{
			_delay_us((3*BIT_TIME)); //Sending a HIGH bit (3 times the normal bit time)
			
		}
		else
		{
			_delay_us(BIT_TIME); //Sending a LOW bit (Only one bit)
		}
		code<<=1;
	}
	IR_carrier(BIT_TIME); //Sending a stop bit
}

void scramble(uint8_t strt_del, uint8_t no_ch, uint16_t pow_cyc) /*This is the scramble routine*/
{
	uint8_t i=0;
	for(i=0; i<strt_del; i++) /*Routine start delay in seconds*/
	{
		LED_PORT|=(1<<IND_LED);
		_delay_ms(500);
		LED_PORT&=~(1<<IND_LED);
		_delay_ms(500);
	}
	
	ir_code=MUTE; 
	IR_send_code(ir_code); /*Muting*/
	_delay_ms(500);
  	
  	i=0;
  	ir_code=CHA_P;
	for(i=0; i< no_ch; i++) /*Change channel upto a specified number*/
	{
		IR_send_code(ir_code);
		_delay_ms(1000);
	}
	
	i=0;
	ir_code=POWER;
	for(i=0; i<pow_cyc; i++) /*Power ON and OFF upto a specified number*/
	{
		IR_send_code(ir_code);
		_delay_ms(1500);
	}
}

int main(void)
{
	clock_prescale_set(clock_div_1); /*CPU clock is set to run at 8 MHz*/
	
	LED_DDR|=(1<<IR_LED); /*IR pin set as output*/
	LED_DDR|=(1<<IND_LED); /*Indicator led is set as output*/
	BUTTON_DDR&=~(1<<BUTTON); /*Just making sure the button pin is in input mode (Not Needed if Port B is not touched)*/
	
	LED_PORT&=~(1<<IR_LED); /*Set the IR port in the low state*/
	LED_PORT&=~(1<<IND_LED); /*Set the indicator LED port as low*/
	
	power_adc_disable(); /*Turning of the power to the ADC*/
	
	while(1)
	{
		sleep();
		/*Start up delay, Number of times to change the channel, 
		 * Number of ON and OFF cycles(USE ODD NUMBERS TO END WITH AN OFF)*/
		
		/*scramble(5,5,5);*/ /*Test cycles*/
		
		initCancelInterrupt0(); /*Used to cancel the scramble operation*/
		scramble(20,10,599); /*This is the scramble routine*/
	}
	return 0;
}
