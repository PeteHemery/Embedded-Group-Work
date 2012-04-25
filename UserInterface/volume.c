/*
 * volume.c
 *
 *  Created on: 16 Feb 2012
 *      Author: Pete Hemery
 */

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include "menu.h"

void SetAlsaVolume(long volume)
{
  long min, max;
  snd_mixer_t *handle;
  snd_mixer_selem_id_t *sid;
  const char *card = "default";
  const char *selem_name = "Master";

  snd_mixer_open(&handle, 0);
  snd_mixer_attach(handle, card);
  snd_mixer_selem_register(handle, NULL, NULL);
  snd_mixer_load(handle);

  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
  snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

  snd_mixer_close(handle);
}

void get_volume(long *ptr){
  long min, max;
  snd_mixer_t *handle;
  snd_mixer_selem_id_t *sid;
  const char *card = "default";
  const char *selem_name = "Master";

  snd_mixer_open(&handle, 0);
  snd_mixer_attach(handle, card);
  snd_mixer_selem_register(handle, NULL, NULL);
  snd_mixer_load(handle);

  snd_mixer_selem_id_alloca(&sid);
  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

  snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
//  printf("Volume range <%lu,%lu>\n", min, max);
  snd_mixer_selem_get_playback_volume(elem,0,ptr);
//  printf("volume val = %lu\n",*ptr);
  *ptr /= (max / 100);
  snd_mixer_close(handle);
}

void volume(void){
  static long output = 50;
  int count = 0;

  char button_read = FALSE;  // Local snapshot of Global 'Button'
  int state_read = SUBMENU_SELECT;

  get_volume(&output);
  display_volume(output);
  set_menu(FALSE);

  while(alive && state_read == SUBMENU_SELECT){

    if(count == 0){
      display_volume(output);
    }

    pthread_mutex_lock(&button_Mutex);
    pthread_cond_wait(&button_Signal, &button_Mutex); // Wait for press
    button_read = button;               // Read the button pressed
    pthread_mutex_unlock(&button_Mutex);

    get_volume(&output);
    if(count == 0){
      display_volume(output);
    }

    pthread_mutex_lock(&state_Mutex);
    state_read = state;
    pthread_mutex_unlock(&state_Mutex);
    if(state_read == EMERGENCY || alive == FALSE){
      set_menu(FALSE); // in display.c
      break; // Get out if there's an emergency
    }

/* Button has been pressed. Now what? */
    switch(button_read){
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if(count == 0){
          reset_buffers();
        }
        if(++count <= 2){
          insert_char(button_read);
        }
        if(count == 2){
          output = (long)atoi(input_buffer);
          SetAlsaVolume(output);
          reset_buffers();
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
        if(--output >= 0){
          SetAlsaVolume(output);
          printf("output: %lu\n",output);
        }
        else{
          output = 0;
          printf("output: MIN\n");
        }
        break;
      case 'F': // Up
        if(++output < 100){
          SetAlsaVolume(output);
          printf("output: %lu\n",output);
        }
        else{
          output = 99;
          printf("output: MAX\n");
        }
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
  }
  return;
}