/*
 * main.c
 *
 *  Created on: 5 Feb 2012
 *      Author: Pete Hemery
 */

#include "main.h"

BYTE alive = TRUE;  // Exit condition of while loops

/*------------------------------------------------------------------------------
 * Main
 *------------------------------------------------------------------------------
 */
int main (void) {
  int ret;
  int prev_state = state;
//  sighandler_t

  /* error handling - reset LEDs */
  atexit(closing_time);
  signal(SIGINT, (void *)closing_time);

  printf("Press 'e' and 'Enter' to toggle Emergency state\n");
  printf("Press 'x' and 'Enter' to exit\n");

  setup_term();
  start_threads();

  while((ret = getchar()) != 'x' && alive){
	if (ret == 'e'){
	  pthread_mutex_lock(&state_Mutex);
	  if (state == EMERGENCY){
        state = prev_state;
        if(state == SUBMENU_SELECT){
          state = MENU_SELECT;
        }
      }
      else {
        prev_state = state;
        state = EMERGENCY;
      }

      pthread_mutex_lock(&button_Mutex);  // Unlock state machine
      pthread_cond_broadcast(&button_Signal);
      pthread_mutex_unlock(&button_Mutex);

	  pthread_cond_broadcast(&state_Signal);
	  pthread_mutex_unlock(&state_Mutex);
    }
  }
  alive = FALSE; // fallen out by pressing 'x'
  return 0;
}