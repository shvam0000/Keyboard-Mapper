#include "usbkeyboard.h"

#include <stdio.h>
#include <stdlib.h> 

/* References on libusb 1.0 and the USB HID/keyboard protocol
 *
 * http://libusb.org
 * https://web.archive.org/web/20210302095553/https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
 *
 * https://www.usb.org/sites/default/files/documents/hid1_11.pdf
 *
 * https://usb.org/sites/default/files/hut1_5.pdf
 */

/*
 * Find and return a USB keyboard device or NULL if not found
 * The argument con
 * 
 */
struct libusb_device_handle *openkeyboard(uint8_t *endpoint_address) {
  libusb_device **devs;
  struct libusb_device_handle *keyboard = NULL;
  struct libusb_device_descriptor desc;
  ssize_t num_devs, d;
  uint8_t i, k;
  
  /* Start the library */
  if ( libusb_init(NULL) < 0 ) {
    fprintf(stderr, "Error: libusb_init failed\n");
    exit(1);
  }

  /* Enumerate all the attached USB devices */
  if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
    fprintf(stderr, "Error: libusb_get_device_list failed\n");
    exit(1);
  }

  /* Look at each device, remembering the first HID device that speaks
     the keyboard protocol */

  for (d = 0 ; d < num_devs ; d++) {
    libusb_device *dev = devs[d];
    if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
      fprintf(stderr, "Error: libusb_get_device_descriptor failed\n");
      exit(1);
    }

    if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
      struct libusb_config_descriptor *config;
      libusb_get_config_descriptor(dev, 0, &config);
      for (i = 0 ; i < config->bNumInterfaces ; i++)	       
	for ( k = 0 ; k < config->interface[i].num_altsetting ; k++ ) {
	  const struct libusb_interface_descriptor *inter =
	    config->interface[i].altsetting + k ;
	  if ( inter->bInterfaceClass == LIBUSB_CLASS_HID &&
	       inter->bInterfaceProtocol == USB_HID_KEYBOARD_PROTOCOL) {
	    int r;
	    if ((r = libusb_open(dev, &keyboard)) != 0) {
	      fprintf(stderr, "Error: libusb_open failed: %d\n", r);
	      exit(1);
	    }
	    if (libusb_kernel_driver_active(keyboard,i))
	      libusb_detach_kernel_driver(keyboard, i);
	    libusb_set_auto_detach_kernel_driver(keyboard, i);
	    if ((r = libusb_claim_interface(keyboard, i)) != 0) {
	      fprintf(stderr, "Error: libusb_claim_interface failed: %d\n", r);
	      exit(1);
	    }
	    *endpoint_address = inter->endpoint[0].bEndpointAddress;
	    goto found;
	  }
	}
    }
  }

 found:
  libusb_free_device_list(devs, 1);

  return keyboard;
}

char decode(struct usb_keyboard_packet packet, int keycode_number){
	char output_character;

	// Lowercase characters
	if(packet.modifiers == 0x00){
		switch(packet.keycode[keycode_number]){
			case 0x04: 
				output_character = 'a';
				break;
			case 0x05: 
				output_character = 'b';
				break;
			case 0x06: 
				output_character = 'c';
				break;
			case 0x07: 
				output_character = 'd';
				break;
			case 0x08: 
				output_character = 'e';
				break;
			case 0x09: 
				output_character = 'f';
				break;
			case 0x0a: 
				output_character = 'g';
				break;
			case 0x0b: 
				output_character = 'h';
				break;
			case 0x0c: 
				output_character = 'i';
				break;
			case 0x0d: 
				output_character = 'j';
				break;
			case 0x0e: 
				output_character = 'k';
				break;
			case 0x0f: 
				output_character = 'l';
				break;
			case 0x10: 
				output_character = 'm';
				break;
			case 0x11: 
				output_character = 'n';
				break;
			case 0x12: 
				output_character = 'o';
				break;
			case 0x13: 
				output_character = 'p';
				break;
			case 0x14: 
				output_character = 'q';
				break;
			case 0x15: 
				output_character = 'r';
				break;
			case 0x16: 
				output_character = 's';
				break;
			case 0x17: 
				output_character = 't';
				break;
			case 0x18: 
				output_character = 'u';
				break;
			case 0x19: 
				output_character = 'v';
				break;
			case 0x1a: 
				output_character = 'w';
				break;
			case 0x1b: 
				output_character = 'x';
				break;
			case 0x1c: 
				output_character = 'y';
				break;
			case 0x1d: 
				output_character = 'z';
				break;
			case 0x2e:
				output_character = '=';
				break;
			case 0x2d:
				output_character = '-';
				break;
			case 0x2c: 
				output_character = ' ';
				break;
			case 0x37: 
				output_character = '.';
				break;
			case 0x36: 
				output_character = ',';
				break;
			case 0x34:
				output_character = '\'';
				break;
			case 0x2f:
				output_character = '[';
				break;
			case 0x30:
				output_character = ']';
				break;
			case 0x38:
				output_character = '/';
				break;
			case 0x28:
				output_character = '\n';
				break;
			case 0x31:
				output_character = '\\';
				break;
			case 0x33:
				output_character = ';';
				break;
			case 0x1e:
				output_character = '1';
				break;		
			case 0x1f:
				output_character = '2';
				break;
			case 0x20:
				output_character = '3';
				break;
			case 0x21:
				output_character = '4';
				break;
			case 0x22:
				output_character = '5';
				break;
			case 0x23:
				output_character = '6';
				break;
			case 0x24:
				output_character = '7';
				break;
			case 0x25:
				output_character = '8';
				break;
			case 0x26:
				output_character = '9';
				break;
			case 0x27:
				output_character = '0';
				break;
			case 0x35:
				output_character = '`';
				break;
			case 0x2a:
				output_character = '\b';
				break;
			default: output_character = '\0';
		}

	}
	else if(packet.modifiers == 0x02 || packet.modifiers == 0x20){
		switch(packet.keycode[keycode_number]){
			case 0x04: 
				output_character = 'A';
				break;
			case 0x05: 
				output_character = 'B';
				break;
			case 0x06: 
				output_character = 'C';
				break;
			case 0x07: 
				output_character = 'D';
				break;
			case 0x08: 
				output_character = 'E';
				break;
			case 0x09: 
				output_character = 'F';
				break;
			case 0x0a: 
				output_character = 'G';
				break;
			case 0x0b: 
				output_character = 'H';
				break;
			case 0x0c: 
				output_character = 'I';
				break;
			case 0x0d: 
				output_character = 'J';
				break;
			case 0x0e: 
				output_character = 'K';
				break;
			case 0x0f: 
				output_character = 'L';
				break;	
			case 0x31:
				output_character = '|';
				break;
			case 0x10: 
				output_character = 'M';
				break;
			case 0x11: 
				output_character = 'N';
				break;
			case 0x12:
				output_character = 'O';
				break;
			case 0x13:
				output_character = 'P';
				break;
			case 0x14: 
				output_character = 'Q';
				break;
			case 0x15: 
				output_character = 'R';
				break;
			case 0x16: 
				output_character = 'S';
				break;
			case 0x17: 
				output_character = 'T';
				break;
			case 0x18: 
				output_character = 'U';
				break;
			case 0x19: 
				output_character = 'V';
				break;
			case 0x1a: 
				output_character = 'W';
				break;
			case 0x1b: 
				output_character = 'X';
				break;
			case 0x1c: 
				output_character = 'Y';
				break;
			case 0x1d: 
				output_character = 'Z';
				break;
			case 0x35:
				output_character = '~';
				break;
			case 0x2c: 
				output_character = ' ';
				break;	
			case 0x33:
				output_character = ':';
				break;
			case 0x36:
			       	output_character = '<';
		       		break;
			case 0x38:
				output_character = '?';
				break;
			case 0x1e:
				output_character = '!';
				break;		
			case 0x1f:
				output_character = '@';
				break;
			case 0x20:
				output_character = '#';
				break;
			case 0x21:
				output_character = '$';
				break;
			case 0x22:
				output_character = '%';
				break;
			case 0x23:
				output_character = '^';
				break;
			case 0x24:
				output_character = '&';
				break;
			case 0x25:
				output_character = '*';
				break;
			case 0x26:
				output_character = '(';
				break;
			case 0x27:
				output_character = ')';
				break;
			case 0x37:
				output_character = '>';
				break;
			case 0x34:
				output_character = '"';
				break;	
			case 0x28:
				output_character = '\n';
				break;
			case 0x2f:
				output_character = '{';
				break;
			case 0x30:
				output_character = '}';
				break;
			case 0x2e:
				output_character = '+';
				break;
			case 0x2d:
				output_character = '_';
				break;
			case 0x2a:
				output_character = '\b';
				break;
			default: output_character = '\0';
		}
	}
	return output_character;
}
