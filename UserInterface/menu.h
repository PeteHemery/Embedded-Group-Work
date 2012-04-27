/*
 * menu.h
 *
 *  Created on: 5 Feb 2012
 *      Author: student
 */

#ifndef MENU_H_
#define MENU_H_

#include "states.h"
#include "threads.h"

void menu_select(void);
void show_choice(int);

void setup_scroll_delay(void);

extern void location_info(void);
extern void volume(void);

extern int set_scroll_delay(int);
extern int get_scroll_delay(void);

#endif /* MENU_H_ */
