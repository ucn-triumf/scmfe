# SCM Frontend

Readout of voltage taps on SCM using a MCC DAQ USB-1208LS, connected via USB to daq02.ucn.triumf.ca. Employs CH0-3 and with a readback rate on the Hz scale. 

MCC DAQ documentatoin 

## Setup

Actions taken on daq02.ucn.triumf.ca: 

* venv setup with python 3.6.8 (doesn't work on daq02?)
    * Within venv, `pip install numpy uldaq`


TODO items:

* `apt update; apt upgrade`
* `apt install gcc g++ make libusb-1.0-0-dev`
* Install [uldaq](https://github.com/mccdaq/uldaq) - mccdaq offical universal library.
    * Documentation [here](https://digilent.com/reference/software/universal-library/linux/start)
    * Python API reference [here](https://files.digilent.com/manuals/UL-Linux/python/index.html)

Source located at `/home/ucn/online/scmfe`

