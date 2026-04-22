"""
    SCM Voltage tabs frontend equipment
    Readout to MIDAS

    Derek Fujimoto
    April 2026
"""

import uldaq
import midas
import midas.client

# TESTING

# get list of devices
devices = uldaq.get_daq_device_inventory(uldaq.InterfaceType.USB)

with uldaq.DaqDevice(devices[0]) as daq_device:
    # Get AiDevice and AiInfo objects for the analog input subsystem
    ai_device = daq_device.get_ai_device()
    ai_info = ai_device.get_info()

    # Read and display voltage values for all analog input channels
    for channel in range(ai_info.get_num_chans()):
        data = ai_device.a_in(channel, uldaq.AiInputMode.SINGLE_ENDED,
                                uldaq.Range.BIP10VOLTS, uldaq.AInFlag.DEFAULT)
        print('Channel', channel, 'Data:', data)