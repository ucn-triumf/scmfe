# SCM Frontend

## SCM Voltage

Readout of voltage taps on SCM using a MCC DAQ USB-1208LS, connected via USB to daq02.ucn.triumf.ca. Employs CH0-3 and with a readback rate on the Hz scale. 

MCC DAQ documentation 

### Setup

Actions taken on daq02.ucn.triumf.ca: 

```bash
# clone this repo
cd /online
git clone git@github.com:ucn-triumf/scmfe.git
cd scmfe

# setup uldaq (as root)
yum install libusbx libusbx-devel hidapi hidapi-devel Cython python3-devel libudev-devel mariadb mariadb-devel 
wget -N https://github.com/mccdaq/uldaq/releases/download/v1.2.1/libuldaq-1.2.1.tar.bz2
tar -xvf libuldaq-1.2.1.tar.bz2
cd libuldaq-1.2.1
./configure && make
make install
cd ..

# setup python
mkdir vnenv
python3 -m venv venv
source venv/bin/activate
pip install Cython libusb1 numpy # or do pip install -r requirements.txt

# setup hidapi python
git clone --recursive https://github.com/trezor/cython-hidapi.git
cd cython-hidapi/
# if the following line fails, need add the following line to setup.py at line 147 and try again:
# extra_compile_args=["-std=c99"],
# see https://github.com/trezor/cython-hidapi/pull/202
pip install .

# setup needed drivers for MCC USB-1208LS
git clone git@github.com:wjasper/Linux_Drivers.git
cd Linux_Drivers/USB/mcc-libusb
make
```

### Useful Links

Note that the uldaq does not support the MCC USB-1208LS device. Otherwise these would have been useful:

* [mccdaq offical universal library](https://github.com/mccdaq/uldaq)
* [Documentation](https://digilent.com/reference/software/universal-library/linux/start)
* [Python API reference](https://files.digilent.com/manuals/UL-Linux/python/index.html)

Instead we use this third-party driver: 

* [Linux_Drivers](https://github.com/wjasper/Linux_Drivers/tree/master) (contains the top-level python code)
    * Driver location: `Linux_Drivers/USB/python/usb_1208LS.py`
* [cython-hidapi](https://github.com/trezor/cython-hidapi) (cython wrappers for the hidapi code which is a dependency for the Linux_Drivers)

### Device

Run `usb-devices`. Output: 

```
T:  Bus=03 Lev=01 Prnt=01 Port=01 Cnt=01 Dev#=  2 Spd=1.5 MxCh= 0
D:  Ver= 1.10 Cls=00(>ifc ) Sub=00 Prot=00 MxPS= 8 #Cfgs=  1
P:  Vendor=09db ProdID=007a Rev=01.02
S:  Manufacturer=MCC
S:  Product=USB-1208LS
S:  SerialNumber=0205DD48
C:  #Ifs= 1 Cfg#= 1 Atr=80 MxPwr=100mA
I:  If#= 0 Alt= 0 #EPs= 2 Cls=03(HID  ) Sub=00 Prot=00 Driver=(none)
```

## SCM Temperature 

Looks like there is no Lakeshore driver for this either. WS had used serial communication, we will do the same. 

### Setup

Assuming setup from [SCM Voltage readback](#scm-voltage) was completed:

```bash
cd scmfe
source venv/bin/activate
pip install pyserial

# check which groups can access ports ttyS*
cd /dev
ls -l | grep ttyS # probably dialout

# check that user is a part of that group, if not add
groups ucn 

# add (as root)
sudo usermod -aG dialout ucn

# check for serial port connectivity
setserial -g /dev/ttyS[0123]
```

### Debugging Notes

```bash
[root@daq02 dev]# dmesg | tail
[ 2148.215263] usb 3-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[ 2148.215270] usb 3-1: Product: USB-Serial Controller 
[ 2148.215277] usb 3-1: Manufacturer: Prolific Technology Inc. 
[ 2148.215282] usb 3-1: SerialNumber: BTDJb10CD20
```


* The Lakeshore might have to be in remote running mode
* 

### Useful links

* [Pyserial documentation](https://pyserial.readthedocs.io/en/latest/pyserial.html)
* [Lakeshore 218 manual](https://www.lakeshore.com/docs/default-source/product-downloads/manuals/218_manual.pdf?sfvrsn=6a03068_3) (section 6.2 is the serial interface, 6.3 is the serial commands)