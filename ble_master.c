
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ble.h"


void master_loop (void)
{
  if ((ble_scan ()) > 0)
  {
    ble_message_t message;

    message.header.type = BLE_ANY;
    while ((ble_receive_message (&message)) >= 0)
    {
    }
  }
}

int main (int argc, char * argv[])
{
  int status = -1;
  int init_attempt = 0;

  do
  {
    init_attempt++;
    printf ("BLE init attempt %d\n", init_attempt);
    
    status = ble_init ();
  } while ((status <= 0) && (init_attempt < 2));

  if (status > 0)
  {
    master_loop ();
    ble_deinit ();
  }

  return 0;
}

