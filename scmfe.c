/********************************************************************\

  Name:         tinyfe.c
  Created by:   Stefan Ritt

  Contents:     Experiment specific readout code (user part) of
                Midas frontend. This example demonstrate the
                implementation of the sub-events packing.
 
  $Id:$

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "midas.h"
#include "msystem.h"
#include "mcstd.h"

#include "pmd.h"
#include "usb-1208LS.h"

#include "RS-232/rs232.h"

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/
/* The frontend name (client name) as seen by other MIDAS clients   */
   char *frontend_name = "scmfe";

/* The frontend file name, don't change it */
   char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
   BOOL frontend_call_loop = TRUE;

/* a frontend status page is displayed with this frequency in ms */
   INT display_period = 1000;

/* maximum event size produced by this frontend */
   INT max_event_size = 10000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
   INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
   INT event_buffer_size = 10 * 10000;



/*-- Function declarations -----------------------------------------*/

   INT frontend_init();
   INT frontend_exit();
   INT begin_of_run(INT run_number, char *error);
   INT end_of_run(INT run_number, char *error);
   INT pause_run(INT run_number, char *error);
   INT resume_run(INT run_number, char *error);
   INT frontend_loop();
   INT read_mcc(char *pevent, INT off);
   void mcc_update(INT hDB, INT hkey, void *info);
   INT read_scm_temp(char *pevent, INT off);
   void scm_temp_update(INT hDB, INT hkey, void *info);
   void scm_temp_open_port();
   INT scm_temp_query(char *cmd, char *reply, uint8_t replysize);

/*-- Equipment list ------------------------------------------------*/

#undef USE_INT

   EQUIPMENT equipment[] = {

      {"MCC",                  /* equipment name */
       { 3, 0,                    /* event ID, trigger mask */
         "SYSTEM",                /* event buffer */
         EQ_PERIODIC,               /* equipment type */
         1,                       /* event source */
         "MIDAS",                 /* format */
         TRUE,                    /* enabled */
         RO_ALWAYS,               /* read always */
         100,                     /* read every 100ms */
         0,                       /* stop run after this event limit */
         0,                      /* number of sub events */
         1,                       /* log history */
         "", "", ""
       },
       read_mcc         /* readout routine */
      },

      {"SCMtemp",
       { 4, 0,
         "SYSTEM",
         EQ_PERIODIC,
         1,
         "MIDAS",
         TRUE,
         RO_ALWAYS,
         1000,
         0,
         0,
         1,
         "", "", ""
      },
      read_scm_temp
     },

     {}
   };

#ifdef __cplusplus
}
#endif

hid_device *hid;

struct MCC_SETTINGS{
  uint8_t gains[4]; // MCC gain settings for differential measurements on four channels (0x00 = +-20V, 0x10 = +-10V, 0x20 = +-5V, 0x30 = +-4V, 0x40 = +-2.5V, 0x50 = +-2V, 0x60 = +-1.25V, 0x70 = +-1V)
} mcc_settings;

const char *mcc_settings_str = "[.]\nGains = BYTE[4] :\n[0] 0\n[1] 0\n[2] 0\n[3] 112";

struct SCM_TEMP_SETTINGS{
  char SerialPort[8]; // serial port of Lakeshore LS218 (ttyS0/ttyUSB0/...)
  uint32_t BaudRate; // baud rate, usually 9600
  char SerialMode[4]; // serial communication format (8N1 = 8 data bits, no parity bit, 1 stop bit; 7O1 = 7 data bits, odd parity bit, 1 stop bit, etc...)
} scm_temp_settings;

int SerialPortNr = 0; // setting string SCM_TEMP_SETTINGS::SerialPort is converted to an internal number

const char *scm_temp_settings_str = "[.]\nSerialPort = STRING : [8] ttyUSB0\nBaudRate = DWORD : 9600\nSerialMode = STRING : [4] 7O1";

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.
  
  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send 
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.

\********************************************************************//*-- Frontend Init -------------------------------------------------*/
INT frontend_init()
{
  HNDLE hDB, hkey;
  cm_get_experiment_database(&hDB, NULL);
  char *settings = "/Equipment/MCC/Settings";
  db_create_record(hDB, 0, settings, mcc_settings_str); // set up ODB readback for MCC settings
  db_find_key(hDB, 0, settings, &hkey);
  int ret = db_open_record(hDB, hkey, &mcc_settings, sizeof(mcc_settings), MODE_READ, mcc_update, NULL);
  if (ret != DB_SUCCESS){
     cm_msg(MERROR, "mcc init", "Cannot open MCC settings in ODB");
     return ret;
  }

  ret = hid_init(); // init USB hid for MCC
  if (ret < 0){
    cm_msg(MERROR, "mcc init", "hid_init failed with return code %d", ret);
    return FE_ERR_HW;
  }   
  hid = hid_open(MCC_VID, USB1208LS_PID, NULL); // open USB hid for MCC
  if (hid <= 0){
    cm_msg(MERROR, "mcc init", "USB-1208LS not found"); 
    return FE_ERR_HW;
  }
  uint8_t id = usbGetID_USB1208LS(hid); // query MCC ID and print
  cm_msg(MINFO, "mcc init", "Connected to USB-1208LS with ID 0x%x", id);
  usbBlink_USB1208LS(hid); // blink MCC LED

  settings = "/Equipment/SCMtemp/Settings";
  db_create_record(hDB, 0, settings, scm_temp_settings_str); // set up ODB readback for Lakeshore readout settings
  db_find_key(hDB, 0, settings, &hkey);
  ret = db_open_record(hDB, hkey, &scm_temp_settings, sizeof(scm_temp_settings), MODE_READ, scm_temp_update, NULL);
  if (ret != DB_SUCCESS){
     cm_msg(MERROR, "scmtemp init", "Cannot open SCMtemp settings in ODB");
     return ret;
  }
  scm_temp_open_port(); // open RS232 port to Lakeshore
  
  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  RS232_CloseComport(SerialPortNr); // close RS232 port to Lakeshore
  cm_msg(MINFO, "scm temp exit", "Closed connection to %s", scm_temp_settings.SerialPort);

  hid_close(hid); // close USB hid to MCC
  hid_exit();

  cm_msg(MINFO, "mcc exit", "Closed connection to USB-1208LS");
 
  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

INT begin_of_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Resuem Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{
  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\
  
  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
  return 0;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, PTYPE adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}


/*-- Read event --------------------------------------------------*/

INT read_mcc(char *pevent, INT offset)
{
  bk_init(pevent);
  float *voltages;
  bk_create(pevent, "CH", TID_FLOAT, (void **)&voltages); // create and init midas data bank
  for (int i = 0; i < 4; ++i){
    *voltages++ = volts_LS(mcc_settings.gains[i], usbAIn_USB1208LS(hid, i, mcc_settings.gains[i])); // read data and convert to voltages from all four MCC channels using gain settings
  }
  bk_close(pevent, voltages); // close midas data bank and return size
  return bk_size(pevent);
}

void mcc_update(INT hDB, INT hkey, void *info){

}

INT read_scm_temp(char *pevent, INT offset)
{
  char reply[65];
  int ret = scm_temp_query("KRDG?0\r\n", reply, 65); // get Kelvin readings from Lakeshore
  float *temps;
  bk_init(pevent);
  bk_create(pevent, "TEMP", TID_FLOAT, (void **)&temps); // init and create midas data bank
  if (ret == SUCCESS 
  && sscanf(reply, "%f,%f,%f,%f,%f,%f,%f,%f",
            temps, temps + 1, temps + 2, temps + 3, temps + 4, temps + 5, temps + 6, temps + 7) == 8 // convert reply into 8 temperatures
  )
  {
    bk_close(pevent, temps + 8); // write temperatures to midas bank
    return bk_size(pevent);
  }
  else{
    cm_msg(MERROR, "scmtemp read", "SCMtemp unable to read temperatures from reply");
    bk_close(pevent, temps);
    return bk_size(pevent);
  }
}

void scm_temp_update(INT hDB, INT hkey, void *info){
  cm_msg(MINFO, "scmtemp update", "New SCMtemp settings: %s, %u baud, mode %s",
         scm_temp_settings.SerialPort, scm_temp_settings.BaudRate, scm_temp_settings.SerialMode);
  RS232_CloseComport(SerialPortNr); // close and reopen RS232 port with new settings
  scm_temp_open_port();
}

void scm_temp_open_port(){
  SerialPortNr = RS232_GetPortnr(scm_temp_settings.SerialPort); // convert RS232 port name into port number
  if (RS232_OpenComport(SerialPortNr, scm_temp_settings.BaudRate, scm_temp_settings.SerialMode)){ // open RS232 port
    cm_msg(MERROR, "scmtemp init", "SCMtemp unable to open serial port %s", scm_temp_settings.SerialPort);
    return;
  }
  cm_msg(MINFO, "scmtemp init", "SCMtemp opened COM port %s (#%d)", scm_temp_settings.SerialPort, SerialPortNr);
  
//  char cmd[] = "QIDN?\r\n"; // query Lakeshore identification string (doesn't work, no reply?)
//  char reply[128];
//  if (scm_temp_query(cmd, reply, 128) == SUCCESS){
//     cm_msg(MINFO, "scmtemp init", "Connected to SCM temp %s", reply);
//  }
}


// send RS232 command and wait for reply with expected replysize
INT scm_temp_query(char *cmd, char *reply, uint8_t replysize){
  char buffer[1024];
  RS232_PollComport(SerialPortNr, buffer, 1024); // empty buffer first
  reply[0] = 0;
  int received = strlen(reply);
  RS232_SendBuf(SerialPortNr, cmd, strlen(cmd)); // send RS232 command
  for (int i = 0; i < 10 && received < replysize; ++i) { // loop until expected amount of bytes has been received or more than 500ms elapsed
    usleep(50000); // sleep 50ms
    int ret = RS232_PollComport(SerialPortNr, buffer, 1024); // read data from RS232 port
    if (received + ret > replysize){ // if more data received than expected
      cm_msg(MINFO, "scmtemp read", "SCMtemp command %s received more bytes (%d) than expected (%d)", cmd, received + ret, replysize);
      strncat(reply, buffer, replysize - received); // append received data, filling reply
    }
    else{
      strncat(reply, buffer, ret); // append received data
    }
    received = strlen(reply);
  }
  if (received < replysize){ // if RS232 port didn't receive expected data within 500ms
    cm_msg(MERROR, "scmtemp read", "SCMtemp command %s received only %ld of %d expected bytes within 500ms", cmd, strlen(reply), replysize);
    frontend_exit();
    if (scm_temp_settings.SerialPort == "ttyUSB0")
      strcpy(scm_temp_settings.SerialPort, "ttyUSB1");
    else if (scm_temp_settings.SerialPort == "ttyUSB1")
      strcpy(scm_temp_settings.SerialPort,"ttyUSB0");
    frontend_init();
    return FE_ERR_HW;
  }
  return SUCCESS;
}
