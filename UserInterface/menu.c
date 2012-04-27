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
            pthread_mutex_lock(&state_Mutex);
            state = SUBMENU_SELECT;
            state_read = state;
            pthread_mutex_unlock(&state_Mutex);
            printf("Scroll Delay Selected\n");
            setup_scroll_delay();
            show_choice(choice); // After return, display correct choice again
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

void setup_scroll_delay(void)
{
  char delay_value[2] = {'\0'};
  int scroll_delay_read;
  char button_read = FALSE;  // Local snapshot of Global 'Button'
  int state_read = SUBMENU_SELECT;


  set_menu(FALSE);

  set_menu(TRUE);
  scroll_delay_read = get_scroll_delay();
  delay_value[0] = scroll_delay_read + '0';
  reset_buffers();
  display_string(delay_value,NOT_BLOCKING);
  insert_char(delay_value[0]);
  printf("scroll delay: %c\n",delay_value[0]);

  set_menu(FALSE);

  while(alive && state_read == SUBMENU_SELECT){

    scroll_delay_read = get_scroll_delay();
    delay_value[0] = scroll_delay_read + '0';
    printf("scroll delay: %c\n",delay_value[0]);

    reset_buffers();
    display_string(delay_value,BLOCKING);
    insert_char(delay_value[0]);

    bzero(delay_value,2);

    pthread_mutex_lock(&button_Mutex);
    pthread_cond_wait(&button_Signal, &button_Mutex); // Wait for press
    button_read = button;               // Read the button pressed
    pthread_mutex_unlock(&button_Mutex);


    pthread_mutex_lock(&state_Mutex);
    state_read = state;
    pthread_mutex_unlock(&state_Mutex);
    if(state_read == EMERGENCY || alive == FALSE){
      //set_menu(FALSE); // in display.c
      break; // Get out if there's an emergency
    }


/* Button has been pressed. Now what? */
    switch(button_read){
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        set_scroll_delay(button_read - '0');
        break;

      case 'B': // Down
        if(--scroll_delay_read <= 1){
          scroll_delay_read = 1;
          printf("scroll delay: MIN\n");
        }
        set_scroll_delay(scroll_delay_read);
        break;

      case 'F': // Up
        if(++scroll_delay_read >= 9){
          scroll_delay_read = 9;
          printf("scroll delay: MAX\n");
        }
        set_scroll_delay(scroll_delay_read);
        break;

      case 'A':
      case 'C':
      case 'E':
        pthread_mutex_lock(&state_Mutex);
        reset_buffers();
        state = MENU_SELECT; // Go back to menu
        state_read = state;
        pthread_mutex_unlock(&state_Mutex);
        set_menu(TRUE);
        break;

      case '0':
      case 'D':
        default:
        break;
    }
  }
  return;

}
