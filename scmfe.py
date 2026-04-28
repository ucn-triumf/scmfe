"""
    SCM Voltage tabs frontend equipment
    Readout to MIDAS

    Derek Fujimoto
    April 2026
"""

import sys
import os
import midas
import midas.frontend
import collections
import midas.event
import numpy as np
from Lakeshore218 import Lakeshore218

path = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(path, 'Linux_Drivers', 'USB', 'python')
sys.path.append(path)

from usb_1208LS import usb_1208LS

class SCMVoltages(midas.frontend.EquipmentBase):
    """Readout of SCM voltages"""

    # default settings
    DEFAULT_SETTINGS = collections.OrderedDict([
        ("Gain range", ['Diff_20V','Diff_20V','Diff_20V','Diff_1V']), 
        ("note0", "SCM gains must be one of Single_10V, Diff_20V, Diff_10V, Diff_5V, Diff_4V, Diff_2.5V, Diff_2V, Diff_1.25V, Diff_1V."),
    ])

    # gain definitions
    GAINS = {'Single_10V':  usb_1208LS.SE_10_00V,   # single ended +/- 10 V
             'Diff_20V':    usb_1208LS.BP_20_00V,   # differential +/- 20 V
             'Diff_10V':    usb_1208LS.BP_10_00V,   # differential +/- 10 V
             'Diff_5V':     usb_1208LS.BP_5_00V,    # differential +/- 5 V
             'Diff_4V':     usb_1208LS.BP_4_00V,    # etc
             'Diff_2.5V':   usb_1208LS.BP_2_50V,
             'Diff_2V':     usb_1208LS.BP_2_00V,
             'Diff_1.25V':  usb_1208LS.BP_1_25V,
             'Diff_1V':     usb_1208LS.BP_1_00V,
            }

    def __init__(self, client):
        # The name of our equipment. This name will be used on the midas status
        # page, and our info will appear in /Equipment/SCMVoltages in
        # the ODB.
        # the ODB.
        equip_name = "SCMVoltages"
        
        # Define the "common" settings of a frontend.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 1
        default_common.period_ms = 1000
        default_common.read_when = midas.RO_ALWAYS
        default_common.log_history = 5
        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, 
                                              equip_name, 
                                              default_common, 
                                              self.DEFAULT_SETTINGS)
        
        self.set_status("Initializing", status_color='yellowLight')

        # connect to MCC DAQ
        self.daq = usb_1208LS()

        m = self.daq.h.get_manufacturer_string()    # manufacturer
        p = self.daq.h.get_product_string()         # product
        s = self.daq.h.get_serial_number_string()   # serial no
        self.client.msg(f"Connected to {m} {p}, serial {s}")

        # check gains
        for gain in self.settings['Gain range']:
            if gain not in self.GAINS.keys():
                self.client.msg(f'SCM gains must be one of {", ".join(self.GAINS.keys())}. Found "{gain}"', is_error=True)

        self.set_status("Running", status_color='greenLight')

    def detailed_settings_changed_func(self, path, idx, new_value):
        # if the path is the gain check that the new setting is one of the potential options
        print(path)
        if "/Gain range" in path: 
            if new_value not in self.GAINS.keys():
                self.client.msg(f'SCM gains must be one of {", ".join(self.GAINS.keys())}. Found "{new_value}"', is_error=True)
                
    def readout_func(self):

        # number of channels to read 
        nchan = 4

        # gain value
        gain = [self.GAINS[g] for g in self.settings['Gain range']]

        # take a measurement for each channel
        data = np.ndarray(nchan, np.float64)
    
        for ch, g in zip(range(nchan), gain):
            val = self.daq.AIn(ch, g)
            data[ch] = self.daq.volts(g, val)

        # make and fill bank with data
        event = midas.event.Event()
        event.create_bank("SCMV", midas.TID_DOUBLE, data)
        return event

class SCMTemps(midas.frontend.EquipmentBase):
    """Readout of SCM temperatures from Lakeshore 218 temperature controller"""

    # default settings
    DEFAULT_SETTINGS = collections.OrderedDict([
        ("serial_port", "ttyUSB0"),
        ("baud_rate", 9600),
    ])

    def __init__(self, client):
        # The name of our equipment. This name will be used on the midas status
        # page, and our info will appear in /Equipment/SCMVoltages in
        # the ODB.
        # the ODB.
        equip_name = "SCMTemperatures"
        
        # Define the "common" settings of a frontend.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 1
        default_common.period_ms = 1000
        default_common.read_when = midas.RO_ALWAYS
        default_common.log_history = 10
        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, 
                                              equip_name, 
                                              default_common, 
                                              self.DEFAULT_SETTINGS)
        
        self.set_status("Initializing", status_color='yellowLight')

        # connect to MCC DAQ
        Lakeshore218.serial_settings['baudrate'] = self.settings['baud_rate']
        self.daq = Lakeshore218(port=f"/dev/{self.settings['serial_port']}")

        self.set_status("Running", status_color='greenLight')

    def settings_changed_func(self):
        self.client.msg('SCMTemperature settings only take effect on frontend restart')
                
    def readout_func(self):

        data = np.array(self.daq.get_temp_K(), dtype=np.float64)

        # make and fill bank with data
        event = midas.event.Event()
        event.create_bank("SCMT", midas.TID_DOUBLE, data)
        return event

class SCMFrontend(midas.frontend.FrontendBase):
    def __init__(self):
        # You must call __init__ from the base class.
        midas.frontend.FrontendBase.__init__(self, "scmfe")
        
        # You can add equipment at any time before you call `run()`, but doing
        # it in __init__() seems logical.
        self.add_equipment(SCMVoltages(self.client))
        self.add_equipment(SCMTemps(self.client))
        
    def begin_of_run(self, run_number):
        return midas.status_codes["SUCCESS"]
        
    def end_of_run(self, run_number):
        return midas.status_codes["SUCCESS"]
    
    def frontend_exit(self):
        self.client.msg("Finished")
        
if __name__ == "__main__":
    # The main executable is very simple - just create the frontend object,
    # and call run() on it.
    with SCMFrontend() as fe:
        fe.run()