/*
 * menu.c
 *
 *  Created on: 5 Feb 2012
 *      Author: Pete Hemery
 */

#include "menu.h"

#define MENU_STR_NUM  7


/*  MENU SELECTION */
void menu_select(void){
  enum {zero, VOLUME, LOCATION, SCROLL, PLAYBACK, LOG_OUT, EXIT_PROG};
  int choice = 1;
  char button_read = 0;
  int state_read;

  set_menu(TRUE); // in display.c
  show_choice(choice);

  pthread_mutex_lock(&state_Mutex);
  state_read = state; // Initialise the copy of the current state.
  pthread_mutex_unlock(&state_Mutex);

  while(alive && (state_read == MENU_SELECT)){

    pthread_mutex_lock(&button_Mutex);
    pthread_cond_wait(&button_Signal, &button_Mutex); // Wait for press
    button_read = button;               // Read the button pressed
    pthread_mutex_unlock(&button_Mutex);

	pthread_mutex_lock(&state_Mutex);
	state_read = state;
	pthread_mutex_unlock(&state_Mutex);
    if(state_read == EMERGENCY || alive == FALSE){
      set_menu(FALSE); // in display.c
      break; // Get out if there's an emergency
    }

/* Button has been pressed. Now what? */
    switch(button_read){
      case '0' + VOLUME: // Volume
      case '0' + LOCATION: // Location
      case '0' + SCROLL: // Scroll Speed Settings
      case '0' + PLAYBACK: // Playback Settings
      case '0' + LOG_OUT: // Log Out
      case '0' + EXIT_PROG: // Log Out
        choice = button_read - '0';
        show_choice(choice);
        break;
      /*case '6':
      case '7':
      case '8':
      case '9':
      case '0':
        break;*/
      case ACCEPT_PLAY:
        break;

      case ENTER_MENU:
  	    switch(choice){
		  case VOLUME:
		    pthread_mutex_lock(&state_Mutex);
            state = SUBMENU_SELECT;
            state_read = state;
            pthread_mutex_unlock(&state_Mutex);
            printf("Volume Selected\n");
            volume();
            show_choice(choice); // After return, display correct choice again
		    break;
		  case LOCATION:
	        //wifi_scan();
		    break;
          case SCROLL:
            break;
          case PLAYBACK:
            break;
		  case LOG_OUT:
            set_menu(FALSE);
            reset_buffers();
		    display_string(" Goodbye ",BLOCKING);
		    pthread_mutex_lock(&state_Mutex);
	        logged_in = FALSE;
            state = INIT_STATE;
            state_read = state;
            pthread_mutex_unlock(&state_Mutex);
            printf("Logging Out\n");
		    break;
		  case EXIT_PROG:
		    printf("Exiting\n");
		    exit(0);
          default:
	        break;
	    }
	    printf("Choice: %d\n",choice);
        break;

      case CANCEL:
        pthread_mutex_lock(&state_Mutex);
        state = WAITING_LOGGED_IN; // Go back to waiting
        state_read = state;
        pthread_mutex_unlock(&state_Mutex);
        set_menu(FALSE);
        break;

      case FORWARD:
        if(++choice == MENU_STR_NUM){
          choice = 1;
        }
        show_choice(choice);
        break;

      case BACK:
        if(--choice == 0){
          choice = MENU_STR_NUM - 1;
        }
        show_choice(choice);
        break;

      case DELETE:
      default:
        break;
    }
  }
}

void show_choice(int choice){
  char *menu_strings[MENU_STR_NUM] = {
    "",
    "1.Volume.",
    "2.Location.",
    "3.Scroll Settings.",
    "4.Playback Settings.",
    "5.Log out.",
    "6.Exit Program"
  };

  display_string(menu_strings[choice],NOT_BLOCKING);

  return;
}
