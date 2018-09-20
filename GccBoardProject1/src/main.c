/* ----------------------------------------------------------------------------
	Oscilloscope Watch DFU Bootloader
    Built with Atmel's ASF wizard, then adding OW custom code

	Gabriel Anzziani
    www.gabotronics.com
-----------------------------------------------------------------------------*/

#include <asf.h>
#include <avr/delay.h>
#include <avr/pgmspace.h>
#include "conf_usb.h"
#include "display.h"
#include "mygccdef.h"
void isp_start_appli(void);

FUSES = {
	.FUSEBYTE0 = 0xFF,  // JTAG not used, ***NEEDS*** to be off
	.FUSEBYTE1 = 0x00,  // Watchdog Configuration
	.FUSEBYTE2 = 0xBF,  // Reset Configuration, BOD off during power down
	.FUSEBYTE4 = 0xF7,  // 4ms Start-up Configuration
	.FUSEBYTE5 = 0xDB,  // No EESAVE on chip erase, BOD sampled when active, 2.4V BO level
};

// Disable writing to the bootloader section
LOCKBITS = (0xBF);

extern uint16_t start_app_key;

// Button definitions
#define K1  3   // K1 - Top 1
#define K2  2   // K2 - Top 2
#define K3  1   // K3 - Top 3
#define KBR 0   // K4 - Bottom Right
#define KUR 6   // K5 - Upper Right
#define KUL 7   // K6 - Upper Left
#define KBL 4   // K7 - Bottom Left
#define KM  5   // K8 - Menu

int main (void) {
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
    PORTC.DIR       = 0b00000000;   // LOGIC
    PORTD.DIR       = 0b00111111;   // D+, D-, LCDVDD, EXTCOMM, LCDIN, LCDDISP, LCDCLK, LCDCS
    PORTD.OUT       = 0b00100000;   // Power to LCD
    PORTE.DIR       = 0b00111111;   // Crystal, crystal, buzzer, buzzer, BATTSENSEPOW, RED, GRN, WHT
    PORTE.OUT       = 0b00000000;
    PORTF.DIR       = 0b00000000;   // Switches
    PORTCFG.MPCMASK = 0xFF;
    PORTF.PIN0CTRL  = 0x18;         // Pull up on pin Port F
    PORTF.INTCTRL   = 0x02;         // PORTA will generate medium level interrupts
    PORTCFG.VPCTRLA = 0x41;         // VP1 Map to PORTE, VP0 Map to PORTB
    PORTCFG.VPCTRLB = 0x32;         // VP3 Map to PORTD, VP2 Map to PORTC

    // CHECK CONDITIONS TO STAY IN BOOTLOADER:
    // Check if the specified pins are being pulled low
	if (((!(PORTF.IN & (1<<KUL))) && (!(PORTF.IN & (1<<KUR))))	// Upper buttons being pressed?
    // If the reset vector is not programmed, there is nothing useful in app flash
	|| pgm_read_word(0) == 0xFFFF	                    // Reset vector not programmed?
    // Check if there was a software reset and if the key is set
    // The key is set only by the bootloader code
	|| (RST.STATUS & RST_SRF_bm && start_app_key!=0x55AA)   // Did the application reset the micro?
	) {	
		// Map interrupt vectors table in bootloader section
		ccp_write_io((uint8_t*)&PMIC.CTRL, PMIC_IVSEL_bm | PMIC_LOLVLEN_bm
				| PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm);
		sysclk_init();
		cpu_irq_enable();
        PR.PRGEN    = 0b01011110;   // Need DMA for LCD SPI transfers
        PR.PRPD     = 0b11101110;   // Need USARTD0 for LCD's SPI, TCD0 to generate 1Hz for LCD's EXTCOMM
        PORTD.REMAP = 0b00000001;   // TC0A - move the location of OC0A from Px0 to Px4
        DMA.CTRL    = 0x80;         // Enable DMA, single buffer, round robin
        TCD0.CTRLB  = 0b00010001;   // CCAEN, Frequency mode
        TCD0.CTRLA  = 0b00000110;   // Prescaler: Clk/256
        TCD0.CCA    = 46875;        // Set 1Hz - Memory LCD needs this signal to avoid burn in
        GLCD_LcdInit();
        lcd_put5x8(30,0,PFSTR("GABOTRONICS"));
        lcd_put5x8(12,5,PFSTR("OSCILLOSCOPE WATCH"));
        lcd_put5x8(36,7,PFSTR("BOOTLOADER"));
        lcd_put5x8(30,9,PFSTR("VERSION 1:1"));
        lcd_put5x8(42,14,PFSTR("TO EXIT"));
        lcd_put5x8(6,15,PFSTR("<= PRESS  BUTTONS =>"));
        dma_display();
		VPORT1.OUT = 0b00100000;			// Buzzer
		for(uint16_t t=10; t<1000; t+=4) {	// Intro Sound to signal bootloader mode
			for(uint16_t n=t; n<1010; n++) { _delay_us(5); }
			VPORT1.OUT ^= 0b00110000;
		}

		udc_start();    // Start the USB Device stack
		udc_attach();   // Attach device to the bus when possible
        
        // Stay here while the bootloader does its job...
		while (true) {	
            // The red LED is turned on in usb_device.c whenever there is a USB bus event interrupt
            // The green LED is turned on in udc.c whenever there is a USB SETUP request
			VPORT1.OUT = 0;                 // Turn off LEDs
            // Exit bootloader if the bottom buttons are being pressed
			if( (!(PORTF.IN & (1<<KBL))) && (!(PORTF.IN & (1<<KBR))) ) {
                GLCD_LcdOff();
                isp_start_appli();
            }
            // The bootloader host can also request to start the application
		}
	}
	if(start_app_key==0x55AA) {             // Did the bootloader reset the micro?
		RST.STATUS = RST_SRF_bm;            // Clear software reset flag
		start_app_key = 0;		            // Clear key
	}
	EIND = 0x00;                            // Clear EIND to ensure jump to beginning of flash
	void (*reset_vect)( void ) = 0x000000;  // Application reset vector address
	reset_vect();                           // Jump to application
}
