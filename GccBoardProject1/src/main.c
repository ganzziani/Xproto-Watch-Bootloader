/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <asf.h>
#include <avr/delay.h>
#include "conf_usb.h"
void isp_start_appli(void);

extern uint16_t start_app_key;

int main (void)
{
    // PORTS CONFIGURATION
    PORTA.DIR       = 0b00101110;   // 1.024V, BATTSENSE, x, CH1, x, x, x, 2.048V
    PORTA.PIN4CTRL  = 0x07;         // Input Disable on pin PA4
    PORTA.PIN6CTRL  = 0x07;         // Input Disable on pin PA6
    PORTA.PIN7CTRL  = 0x07;         // Input Disable on pin PA7
    PORTB.DIR       = 0b11000011;   // LOGICDIR, ANPOW, CH2, 1.024V, AWG, TRIG, x, x
    PORTB.PIN2CTRL  = 0x01;         // Sense rising edge (Freq. counter)
    PORTB.PIN3CTRL	= 0x07;         // Input Disable on pin PB3
    PORTB.PIN4CTRL  = 0x07;         // Input Disable on pin PB4
    PORTB.PIN5CTRL  = 0x07;         // Input Disable on pin PB5
    PORTB.OUT       = 0b10000000;   // Logic port as input, Analog power off
    //PORTC.DIR = 0b00000000;       // LOGIC, register initial value is 0
    PORTD.DIR       = 0b00111111;   // D+, D-, LCDVDD, EXTCOMM, LCDIN, LCDDISP, LCDCLK, LCDCS
    PORTD.OUT       = 0b00100000;   // Power to LCD
    PORTE.DIR       = 0b00111111;   // Crystal, crystal, buzzer, buzzer, BATTSENSEPOW, RED, GRN, WHT
    // PORTE.OUT       = 0x20;
    // PORTF.DIR       = 0x00;      // Switches
    PORTCFG.MPCMASK = 0xFF;
    PORTF.PIN0CTRL  = 0x18;         // Pull up on pin Port F
    PORTF.INTCTRL   = 0x02;         // PORTA will generate medium level interrupts
    PORTCFG.VPCTRLA = 0x41;         // VP1 Map to PORTE, VP0 Map to PORTB
    PORTCFG.VPCTRLB = 0x32;         // VP3 Map to PORTD, VP2 Map to PORTC

	if (!(PORTF.IN & (1<<7))		// If the specified pin is pulled LOW
	|| pgm_read_word(0) == 0xFFFF	// Get the value of the reset vector. If it's unprogrammed, we know
	// there's nothing useful in app flash
	|| (RST.STATUS & RST_SRF_bm && start_app_key!=0x55AA) // If the app code reset into the bootloader
	) {	
		// Map interrupt vectors table in bootloader section
		ccp_write_io((uint8_t*)&PMIC.CTRL, PMIC_IVSEL_bm | PMIC_LOLVLEN_bm
				| PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm);
		sysclk_init();
		cpu_irq_enable();
		VPORT1.OUT = 0b00100000;			// Buzzer
		for(uint16_t t=10; t<1000; t+=4) {	// Intro Sound
			for(uint16_t n=t; n<1010; n++) { _delay_us(100); }
			VPORT1.OUT ^= 0b00110000;
		}

		udc_start();
		udc_attach();
		while (true) {	
			VPORT1.OUT = 0;
			if( (!(PORTF.IN & (1<<4))) && (!(PORTF.IN & (1<<4))) ) isp_start_appli();
		}
	}

	if(start_app_key==0x55AA) {
		RST.STATUS = RST_SRF_bm;
		start_app_key = 0;		
	}
	EIND = 0x00;
	void (*reset_vect)( void ) = 0x000000;
	reset_vect();
}
