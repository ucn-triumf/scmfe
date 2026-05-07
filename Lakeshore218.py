# Lakeshore 218 python driver
# Derek Fujimoto
# April 2026

from serial.tools import list_ports
import serial
import datetime

DEBUG = False


class Lakeshore218(object):

    # settings
    serial_settings = { 'baudrate':     9600,
                        'bytesize':     serial.SEVENBITS,
                        'parity':       serial.PARITY_ODD,
                        'stopbits':     serial.STOPBITS_ONE,
                        'timeout':      10,     # seconds, read
                        'write_timeout':10,     # seconds, write
                       }

    def __init__(self, port=""):
        """Connect to Lakeshore 218 device via serial connection

        Args:
            port (str): name of port, ex: "/dev/ttyUSB0"
        """
        
        # connect to first available port if none given
        if not port:
            port = str(list_ports.comports()[0])
            port = port.split()[0]
        
        self.port = port
        self.connect()
        
    def __delete__(self):
        self.close()

    def connect(self):
        self.ser = serial.Serial(self.port, **self.serial_settings)
        if DEBUG: print(f'Connection to port {self.port} success')

    def close(self):
        self.ser.close()
        if DEBUG: print(f'Connection to serial port closed')

    def get_temp_C(self, ch=0):
        """Get temperatures in Celcius
        
        Args:
            ch (int): if ch == 0, return for all channels, else return for specific channel
        Returns:
            float|tuple: temperature in Celcius
        """
        
        if ch == 0:
            val = self.query('CRDG?')
            temps = tuple(map(float, val.split(',')))
        else:
            val = self.query(f'CRDG? {ch}')
            temps = float(val)

        return temps

    def get_temp_K(self, ch=0):
        """Get temperatures in Kelvin
        
        Args:
            ch (int): if ch == 0, return for all channels, else return for specific channel
        Returns:
            float|tuple: temperature in Kelvin
        """
        
        if ch == 0:
            val = self.query('KRDG?')
            temps = tuple(map(float, val.split(',')))
        else:
            val = self.query(f'KRDG? {ch}')
            temps = float(val)

        return temps

    def write(self, cmd:str):
        self.ser.reset_output_buffer()
        self.ser.write(cmd.encode('ascii') + b'\n')

    def query(self, cmd:str):
        """
        See https://github.com/ucn-triumf/scmfe/blob/524c1c12ea32d356211d9681a12d902e083e0788/scmfe.c#L307-L313

        Args:
            cmd (str): command to send
        
        Returns:
            str: response from device
        """

        if DEBUG: print(f'{datetime.datetime.now()} Query {cmd}', flush=True)
        self.ser.reset_input_buffer()
        self.write(cmd)
        
        if DEBUG: print(f'{datetime.datetime.now()}\tWrite completed.', flush=True)
        out = self.ser.read_until(expected=b'\n')
        out = out.decode().strip()
        out = out.replace('\r\n', '')
        
        if DEBUG: print(f"{datetime.datetime.now()}\tResponse: {out}.", flush=True)

        return out