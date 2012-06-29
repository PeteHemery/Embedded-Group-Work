/******************************************************************************************
* gstClient.c *
* Description: This is file is used to setup gstreamer to receive an audio stream and *
* playback the stream and offer the controls to change the state of the *
* pipleine used to stream the audio. *
* External globals: None *
* Author: James Sleeman *
* This is based on the Gstreamer hello world application which cam be found here: *
* http://gstreamer.freedesktop.org/data/doc/gstreamer/head/manual/html/chapter-helloworld.html
*****************************************************************************************/

#include <string.h>
#include <pthread.h>

#include <gst/gst.h>
#include <glib.h>

#include "gst.h"

extern unsigned char alive;

GstElement *pipeline; // Moved here to allow other gst functions to use the variable.
GMainLoop *loop;

char file_loc[] = "/media/Data/Ab/Work/ESD/Audio/";
int floclen;
char * fn;
int file_set = FALSE;

int gst_playing = FALSE;
int gst_paused = FALSE;


void * gst_control(void){

  int status = 0;
  void *res;

  floclen = strlen(file_loc);

  while(alive)
  {
    pthread_t gst_thread;
    pthread_attr_t gst_Attr;

    pthread_attr_init(&gst_Attr);
    pthread_attr_setdetachstate(&gst_Attr, PTHREAD_CREATE_JOINABLE);

    if(pthread_create( &gst_thread, &gst_Attr, (void *)gst, NULL) != 0)
    {
      perror("gst thread failed to start\n");
      exit(EXIT_FAILURE);
    }

    /* Destroy the thread attributes object, since it is no longer needed */

    status = pthread_attr_destroy(&gst_Attr);
    if (status != 0)
    {
      errno = status;
      perror("pthread_attr_destroy");
    }

    status = pthread_join(gst_thread, &res);

    if (status != 0)
    {
      errno = status;
      perror("pthread_join");
    }
    if (res == (void *)-1)
    {
      printf("gst thread exited abnormally!\n");
    }
    else
    {
      printf("Joined with thread; returned value was %s\n", (char *) res);
      free(res); /* Free memory allocated by thread */
    }

  }
  pthread_exit(0);
}


static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg))
    {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit(loop);
      break;

    case GST_MESSAGE_ERROR:
      {
          gchar *debug;
          GError *error;

          gst_message_parse_error(msg, &error, &debug);
          g_free(debug);

          g_printerr("Error: %s\n", error->message);
          g_error_free(error);

          g_main_loop_quit(loop);
          break;
      }

    default:
      break;
    }

  return TRUE;
}

void killGst()
{
  if (gst_playing == TRUE)
  {
    g_main_loop_quit(loop);
  }
  gst_playing = FALSE;
}


void set_filename(char *filename)
{
  if (gst_playing == TRUE)
  {
    killGst();
  }
  fn = malloc(sizeof(char) * (strlen(filename) + floclen));
  strcpy(fn,file_loc);
  strcat(fn,filename);
  printf("filename: %s\n",fn);
  file_set = TRUE;
  gst_paused = FALSE;
}


static void on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (decoder, "sink");

  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);
}

void * gst(void)
{
  GstElement *src,  *ffdemux_mp3, *ffdec_mp3, *sink;
  GstBus *bus;

  printf("gst thread alive!\n");

  while(file_set == FALSE && alive == TRUE); //Wait for filename
  if (alive != TRUE)
    pthread_exit((void *)0);
  file_set = FALSE;

  /* Initialisation */
  gst_init (NULL,NULL);

  loop = g_main_loop_new (NULL, FALSE);


  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("client");
  src = gst_element_factory_make ("filesrc", "src");

  /*  MP3 */
  ffdemux_mp3  = gst_element_factory_make ("ffdemux_mp3", "ffdemux_mp3");
  ffdec_mp3  = gst_element_factory_make ("ffdec_mp3", "ffdec_mp3");
  sink = gst_element_factory_make ("alsasink", "sink");

  if (!pipeline || !src || !ffdemux_mp3 || !ffdec_mp3 || !sink)
  {
    g_printerr ("One element could not be created. Exiting.\n");
    pthread_exit((void *)-1);
  }

  /* we set the input filename to the source element */
  g_object_set (G_OBJECT (src), "location", fn, NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), src, ffdemux_mp3, ffdec_mp3, sink, NULL);

  /* we link the elements together */
  gst_element_link (src, ffdemux_mp3);
  gst_element_link (ffdec_mp3, sink);
  g_signal_connect (ffdemux_mp3, "pad-added", G_CALLBACK (on_pad_added), ffdec_mp3);

  /* Set the pipeline to "playing" state*/
  gst_element_set_state(pipeline, GST_STATE_PLAYING);


  /* wait until it's up and running or failed */
  if (gst_element_get_state (pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE)
  {
    g_error ("Failed to go into PLAYING state");
    pthread_exit((void *)-1);
  }

  gst_playing = TRUE;
  free(fn);
  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  gst_playing = FALSE;
  gst_paused = FALSE;

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state(pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref(GST_OBJECT (pipeline));

  pthread_exit((void *)0);
}

int gst_play_pause()
{
  if (gst_playing)
  {
    if (gst_paused == FALSE)
    {
      gst_element_set_state(pipeline, GST_STATE_PAUSED);
      gst_paused = TRUE;
    }
    else
    {
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
      gst_paused = FALSE;
    }
    printf("gst_paused = %d\n",gst_paused);
    return gst_paused;
  }
  return -1;
}

void playGst()
{
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void pauseGst()
{
  gst_element_set_state(pipeline, GST_STATE_PAUSED);
}

long long int getTimeGst()
{
  char trackTime[12] = {0};

  GstFormat format = GST_FORMAT_TIME; //Time in nanoseconds
  gint64 curPos; //Stores the current position
  if(gst_playing)
    {
      if(gst_element_query_position(pipeline, &format, &curPos))
      {

        /* The maximum time supported is by this print statement is 9 hours 59 minutes
and 59 seconds */
        snprintf(trackTime, 11, "%u:%02u:%.2u.%2.2u\n", GST_TIME_ARGS (curPos));
        printf("trackTime %s\n",trackTime);
      }
      return curPos;
    }

  return 0;
}
