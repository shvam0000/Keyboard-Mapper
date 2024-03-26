#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128
#define SCREEN_WIDTH 64
#define INPUT_START_ROW 21

/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

char* filter_cursor_from_string(char* input, int cursor_index);

// Screen handlers
void clear_output_screen();
void clear_screen();
void split_screen();
void clear_input_display();
void toggle_cursor_state();
void display_buffer(char* inputBuffer);

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;
  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];
  char ascii;
  char input_buffer[BUFFER_SIZE] = "|\0";
  int cursor_idx = 0;	
  int shift_pressed = 0;
  int shift_state_change = 0;
  int shift_state_prev = 0;
  int last_key, last_key_prev;
  int no_print = 0;

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  /* Draw rows of asterisks across the top and bottom of the screen */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 0, col);
    fbputchar('*', 23, col);
  }

  fbputs("Hello CSEE 4840 World!", 4, 10);

  clear_screen();
  split_screen();

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  for (;;) {    
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
		      packet.keycode[1]);
      printf("%s\n", keystate);

      last_key_prev = last_key;
      last_key = 0;
      while(packet.keycode[last_key]){
	      last_key++;
      }
      last_key -= 1;

      /* Do not print a character when a character key is released */
      if(last_key_prev - last_key == 1){
	      no_print = 1;
      }
      else{
	      no_print = 0;
      }

      ascii = decode(packet, last_key);

       /* detect shift state change */
      if((shift_state_prev && (packet.modifiers == 0x00)) || (!shift_state_prev && (packet.modifiers == 0x02))){
	      shift_state_change = 1;
      }
      else{
	      shift_state_change = 0;
      }
      shift_state_prev = (packet.modifiers == 0x02);
     
      /* Reset shift_pressed flag*/
      if(packet.modifiers == 0x00 || (packet.modifiers == 0x02 && !packet.keycode[0])){
	     shift_pressed = 0;
      }
      else if(packet.modifiers == 0x02 && packet.keycode[0]){ // what does this do?
	      //shift_pressed = 1;
      }
	  	
	/* All buffer operations*/
	if(!no_print && !shift_pressed && !shift_state_change && packet.keycode[0] != 0x00){
		char temp_string[BUFFER_SIZE] = "";
		strcpy(temp_string, input_buffer);

		/* backspace */
		if(ascii  == '\b' && cursor_idx > 0 && !shift_pressed){
			strcpy(temp_string, input_buffer);

			for(int i = cursor_idx-1; i < strlen(temp_string); i++){
				if( i>=0){
				temp_string[i] = temp_string[i+1];
				}
			}	

			temp_string[strlen(temp_string)] = '\0';
			
			printf("TEMP STRING %s", temp_string);
			
			cursor_idx = cursor_idx - 1;
			strcpy(input_buffer, temp_string);
			
			printf("INPUT BUFFER %s", input_buffer);
		}
		
		/* rightarrow */
		else if(packet.keycode[0] == 0x4f && cursor_idx < strlen(input_buffer) - 1){
			temp_string[cursor_idx+1] = '|';
			temp_string[cursor_idx] = input_buffer[cursor_idx+1];

			cursor_idx++;
		}

		/* leftarrow */
		else if(packet.keycode[0] == 0x50 && cursor_idx > 0){
			temp_string[cursor_idx-1] = '|';
			temp_string[cursor_idx] = input_buffer[cursor_idx-1];

			cursor_idx--;
		}

		/* write character to the buffer */
		else if(strlen(input_buffer) < BUFFER_SIZE-1 && ascii != '\0' && cursor_idx != BUFFER_SIZE && packet.keycode[1] != 0x28 && ascii != '\b'){
			printf("In write char \n");
			temp_string[cursor_idx] = ascii;
			for(int i=0; i+cursor_idx < strlen(input_buffer);i++){
				temp_string[cursor_idx+i+1] = input_buffer[cursor_idx+i];
			}
			if(cursor_idx == strlen(input_buffer)-1){
				temp_string[strlen(input_buffer)] = '|';
			}
			cursor_idx++;
		}
		strcpy(input_buffer, temp_string);
	}
	display_buffer(input_buffer);

	/* ESC pressed? */ 
      	if (packet.keycode[0] == 0x29) {
		clear_screen();
		split_screen();
		break;
      	} 
       
	/* Return pressed */      
       	if ((packet.keycode[0] == 0x28 && !shift_pressed) || (packet.keycode[1] == 0x28)) {
		char output[BUFFER_SIZE];
		for(int i = 0; i < cursor_idx-1; i++){
		 output[i] = input_buffer[i];
		 }
		 for(int i = cursor_idx-1; i < strlen(input_buffer)-2; i++){
			 output[i] = input_buffer[i+2];
		    }
		 output[strlen(input_buffer)-1] = '\0';
		strcpy(input_buffer,output);
		printf("The buffer = %s \n", output);
		printf("The buffer length = %d \n", strlen(input_buffer));
      		write(sockfd,input_buffer, strlen(input_buffer));
		strcpy(input_buffer, "|");

		/* Clearing the row after printing */
		clear_input_display();
		display_buffer(input_buffer);
		cursor_idx = 0;
      	}
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  int row = 1;
  /* Receive data */
  while ((n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n-1] = '\0';
    printf("%s", recvBuf);
    for(int i = 0; i < strlen(recvBuf); i++){
	    fbputchar(recvBuf[i], row + i/SCREEN_WIDTH, i%SCREEN_WIDTH);
    }
    if(strlen(recvBuf) > 63){
    row++;
    }
    row++;

    /* clear if full */
    if (row >=20) {
    	clear_output_screen();
	split_screen();
	row = 1;
    }
  }
  return NULL;
}

void clear_screen()
{
/* Draw rows of asterisks across the top and bottom of the screen */
  for (int col = 0 ; col < 64 ; col++) {
	  for(int row = 1; row < 23; row++){
		  fbputchar(' ', row, col);
	  }
  }
}


void clear_output_screen()
{
  for (int col = 0; col<64; col++) {
	for (int row = 1; row<20; row++) {
		fbputchar(' ', row, col);
	}
  }
}

void split_screen(){
	for(int col = 0; col <= 64; col++){
		fbputchar('=', 20, col);
	}
}

void clear_input_display() {
	for (int col = 0; col < 64; col++) {
		for (int i=INPUT_START_ROW; i<=22; i++) {
			fbputchar(' ', i, col);
		}
	}
}

void display_buffer(char* input_buffer){
	clear_input_display();
	for(int i = 0; i < strlen(input_buffer); i++){
			fbputchar(input_buffer[i], INPUT_START_ROW + i/SCREEN_WIDTH, i%SCREEN_WIDTH);
	}
}

