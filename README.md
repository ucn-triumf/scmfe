# SCM Frontend

Readout of voltage taps on SCM using a MCC DAQ USB-1208LS, connected via USB to daq02.ucn.triumf.ca. Employs CH0-3 and with a readback rate on the Hz scale. 

MCC DAQ documentatoin 

## Setup

Actions taken on daq02.ucn.triumf.ca: 

```bash
# clone this repo
cd /online
git clone git@github.com:ucn-triumf/scmfe.git
cd scmfe

# setup uldaq (as root)
yum install libusbx libusbx-devel hidapi hidapi-devel Cython python3-devel libudev-devel 
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
pip install uldaq Cython libusb1

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

## Useful Links

Note that the uldaq does not support the MCC USB-1208LS device. Otherwise these would have been useful:

* [mccdaq offical universal library](https://github.com/mccdaq/uldaq)
* [Documentation](https://digilent.com/reference/software/universal-library/linux/start)
* [Python API reference](https://files.digilent.com/manuals/UL-Linux/python/index.html)

Instead we use this third-party driver: 

* [Linux_Drivers](https://github.com/wjasper/Linux_Drivers/tree/master) (contains the top-level python code)
    * Driver location: `Linux_Drivers/USB/python/usb_1208LS.py`
* [cython-hidapi](https://github.com/trezor/cython-hidapi) (cython wrappers for the hidapi code which is a dependency for the Linux_Drivers)

## Device

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