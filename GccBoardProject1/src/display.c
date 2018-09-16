/* ----------------------------------------------------------------------------
	Basic driver for the LS013B7DH03.c Memory LCD
    Refer to document: LS013B7DH03 Application Information
	Gabriel Anzziani
    www.gabotronics.com
-----------------------------------------------------------------------------*/
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "display.h"
#include "mygccdef.h"

Disp_data Disp_send;                // Data buffer
uint8_t   u8CursorX, u8CursorY;     // Text cursor position

const unsigned char Font5x8[][5] PROGMEM = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},     // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},     // 1
    {0x62, 0x51, 0x49, 0x49, 0x46},     // 2
    {0x22, 0x41, 0x49, 0x49, 0x36},     // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},     // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},     // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},     // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},     // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},     // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E},     // 9
    {0x00, 0x00, 0x60, 0x00, 0x00},     // .
    {0x00, 0xAC, 0x6C, 0x00, 0x00},     // ;
    {0x08, 0x14, 0x22, 0x41, 0x00},     // <
    {0x14, 0x14, 0x14, 0x14, 0x14},     // =
    {0x41, 0x22, 0x14, 0x08, 0x00},     // >
    {0x02, 0x01, 0x51, 0x09, 0x06},     // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E},     // @
    {0x7E, 0x09, 0x09, 0x09, 0x7E},     // A
    {0x7F, 0x49, 0x49, 0x49, 0x36},     // B
    {0x3E, 0x41, 0x41, 0x41, 0x22},     // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C},     // D
    {0x7F, 0x49, 0x49, 0x49, 0x41},     // E
    {0x7F, 0x09, 0x09, 0x09, 0x01},     // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A},     // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F},     // H
    {0x00, 0x41, 0x7F, 0x41, 0x00},     // I
    {0x20, 0x40, 0x41, 0x3F, 0x01},     // J
    {0x7F, 0x08, 0x14, 0x22, 0x41},     // K
    {0x7F, 0x40, 0x40, 0x40, 0x40},     // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},     // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F},     // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E},     // O
    {0x7F, 0x09, 0x09, 0x09, 0x06},     // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E},     // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46},     // R
    {0x26, 0x49, 0x49, 0x49, 0x32},     // S
    {0x01, 0x01, 0x7F, 0x01, 0x01},     // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F},     // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F},     // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F},     // W
    {0x63, 0x14, 0x08, 0x14, 0x63},     // X

};


// LCD Initialization
void GLCD_LcdInit(void)	{
    // Initialize USARTD0 for OLED
    // FBAUD = FPER/(2*(BSEL+1))    - CPU is at 24MHz
    USARTD0.BAUDCTRLA = 255;	        // SPI clock rate for display at 46.875kHz
    // A very slow SPI creates a "slide in" effect when displaying the screen
    USARTD0.CTRLC     = 0xC0;   		// Master SPI mode,
    USARTD0.CTRLB     = 0x08;   		// Enable TX+
    // Recommended power up sequence
    setbit(LCD_CTRL,LCDVDD);            // Power for display
    _delay_ms(1);
    clrbit(LCD_CTRL, LCD_DISP);         // DISP OFF
    LcdInstructionWrite(CLEAR_ALL);     // Clear Screen
    setbit(LCD_CTRL, LCD_DISP);         // DISP ON
    // Prepare buffer for future writes
    // Writing Multiple Lines to LCD
    // First Byte:  Command
    // Lines:       Line Number, Data (16 bytes), Trailer
    // Last Byte:   Trailer
    Disp_send.spidata[0] = DYNAMIC_MODE;    // Command
    Disp_send.spidata[1] = 128;             // Address of line 1 ('1' bit reversed)
    for(uint8_t i=0; i<128; i++) {
        uint8_t r=i+2;                      // Address of line 2
        REVERSE(r);                         // The address needs to be bit reversed
        // Each line needs 18 bytes of data, prepare the first two bytes:
        Disp_send.buffer[16+i*18] = i;      // Trailer
        Disp_send.buffer[17+i*18] = r;      // Address (or Trailer of last line)
    }
    Disp_send.buffer[DISPLAY_DATA_SIZE-1] = STATIC_MODE;
}

// Turn off LCD
void GLCD_LcdOff(void)	{
	cli();  // disable interrupts
    LcdInstructionWrite(CLEAR_ALL);     // Clear Screen
    clrbit(LCD_CTRL, LCD_DISP);         // DISP OFF
    TCD0.CTRLB = 0b00000000;            // Disable 1Hz signal
    _delay_us(30);                      // TCOM
	sei();
    clrbit(VPORT3.OUT, 2);              // Power down board
}

// Send instruction to the LCD thru the SPI
void LcdInstructionWrite (uint8_t u8Instruction) {
    setbit(LCD_CTRL, LCD_CS);			// Select
    _delay_us(6);                       // tsSCS
    USARTD0.DATA= u8Instruction;        // Send command
    while(!testbit(USARTD0.STATUS,6));  // Wait until transmit done
    setbit(USARTD0.STATUS,6);
    USARTD0.DATA = 0;                   // Trailer (dummy data)
    while(!testbit(USARTD0.STATUS,6));  // Wait until transmit done
    setbit(USARTD0.STATUS,6);    
    _delay_us(2);                       // thSCS
    clrbit(LCD_CTRL, LCD_CS);			// Select
}

// Transfer display buffer to LCD - 2306 bytes will be sent
void dma_display(void) {
    setbit(LCD_CTRL, LCD_CS);			// Select
    setbit(DMA.CH2.CTRLA,6);            // reset DMA CH2
    _delay_us(6);                       // tsSCS
    DMA.CH2.ADDRCTRL  = 0b00010000;     // Increment source, Destination fixed
    DMA.CH2.TRFCNT    = DISPLAY_DATA_SIZE+2;
    DMA.CH2.DESTADDR0 = (((uint16_t) &USARTD0.DATA)>>0*8) & 0xFF;
    DMA.CH2.DESTADDR1 = (((uint16_t) &USARTD0.DATA)>>1*8) & 0xFF;
    DMA.CH2.TRIGSRC   = DMA_CH_TRIGSRC_USARTD0_DRE_gc;
    DMA.CH2.SRCADDR0  = (((uint16_t)(Disp_send.spidata))>>0*8) & 0xFF;
    DMA.CH2.SRCADDR1  = (((uint16_t)(Disp_send.spidata))>>1*8) & 0xFF;
    DMA.CH2.CTRLB     = 0b00010001;     // Low priority interrupt on complete
    DMA.CH2.CTRLA     = 0b10000100;     // no repeat, 1 byte burst
}

// DMA done, now at most 2 bytes are left to be sent
ISR(DMA_CH2_vect) {
    _delay_ms(8);
    clrbit(LCD_CTRL, LCD_CS);			    // DeSelect
    setbit(DMA.INTFLAGS, 0);
}

// OR byte on display buffer
void write_display(uint8_t data) {
    uint8_t x;
    x=u8CursorX++;
    x=127-x;
    REVERSE(data);
    Disp_send.buffer[((uint16_t)(x*18)) + (u8CursorY)] |= data;
}

// Print a character on the LCD at the current position
void putchar5x8(char u8Char) {
    if(u8Char==' ') {
        for(uint8_t u8CharColumn=0; u8CharColumn<5; u8CharColumn++) {
            write_display(0);
        }
    }
    else {
        uint32_t pointer;
	    uint8_t data,u8CharColumn=0;
	    pointer = GET_FAR_ADDRESS(Font5x8)+(u8Char-'0')*(5);    // Fonts start at char '0'
        if(u8Char!='\n') {
       	    /* Draw a char */
    	    while (u8CharColumn < 5)	{
                data = pgm_read_byte_far(pointer++);
		        write_display(data);
		        u8CharColumn++;
	        }
        }
    }        
    // Insert a space before next letter
	write_display(0);
}

// Prints a string from program memory at the specified position
void lcd_put5x8 (uint8_t x, uint8_t y, uint32_t ptr) {
    u8CursorX = x;
    u8CursorY = y;
    char c;
    while ((c=pgm_read_byte_far(ptr++)) != 0x00) {
        putchar5x8(c);
    }
}
