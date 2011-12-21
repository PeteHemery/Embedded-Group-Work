/*****
 terminal code from:  http://www.st.ewi.tudelft.nl/~gemund/Courses/In4073/Resources/myterm.c
 used to send ASCII to USB PIO keypad
 20/12/2011 - Pete Hemery
*****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>	//file descriptors
#include <ctype.h>
#include <termios.h>	//terminal
#include <assert.h>
#include <time.h>
#include <sys/types.h> 	//threads
#include <pthread.h>

#define FALSE 0
#define TRUE 1

#define COLSX 4
#define ROWSX 4

#define SERIAL_DEVICE 	"/dev/ttyACM0"
#define A 0
#define B 1
#define C 2
#define SLEEP 1300
#define DELAY 50000000
#define READ_TIMEOUT 10
#define DEBOUNCE 4


typedef unsigned char BYTE;
typedef unsigned int DWORD;

/******************
  7-Seg hex map
    --1--
   20   2
     40  
   10   4
    --8-- 80
*******************/
BYTE digits[COLSX] ={0x73,0x79,0x78,0x79};
const BYTE numtab[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
const BYTE keytab[] = {1,4,7,10, 2,5,8,0, 3,6,9,11, 15,14,13,12};
/*                        A,   b,   C,   d,   E,   F,   g,   H,   I,   J,   K*/
const BYTE alphaU[] = {0x77,0x7C,0x39,0x5E,0x79,0x71,0x6F,0x76,0x30,0x1E,0x76,
/*    L,   M,   n,   O,   P,   Q,   r,   S,   t,   U,   V,   W,   X,   Y,   Z*/
   0x38,0x15,0x54,0x3F,0x73,0x67,0x50,0x6D,0x78,0x3E,0x1C,0x2A,0x76,0x6E,0x5B};
/*                        A,   b,   c,   d,   E,   F,   g,   h   i,   J,   K*/
const BYTE alphaL[] = {0x77,0x7C,0x58,0x5E,0x79,0x71,0x6F,0x74,0x04,0x1E,0x76,
/*    l,   M,   n,   o,   P,   Q,   r,   S,   t,   U,   V,   W,   X,   Y,   Z*/
   0x18,0x15,0x54,0x5C,0x73,0x67,0x50,0x6D,0x78,0x3E,0x1C,0x2A,0x76,0x6E,0x5B};

BYTE key[DEBOUNCE];
BYTE keytrue = 0;
pthread_t keypad_thread;
BYTE alive = TRUE;

/*----------------------------------------------------------------
 * error handling - exit subroutine
 *----------------------------------------------------------------
 */
void closing_time() {
  alive = 0;
  pthread_join(keypad_thread, NULL);
}


/*------------------------------------------------------------
 * console I/O
 *------------------------------------------------------------
 */
struct termios 	savetty;

void term_initio(){
  struct termios tty;

  tcgetattr(0, &savetty);
  tcgetattr(0, &tty);
  tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
  tcsetattr(0, TCSADRAIN, &tty);
}

void term_exitio(){
  tcsetattr(0, TCSADRAIN, &savetty);
}

/*------------------------------------------------------------
 * serial I/O (8 bits, 1 stopbit, no parity, 38,400 baud)
 *------------------------------------------------------------
 */
int fd_RS232;

int rs232_open(void){
  char 		*name;
  int 		result;  
  struct termios	tty;
  
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
}

int rs232_close(void){
  int 	result;
  result = close(fd_RS232);
  assert (result==0);
}
/*----------------------------------------------------------------
 * USB-PIO specific functions
 *----------------------------------------------------------------
 */
void setup_ports(){
  char out[4];
  write(fd_RS232,"@00D000\r",8);
  read(fd_RS232,out,4);
  usleep(SLEEP);
  write(fd_RS232,"@00D1FF\r",8);
  read(fd_RS232,out,4);
  usleep(SLEEP);
  write(fd_RS232,"@00D200\r",8);
  read(fd_RS232,out,4);
  usleep(SLEEP);
}

void write_to_port(int port, int bits){
  char out[4];
  char str[10];

  snprintf(str,8,"@00P%d%02x\r",port,bits);
  write(fd_RS232,str,8);
  read(fd_RS232,out,4);
  usleep(SLEEP);
}

void read_buttons() {
  char str[10], in[12];
  int out, button;


/*printf("%04x\n",out);
  out |= ((0x0F & (str[i+1])) << (4 * i));      // 0-9
printf("%04x\n",out);
*/
}
/****************
 keypad thread - this function continiously outputs to LEDs and occasionally reads for button presses
****************/
void * keypad(){
  int i;
  int colsel;
  int read_timeout = READ_TIMEOUT;
  char out[12];

  while(alive){
    for(i=0;i<4;i++){
  /* Write the the LED's */  
      write_to_port(A ,(BYTE) (01 << i));    /* next col/digit sel */
      write_to_port(C ,digits[i]);		/* next LEDs pattern on */
      write_to_port(C ,0);     			/* LEDS off */
//      if (read_timeout-- == 0) {
//      read_buttons();
//      read_timeout = READ_TIMEOUT;
      write(fd_RS232,"@00P1?\r",7);
      usleep(3000);
      read(fd_RS232,out,8);
      usleep(3000);
      printf("%s\n",out);

    }
  }
}

void delay(){
  int i;
  for(i=0;i<DELAY;i++);
}

void shift_digits(){
  delay();

  digits[0] = digits[1];
  digits[1] = digits[2];
  digits[2] = digits[3];
}

void display_char(char key){
  if((key >= 0) && (key < 10)){
    shift_digits();
    digits[3] = numtab[key];
  }
  else if((key > 0x40) && (key < 0x5B)){/* Upper case alphabet */
    shift_digits();
    digits[3] = alphaU[key-0x41];
  }
  else if((key > 0x60) && (key < 0x7B)){
    shift_digits();
    digits[3] = alphaL[key-0x61];
  }
  else if(key == 0x20){/*space*/
    shift_digits();
    digits[3] = 0x00;
  }
  else if(key == 0x2E){/* full stop . */
    shift_digits();
    digits[3] = 0x80;
  }
  else if(key == 0x3F){/* ? */
    shift_digits();
    digits[3] = 0xD3;
  }
  else if((key == 0x28)||(key == 0x5B)||(key == 0x7B)){/* [ */
    shift_digits();
    digits[3] = 0x39;
  }
  else if((key == 0x29)||(key == 0x5D)||(key == 0x7D)){ /* ] */
    shift_digits();
    digits[3] = 0x0F;
  }
  else if(key == 0x2D){/* - */
    shift_digits();
    digits[3] = 0x40;
  }
  else if(key == 0x5F){/* _ */
    shift_digits();
    digits[3] = 0x08;
  }
  else if(key == 0x3D){/* = */
    shift_digits();
    digits[3] = 0x48;
  }
}

void display_string(char *in){
  while(*in!='\0'){
    display_char(*in);
    in++;
  }
}

/*----------------------------------------------------------------
 * main -- open terminal and write to it.
 *----------------------------------------------------------------
 */

int main () {
  char key;
  int i, l;

  BYTE know;
  DWORD keyflags;
  BYTE colsel=-1;
  char *menu="menu";
  char *info="info";
  char *track="track";
  char *time="time";
  char *welcome="Hello World.\0";
  char* ptr;
  int ret;

  ptr = welcome;
  l = 0;

  /* error handling - reset led's and close file descriptor & terminal io */
  atexit(closing_time);
  signal(SIGINT, closing_time);

  term_initio();
  rs232_open();
  
  setup_ports();
  ret = pthread_create( &keypad_thread, NULL, keypad, NULL);

  /*
 for(i=0;i<4;i++){
   digits[i] = menu[i]-0x61;
   }*/

 /*
 digits[0]=alphaU[l++];
 digits[1]=alphaU[l++];
 digits[2]=alphaU[l++];
 digits[3]=alphaU[l++];
 * /
while(1){
 display_string(welcome);
 delay();
}
/*
 display_string(track);
 display_char('.');

 display_string(menu);
 display_char('.');

 display_string(info);
 display_char('.');

 display_string(time);
 display_char('.');

  while(1) {

    digits[0] = digits[1];
    digits[1] = digits[2];
    digits[2] = digits[3];
    digits[3] = alphaU[l++];
      
    if (l == 26){
      display_char(' ');
      for(i=0;i<DELAY;i++);
      l=0;
      display_string(welcome);
      for(i=0;i<DELAY;i++);
    }
  }
*/
  pthread_join(keypad_thread, NULL);
  term_exitio();
  rs232_close();

  return 0;
}
