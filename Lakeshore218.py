# Lakeshore 218 python driver
# Derek Fujimoto
# April 2026

from serial.tools import list_ports
import serial

class Lakeshore218(object):

    # settings
    serial_settings = { 'baudrate':     9600,
                        'bytesize':     serial.SEVENBITS,
                        'parity':       serial.PARITY_ODD,
                        'stopbits':     serial.STOPBITS_ONE,
                        'write_timeout':10,     # seconds
                        'xonxoff':      False,
                        'rtscts':       False,
                        'dsrdtr':       False,
                       }

    def __init__(self, port=""):
        """Connect to Lakeshore 218 device via serial connection

        Args:
            port (str): name of port, ex: "/dev/ttyUSB0"
        """
        
        # connect to first available port
        if not port:
            port = str(list_ports.comports()[0])
            port = port.split()[0]
        
        self.ser = serial.Serial(port, **self.serial_settings)

    def __delete__(self):
        self.ser.close()

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
        self.ser.write(cmd.encode('ascii') + b'\n')

    def query(self, cmd:str):
        """
        See https://github.com/ucn-triumf/scmfe/blob/524c1c12ea32d356211d9681a12d902e083e0788/scmfe.c#L307-L313
        """
        self.write(cmd)
        out = self.ser.read_until(expected=b'\n')
        out = out.decode().strip()
        return out.replace('\r\n', '')