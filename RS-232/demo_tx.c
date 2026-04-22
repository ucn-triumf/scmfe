
/**************************************************

file: demo_tx.c
purpose: simple demo that transmits characters to
the serial port and print them on the screen,
exit the program by pressing Ctrl-C

compile with the command: gcc demo_tx.c rs232.c -Wall -Wextra -o2 -o test_tx

**************************************************/

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"



int main()
{
  int i=0,
      cport_nr=0,        /* /dev/ttyS0 (COM1 on windows) */
      bdrate=9600;       /* 9600 baud */

  char mode[]={'7','O','1',0},
       str[2][512];

  cport_nr = RS232_GetPortnr("ttyUSB0");
  printf("Port #%d\n", cport_nr);

  strcpy(str[0], "QIDN?\r\n");

  strcpy(str[1], "KRDG?0\r\n");

  if(RS232_OpenComport(cport_nr, bdrate, mode))
  {
    printf("Can not open comport\n");

    return(0);
  }

  while(1)
  {
    RS232_SendBuf(cport_nr, str[i], strlen(str[i]));

    printf("sent: %s", str[i]);

#ifdef _WIN32
    Sleep(1000);
#else
    usleep(1000000);  /* sleep for 1 Second */
#endif

    unsigned char buf[128];
    int read = RS232_PollComport(cport_nr, buf, 128);
    buf[read] = 0;
    printf("Answered %d bytes: %s\n", read, buf);

    i++;

    i %= 2;
  }

  return(0);
}

