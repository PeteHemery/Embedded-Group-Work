/*
 * external.c
 *
 *  Created on: 5 Feb 2012
 *      Author: Pete Hemery
 */
#include "top.h"
#include <string.h>

extern void set_filename(char *);

BYTE check_pin(char * buffer, int buf_len){
//  if (buf_len == 4){
  if (strcmp(buffer,"1234") == 0){
    printf("win\n");
    set_filename("Welcome/English/welcome.mp3");
	return TRUE;
  }
  else{
    printf("fail\n");
	return FALSE;
  }
}

BYTE play_track(char * buffer,int buf_len){
int track_num;

  track_num = atoi(buffer);

  switch(track_num){
  case 1:
    set_filename("Exhibit/English/Expert/01.mp3");
    break;
  case 2:
    set_filename("Exhibit/English/Beginner/02.mp3");
    break;
  case 3:
    set_filename("Exhibit/Spanish/Expert/03.mp3");
    break;
  case 4:
    set_filename("Exhibit/French/Further/04.mp3");
    break;
  case 5:
    set_filename("Exhibit/Welsh/Primary/05.mp3");
    break;
  case 6:
    set_filename("Exhibit/Spanish/Higher/06.mp3");
    break;
  case 311:
    set_filename("LocationInfo/3p11/English/welcome.mp3");
    break;
  case 328:
    set_filename("LocationInfo/3p28/English/welcome.mp3");
    break;
  case 31101:
    set_filename("Exhibit/Classical/31101.mp3");
    break;
  case 31102:
    set_filename("Exhibit/Classical/31102.mp3");
    break;
  case 31103:
    set_filename("Exhibit/Classical/31103.mp3");
    break;

  case 32801:
    set_filename("Exhibit/Classical/32801.mp3");
    break;
  case 32802:
    set_filename("Exhibit/Classical/32802.mp3");
    break;
  case 32803:
    set_filename("Exhibit/Classical/32803.mp3");
    break;
  default:
    set_filename("Control/English/not_found.mp3");
    return FALSE;
  }

  return TRUE;
}






/*
  case 420:
    set_filename("dumbfoundead - green.mp3");
    break;
 */
