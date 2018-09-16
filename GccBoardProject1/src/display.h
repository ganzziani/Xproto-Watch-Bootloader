/* ----------------------------------------------------------------------------
	Basic driver for the LS013B7DH03.c Memory LCD
    Refer to document: LS013B7DH03 Application Information
	Gabriel Anzziani
    www.gabotronics.com
-----------------------------------------------------------------------------*/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

 // LCD connected on PORTD
 #define LCD_CS		    0           // Chip select
 #define LCD_DISP       2           // DISPLAY ON/OFF Signal
 #define EXTCOMM        4           // PORTD
 #define LCDVDD         5           // LCD VDD
 #define LCD_CTRL       VPORT3.OUT  // PORTD is mapped to VPORT3 on main

// LS013B7DH03 Commands
// M0 M1 M2 DMY DMY DMY DMY DMY

// M0: H' the module enters Dynamic Mode, where pixel data will be updated.
// M0: 'L' the module remains in Static Mode, where pixel data is retained

// M1: VCOM When M1 is 'H' then VCOM = 'H' is output. If M1 is 'L' then VCOM = 'L' is output.
// When EXTMODE = 'H', M1 value = XX (don’t care)

// M2 CLEAR ALL When M2 is 'L' then all flags are cleared. When a full display clearing is required,
// set M0 and M2 = HIGH and set all display data to white.

#define     STATIC_MODE     0b00000000      // Set Static Mode
#define     CLEAR_ALL       0b00100000      // Clear screen
#define     DYNAMIC_MODE    0b10000000      // Write Single or Multiple Line

#define DISPLAY_DATA_SIZE	(2048+128*2) // Data + Addresses + Trailers

typedef struct {
    uint8_t     spidata[2];
    uint8_t     buffer[DISPLAY_DATA_SIZE];
} Disp_data;

extern Disp_data Disp_send;
extern uint8_t u8CursorX, u8CursorY;

#define lcd_goto(x,y) { u8CursorX=(x); u8CursorY=(y); }

/* Function Prototype(s) */
void GLCD_LcdInit(void);
void GLCD_LcdOff(void);
void clr_display(void);
void write_display(uint8_t data);
void lcd_put5x8 (uint8_t x, uint8_t y, uint32_t ptr);
void putchar5x8(char u8Char);
void dma_display(void);
void LcdInstructionWrite (unsigned char);
    
#endif
