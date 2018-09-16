Xproto-Watch-Bootloader
=======================

Bootloader for the ATXMEGA256A3U.  
The accompanying software on the PC is Atmel FLIP, or any other software that uses the FLIP protocol version 2, like AVRDUDE.

The project is developed using Atmel Studio 7 using the ASF Wizard.  
It uses the USB Device dfu_atmel module.

The following modifications were done to the ASF code:  
Compiler Optimization set to -Os

isp.c -> bool isp_is_security(void):  
Fixed bug, a register had an incorrect name:  
return !(NVM.LOCK_BITS&NVM_LOCKBITS_LB1_bm);

usb_device.c -> ISR(USB_BUSEVENT_vect):  
VPORT1.OUT = 0x04;    // Red LED  
Turns on red LED when there is a USB Bus event.  
Thus, the red LED indicates if the USB connection is active.

udc.c -> bool udc_process_setup(void):  
VPORT1.OUT = 0x02;    // Green LED  
Turns on green LED when there is a USB setup request.  
The green LED indicates if the host is actively communicating with the device, for example, when the firmware is being updated.

conf_click.h:  
Set the Internal "32MHZ" oscillator to run at 48MHz for the USB module.  
Set the CPU clock at 24MHz by setting PSA to 2  
//#define CONFIG_SYSCLK_SOURCE              SYSCLK_SRC_RC2MHZ  
#define CONFIG_SYSCLK_SOURCE                SYSCLK_SRC_RC32MHZ  
#define CONFIG_SYSCLK_PSADIV                SYSCLK_PSADIV_2  
#define CONFIG_OSC_RC32_CAL                 48000000UL  
#define CONFIG_OSC_AUTOCAL_RC32MHZ_REF_OSC  OSC_ID_USBSOF

conf_isp.h:  
Hardware condition to enter bootloader (although these definitions are not used in my code)  
#define ISP_PORT_DIR      PORTF_DIR  
#define ISP_PORT_PINCTRL  PORTF_PIN7CTRL  
#define ISP_PORT_IN       PORTF_IN  
#define ISP_PORT_PIN      7

conf_usb.h:  
#define  USB_DEVICE_PRODUCT_ID            USB_PID_ATMEL_DFU_ATXMEGA256A3U  
//#define  UDI_DFU_ATMEL_PROTOCOL_2_SPLIT_ERASE_CHIP    // This causes compilation errors  
//#define  UDI_DFU_SMALL_RAM                            // All of the micro's RAM is available during bootloader  
#define    UDD_NO_SLEEP_MGR  
#define  USB_DEVICE_MANUFACTURE_NAME      "Gabotronics"  
#define  USB_DEVICE_PRODUCT_NAME          "Oscilloscope Watch"  

main.c:  
Hardware initialization and code to enter and exit the bootloader.