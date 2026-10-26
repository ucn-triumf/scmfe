#!/bin/bash

export MIDASSYS=/home/ucn/packages_tucan01/midas
export PYTHONPATH=$PYTHONPATH:$MIDASSYS/python
export PATH=$PATH:$MIDASSYS/bin

cd /home/ucn/online/scmfe
source venv/bin/activate
python3 scmfe.py
