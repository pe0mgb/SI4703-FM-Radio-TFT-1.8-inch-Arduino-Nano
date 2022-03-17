# SI4703-FM-Radio-TFT-1.8-inch-Arduino-Nano
This program controls a SI4703 by means of an Arduino Nano together with a rotary encoder and rotary switch.

    Frequency : 87.5 MHz - 108MHz
    Frequency display
    Stereo indicator
    RDS station name
    Signalstrenghtmeter in dBuV

All functions are controlled by the rotary encoder.
4 Functions implemented :

    Volume control with volume indicator.
    Presets
    Tune 
    Scan
    
Last status of the radio is stored in the eeprom. 

Preset stations are hardcoded in the sketch.


The following libraries are used :

    GitHub - mathertel/Radio           : An Arduino library to control FM radio chips like SI4703, SI4705, RDA5807M, TEA5767. 

    GitHub - mathertel/RotaryEncoder   : RotaryEncoder Arduino Library 

    GitHub - mathertel/OneButton       : An Arduino library for using a single button for multiple purpose input. 

    https://github.com/Bodmer/TFT_ST7735
        user_setup file dedicated to the hardware available.    
    
