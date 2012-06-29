/*
 * states.c
 *
 *  Created on: 5 Feb 2012
 *      Author: Pete Hemery
 */

#include "states.h"

int state = INIT_STATE; // State machine variable
//int state = WAITING_LOGGED_IN; // State machine variable
int logged_in = FALSE;  		// Client connected to server

extern int gst_play_pause();
extern void set_filename(char *filename);

void * state_machine(void){
  char *emergency="! EMERGENCY !";
  char button_read = FALSE;  // Local snapshot of Global 'Button'
  int state_read;

  while(alive){
	pthread_mutex_lock(&state_Mutex);
	state_read = state;
	pthread_mutex_unlock(&state_Mutex);

    if(state_read == EMERGENCY){ // Display Blocking Message
      reset_buffers();
      display_string(emergency,BLOCKING);
      continue;
    }

    if(state_read != INIT_STATE && state_read != MENU_SELECT){
      pthread_mutex_lock(&button_Mutex);	// Wait for a button press
      pthread_cond_wait(&button_Signal, &button_Mutex);
      button_read = button;
      pthread_mutex_unlock(&button_Mutex);
      if(!alive) continue;                     // Check for kill signal

      pthread_mutex_lock(&state_Mutex); 	// Check for Emergency since we've been waiting
      state_read = state;
      pthread_mutex_unlock(&state_Mutex);

      if(state_read == EMERGENCY){ // Display Blocking Message
        reset_buffers();
        set_filename("Emergency/English/emergency.mp3");
        continue;
      }
    }

	switch(state_read){
      case EMERGENCY:  // Emergency since we've been waiting
    	break;         // Drop out and catch it next round

      case INIT_STATE:
        reset_buffers();
   	    pthread_mutex_lock(&state_Mutex);
    	state = WAITING_LOGGED_OUT;
    	pthread_mutex_unlock(&state_Mutex);

      case WAITING_LOGGED_OUT:
        if(button_read >= '0' && button_read <= '9'){
          pthread_mutex_lock(&state_Mutex);
          state = INPUTTING_PIN; // Fall through to next state
      	  pthread_mutex_unlock(&state_Mutex);
        }
        else{
          if(button_read == 'C' && input_len == 0){
            alive = FALSE; // NEEDS WORK
          }
          display_string("Please Enter PIN.",NOT_BLOCKING);
          break;
        }

      case INPUTTING_PIN:
        if(button_read){
          input_pin(button_read);  // Sends a snapshot of button pressed
        }
        break;

      case WAITING_LOGGED_IN:
        switch(button_read){

        case ACCEPT_PLAY:
/*
          if (!input_len){
            printf("waiting loggedin\n");
            if (gst_play_pause()){
              display_string("Paused",NOT_BLOCKING);
            }
            else{
              display_string("Playing",NOT_BLOCKING);
            }
          }
*/
          break;

        case CANCEL:
          break;

          case ENTER_MENU:
            pthread_mutex_lock(&state_Mutex);
            state = MENU_SELECT;
            pthread_mutex_unlock(&state_Mutex);
            break;

          default:
            if(button_read >= '0' && button_read <= '9'){
              pthread_mutex_lock(&state_Mutex);
              state = INPUTTING_TRACK_NUMBER;
              pthread_mutex_unlock(&state_Mutex);
            }
            else{
              display_string("Enter Track Number.",NOT_BLOCKING);
              break;
            }
          }
          //break;

      case INPUTTING_TRACK_NUMBER:
        if(button_read){
          input_track_number(button_read);  // Sends a snapshot of button
        }
        break;

      case MENU_SELECT:
        display_string("MENU.",BLOCKING);
        menu_select();
        break;

      case SUBMENU_SELECT:

      default:
        break;
    }
  }
  return 0;
}
