/*****
 terminal code used to send ASCII to USB PIO cable. From:
 http://www.st.ewi.tudelft.nl/~gemund/Courses/In4073/Resources/myterm.c

 Keypad handling routines for Embedded Systems Design coursework
 22/12/2011 - Pete Hemery
 0.2 - User Interface state machine
*****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>   //file descriptors
#include <ctype.h>
#include <termios.h>  //terminal
#include <assert.h>
#include <time.h>
#include <sys/types.h>  //threads
#include <pthread.h>
#include "keypad_rewrite.h"

pthread_t keypad_thread;
BYTE alive = TRUE;  // Exit condition of while loops
char button = FALSE;  // Button pressed 1-16 or -1 for multiple buttons
int fd_RS232;   // Terminal File descriptor
int state = WAITING_LOGGED_OUT;  // State machine variable
//int state = MENU_SELECT;
int logged_in = FALSE;  // Client connected to server

BYTE blocking = TRUE;
BYTE padding = TRUE;

char authentication = FALSE;
BYTE playing = FALSE;
BYTE digits[COLSX] = {0x73,0x79,0x78,0x79};

char input_buffer[BUF_SIZE] = {0};
char display_buffer[BUF_SIZE] = {0};

int display_flag = WAITING;
int cursor_blink = FALSE;
int cur_pos = 0;

/*------------------------------------------------------------------------------
 * main -- Open terminal and launch keypad thread.
 *         Check for button input and respond appropriately
 *------------------------------------------------------------------------------
 */
int main (void) {
//  int i;
  int ret;
  char *emergency="! EMERGENCY !";
  char *welcome="Welcome.";
  char *enter_pin="Please Enter PIN.";
  char button_read = FALSE;  // Local snapshot of Global 'Button'
  char prev_button = 0;

  /* error handling - reset LEDs */
  atexit(closing_time);
  signal(SIGINT, closing_time);

  term_initio();
  rs232_open();
  
  setup_ports();
  ret = pthread_create( &keypad_thread, NULL, keypad, NULL);
  
  strcpy(display_buffer,"1.Volume");
  blocking = TRUE;
  display_flag = CHANGED;
  
  while(alive){
//    display_string(welcome,PADDED,NOT_BLOCKING);
    if(button_read = button){
      if(prev_button != button_read){
        if(blocking){
          while(blocking == TRUE && alive);
        }
        else{
          if(button_read == 'F'){
            if(++cur_pos > COLSX){
              cur_pos = COLSX;
            }
          }
          else if(button_read == 'B'){
            if(--cur_pos < 0){
              cur_pos = 0;
            }
          }
          else{
            printf("button: %c\n",button_read);
            sprintf(display_buffer,"Button %c",button_read);        
            display_flag = CHANGED;
          }
        }
      }
    }
    prev_button = button_read;
  }

  pthread_join(keypad_thread, NULL);

  term_exitio();
  rs232_close();
  return 0;
}


/*------------------------------------------------------------------------------
 * keypad thread - this function continiously outputs to LEDs and reads buttons
 *------------------------------------------------------------------------------
 */
void read_button(int col, char in){
  int row;
  int out = 0;
  static char temp;
  static BYTE keypresses = 0;
  
  if(in > 0x40) {   // Convert output from ASCII to binary
    out |= (0x0F & (in-0x07)); // A-F
  }
  else{
    out |= (0x0F & (in));      // 0-9
  }

  for(row=0; row<ROWSX; row++){     // Scan the rows for key presses
    if((out >> row) & 0x01){
      keypresses++;
      temp = uitab[((col+1)+(row*4))];// Set the detected button
    }
  }

  if(col == COLSX-1){       // After reading all the columns
    switch(keypresses){
      case 0:
        button=FALSE; // No key press detected
        break;
      case 1:
        button=temp; // Write ASCII value from uitab
        break;
      default:
        button=ERROR;       // Multiple keys pressed
        break;
    }
    keypresses = 0;
  }
  return;
} 

void update_display(void){
  int i;
  static int started_waiting = TRUE;
  static int block = FALSE;
  static int offset = 0;
  static int finished = TRUE;
  static int pad = 0;
  static BYTE saved_digits[COLSX] = {0x73,0x79,0x78,0x79};
  static int prev_cur_pos = 0;
  int cur_read = 0;

  switch(display_flag){
    case CHANGED:
      cursor_blink = FALSE;
      switch(finished){
        case FALSE:
          switch(block){
            case TRUE:
              printf("Blocked\n");
              break;
            case FALSE:
              display_flag = WRITING;
              for(i=0;i<COLSX;i++){
                digits[i] = 0;
              }
              block = blocking;
              offset = 0;
              break;
            default:
              break;
          }
          break;
        case TRUE:
          display_flag = WRITING;
          finished = FALSE;
          block = blocking;
          offset = 0;
          break;
        default:
          break;
      }
      break;
      
    case WRITING:
      switch(finished){
        case FALSE:
          if(offset == 0){
            for(i=0;i<COLSX;i++){
              digits[i] = digits[i+1];
            }
            digits[3] = 0;                 // Make space for new string
          }
          if(display_buffer[offset] != 0){
            while(display_buffer[offset] == '.' && display_buffer[offset] != 0){
              digits[3] |= CURSOR_VALUE;
              offset++;
            }
            for(i=0;i<3;i++){
              digits[i] = digits[i+1];
            }
            digits[3] = display_char(display_buffer[offset]);
            offset++;
          }
          else{
            finished = TRUE;
            for(i=0;i<3;i++){
              digits[i] = digits[i+1];
            }
            digits[3] = 0;  // Space between end of string and restored digits
           
            if(padding == TRUE){
              pad = 4;
            }
          }
          break;
        
        case TRUE:
          if(pad){
            for(i=0;i<3;i++){
              digits[i] = digits[i+1];
            }
            digits[3] = saved_digits[COLSX-pad];
            pad--;
          }
          else{
            blocking = FALSE;
            block = blocking;
            offset = 0;
            started_waiting = TRUE;
            display_flag = WAITING;
          }
          break;
        default:
          break;
        }
      break;
      
    case WAITING:
      if(started_waiting){
        for(i=0;i<COLSX;i++){
          saved_digits[i] = digits[i]; // Copy current display
        }
        started_waiting = FALSE;
        cursor_blink = TRUE;
      }
      if(cursor_blink == TRUE){
        cur_read = cur_pos;      
        if(prev_cur_pos != cur_read){
          digits[prev_cur_pos] &= NO_CURSOR;
        }
        if(cur_read < COLSX){
          digits[cur_read] ^= CURSOR_VALUE;
        }
        prev_cur_pos = cur_read;        
      }
      break;
    default:
      break;
  }
  
}

void * keypad(){
  int col;
  int timeout = DELAY;
  char str[6];
  
  while(alive){
    if(display_flag == CHANGED){
      timeout = 1; //Trigger an update
    }
    if(--timeout == 0){
      update_display();
      timeout = DELAY;
    }
    for(col=0;col<COLSX;col++){
      write_to_port(C, 0);        // LEDS off
      write_to_port(A, (BYTE) (01 << col));     // select column
      write_to_port(C, digits[col]);  // next LED pattern  

      write(fd_RS232,"@00P1?\r",7);             // Read the column
      usleep(SLEEP);
      read(fd_RS232,str,6);
      
      read_button(col,str[4]);
    }
  }
  return 0;
}

/*------------------------------------------------------------------------------
 * Display Char Routine
 *------------------------------------------------------------------------------
 */
BYTE display_char(char key){
  BYTE character = 0;
  switch(key){
    case ' ':
      character = 0x00;
      break;
    case '-':
      character = 0x40;
      break;
    case '=':
      character = 0x48;
      break;
    case '.':
      character = 0x80;
      break;
    case '?':
      character = 0x83;
      break;
    case '!':
      character = 0x82;
      break;
    case '_':
      character = 0x08;
      break;
    case '(':
    case '<':
    case '[':
    case '{':
      character = 0x39;
      break;
    case ')':
    case '>':
    case ']':
    case '}':
      character = 0x0F;
      break;
    default:
      if((key >= '0') && (key <= '9')){      // Numbers
        character = numtab[key-'0'];
      }
      else if((key >= 'A') && (key <= 'Z')){ // "Upper case" alphabet
        character = alphaU[key-'A'];
      }
      else if((key >= 'a') && (key <= 'z')){ // "Lower case" alphabet
        character = alphaL[key-'a'];
      }
      else{
        character = 0;
      }
      break;
  }
  return character;
}

/*------------------------------------------------------------------------------
 * console I/O
 *------------------------------------------------------------------------------
 */
struct termios  savetty;

void term_initio(void){
  struct termios tty;

  tcgetattr(0, &savetty);
  tcgetattr(0, &tty);
  tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
  tcsetattr(0, TCSADRAIN, &tty);
}

void term_exitio(void){
  tcsetattr(0, TCSADRAIN, &savetty);
}
/*------------------------------------------------------------------------------
 * serial I/O (8 bits, 1 stopbit, no parity, 38,400 baud)
 *------------------------------------------------------------------------------
 */
int rs232_open(void){
  char   *name;
  int   result;  
  struct termios tty;
  
  fd_RS232 = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY);
  assert(fd_RS232>=0);
  result = isatty(fd_RS232);
  assert(result == 1);
  name = ttyname(fd_RS232);
  assert(name != 0);
  result = tcgetattr(fd_RS232, &tty);
  assert(result == 0);

  tty.c_iflag = IGNBRK; /* ignore break condition */
  tty.c_oflag = 0;
  tty.c_lflag = 0;
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bits-per-character */
  tty.c_cflag |= CLOCAL | CREAD; /* Ignore model status + read input */
  cfsetospeed(&tty, B38400); /* set output baud rate */
  cfsetispeed(&tty, B38400); /* set input baud rate */
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 0;
  tty.c_iflag &= ~(IXON|IXOFF|IXANY);
  result = tcsetattr (fd_RS232, TCSANOW, &tty); /* non-canonical */
  tcflush(fd_RS232, TCIOFLUSH); /* flush I/O buffer */
  return 0;
}

int rs232_close(void){
  int  result;
  result = close(fd_RS232);
  assert (result==0);
  return 0;
}
/*------------------------------------------------------------------------------
 * USB-PIO specific functions
 *------------------------------------------------------------------------------
 */
void setup_ports(){
  char str[4];
  write(fd_RS232,"@00D000\r",8); // Port A input
  usleep(SLEEP);                 // Needs time for reply
  usleep(SLEEP);
  read(fd_RS232,str,4);

  write(fd_RS232,"@00D1FF\r",8); // Port B output
  usleep(SLEEP);
  read(fd_RS232,str,4);

  write(fd_RS232,"@00D200\r",8); // Port C input
  usleep(SLEEP);
  read(fd_RS232,str,4);
}

void write_to_port(int port, BYTE bits){
  char str[10];

  snprintf(str,10,"@00P%d%02x\r",port,bits);
  write(fd_RS232,str,10);
  usleep(SLEEP);
  read(fd_RS232,str,4);
}
/*------------------------------------------------------------------------------
 * exit subroutine
 *------------------------------------------------------------------------------
 */
void closing_time() {
  alive = FALSE;
  pthread_join(keypad_thread, NULL);
  write_to_port(C, 0);      // Last LED off
}

