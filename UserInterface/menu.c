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
  char delay_value[3] = {'\0'};
  int count = 0, scroll_delay_read;
  char button_read = FALSE;  // Local snapshot of Global 'Button'
  int state_read = SUBMENU_SELECT;


  while(alive && state_read == SUBMENU_SELECT){

    if(count == 0){
      scroll_delay_read = get_scroll_delay();
      if (scroll_delay_read < 10){
        delay_value[0] = scroll_delay_read + '0';
        insert_char(delay_value[0]);
      }
      else{
        delay_value[0] = '1';
        delay_value[1] = scroll_delay_read + '0';

        insert_char(delay_value[0]);
        insert_char(delay_value[1]);
      }
      display_string(delay_value,BLOCKING);
      bzero(delay_value,3);
      reset_buffers();
    }

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

    count++;
/* Button has been pressed. Now what? */
    switch(button_read){
      case '1':
        if(count == 1)
        {
          set_scroll_delay(1);
          //delay_value[0] = button_read;
          //display_string(delay_value,NOT_BLOCKING);
        }
        if (count == 2)
        {
          set_scroll_delay(11);
          //delay_value[0] = '1';
          //delay_value[1] = '1';
          //display_string("11",NOT_BLOCKING);
          count = 0;
          //reset_buffers();
        }
        insert_char(button_read);
        break;

      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if(count == 2){
          delay_value[0] = '1';
          delay_value[1] = button_read;
          set_scroll_delay((button_read - '0') + 10);
        }
        else{
          delay_value[0] = button_read;
          set_scroll_delay(button_read - '0');
        }
        //reset_buffers();
        insert_char(button_read);
        //display_string(delay_value,NOT_BLOCKING);
        count = 0;
        break;

      case '0':
        if(count == 2){
          scroll_delay_read = 10;
          set_scroll_delay(scroll_delay_read);
          insert_char('1');
          insert_char('0');
          //display_string("10",NOT_BLOCKING);
          count = 0;
        }
        break;

      case 'D':
        if(count){
          count--;
          delete_char();
        }
        break;

      case 'B': // Down
        if(--scroll_delay_read > 0){
          set_scroll_delay(scroll_delay_read);
          printf("scroll delay: %d\n",scroll_delay_read);
        }
        else{
          scroll_delay_read = 1;
          printf("scroll delay: MIN\n");
        }
        if (scroll_delay_read > 9){
          insert_char('1');
        }
        insert_char('0' + (scroll_delay_read % 10));
        count = 0;
        //display_string(delay_value,NOT_BLOCKING);
        break;
      case 'F': // Up
        if(++scroll_delay_read <= 15){
          set_scroll_delay(scroll_delay_read);
          printf("scroll delay: %d\n",scroll_delay_read);
        }
        else{
          scroll_delay_read = 15;
          printf("scroll delay: MAX\n");
        }
        if (scroll_delay_read > 9){
          insert_char('1');
        }
        insert_char('0' + (scroll_delay_read % 10));
        count = 0;
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
      default:
        break;
    }

    printf("scroll delay: %d\n",scroll_delay_read);
  }
  return;

}
