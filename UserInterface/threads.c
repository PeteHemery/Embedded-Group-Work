/*
 * threads.c
 *
 *  Created on: 6 Feb 2012
 *      Author: Pete Hemery
 */


#include "main.h"

/* Thread Names */
pthread_t keypad_thread;
pthread_t state_machine_thread;
pthread_t gst_control_thread;
pthread_t timer_thread;

pthread_attr_t keypad_Attr;
pthread_attr_t state_machine_Attr;
pthread_attr_t gst_control_Attr;
pthread_attr_t timer_Attr;

pthread_mutex_t button_Mutex;
pthread_cond_t button_Signal;

/* Thread State */
pthread_mutex_t state_Mutex;
pthread_cond_t state_Signal;

pthread_mutex_t display_Mutex;
pthread_cond_t display_Signal;

pthread_mutex_t gst_control_Mutex;
pthread_cond_t gst_control_Signal;

pthread_mutex_t timer_Mutex;
pthread_cond_t timer_Signal;

int button_thread_state;

void start_threads(void){
  extern void * keypad(void);
  extern void * state_machine(void);
  extern void * gst_control(void);
  extern void * timer(void);

  int ret;

  /* Setup Mutex */
  pthread_mutex_init(&button_Mutex, NULL);
  pthread_mutex_init(&state_Mutex, NULL);
  pthread_mutex_init(&display_Mutex, NULL);
  pthread_mutex_init(&gst_control_Mutex, NULL);
  pthread_mutex_init(&timer_Mutex, NULL);

  /* Setup Conditions */
  pthread_cond_init(&button_Signal, NULL);
  pthread_cond_init(&state_Signal, NULL);
  pthread_cond_init(&display_Signal, NULL);
  pthread_cond_init(&gst_control_Signal, NULL);
  pthread_cond_init(&timer_Signal, NULL);

  /* Setup Threads */
  pthread_attr_init(&keypad_Attr);
  pthread_attr_init(&state_machine_Attr);
  pthread_attr_init(&gst_control_Attr);
  pthread_attr_init(&timer_Attr);

  ret = pthread_create( &keypad_thread, &keypad_Attr, (void *)keypad, NULL); /* TODO: Error Checking */
  ret = pthread_create( &state_machine_thread, &state_machine_Attr, (void *)state_machine, NULL); /* TODO: Error Checking */

  pthread_attr_setdetachstate(&gst_control_Attr, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setdetachstate(&timer_Attr, PTHREAD_CREATE_JOINABLE);

  if((pthread_create( &gst_control_thread, &gst_control_Attr,
                        (void *)gst_control, NULL)) != 0){

    perror("gst_control thread failed to start\n");
    exit(EXIT_FAILURE);
  }

  if((pthread_create( &timer_thread, &timer_Attr,
                                            (void *)timer, NULL)) != 0){
    perror("Timer thread failed to start\n");
    exit(EXIT_FAILURE);
  }

  pthread_attr_destroy(&gst_control_Attr);
  pthread_attr_destroy(&timer_Attr);

  return;
}

/*------------------------------------------------------------------------------
 * exit subroutine
 *------------------------------------------------------------------------------
 */
void closing_time(void){
  alive = FALSE;

  pthread_mutex_lock(&state_Mutex);
  button_thread_state = STATE_KILL;
  pthread_cond_broadcast(&state_Signal);
  pthread_mutex_unlock(&state_Mutex);

  pthread_mutex_lock(&button_Mutex);
  pthread_cond_broadcast(&button_Signal);
  pthread_mutex_unlock(&button_Mutex);

  pthread_join(keypad_thread, NULL);
  pthread_join(state_machine_thread, NULL);
  pthread_join(gst_control_thread, NULL);
  printf("Signalled threads to close\n");

  write_to_port(C, 0);      /* Last LED off */
  close_term();

  pthread_attr_destroy(&keypad_Attr);
  pthread_attr_destroy(&state_machine_Attr);

  pthread_mutex_destroy(&button_Mutex);
  pthread_mutex_destroy(&state_Mutex);
  pthread_mutex_destroy(&display_Mutex);
  pthread_mutex_destroy(&gst_control_Mutex);

  pthread_cond_destroy(&button_Signal);
  pthread_cond_destroy(&state_Signal);
  pthread_cond_destroy(&display_Signal);
  pthread_cond_destroy(&gst_control_Signal);

  printf("Closing\n");
}

