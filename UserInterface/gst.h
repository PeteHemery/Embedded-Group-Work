/******************************************************************************************
* gstClient.h *
* Author: James Sleeman *
*****************************************************************************************/

#ifndef GST_H_
#define GST_H_

//#define STANDALONE

void killGst();
void playGst();
void pauseGst();
long long int getTimeGst();
void seekGst();
void set_ip_and_port(char *ip_in, int port_in);

void * gst_multicast(void);
#ifndef STANDALONE
void * gst(void);
#endif

#endif
