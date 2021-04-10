# Passive buzzer via http

This project implements a http server where you can edit the tone of the passive buzzer.

## How to use this project

### Hardware Required

* A ESP32
* A USB cable for power supply and programming
* Two cables
* A passive buzzer

Make sure DAC output pin which is GPIO25 if channel 1 set, GPIO26 if channel 2 set, is connected to the passive buzzer correctly.

### Configure the project
To get this project working you have to change a few settings.

Execute following statements in terminal:

```
idf.py menuconfig
```

Go to 
```
Component config
```

Here at the bottom you can find the 
```
Http controller
```

Here you have to set the ssid and the password to your network via whom we connect.

If you want to change other settings you can do that in the ```Component config```. here go to ```Example  Configuration```.
In the following page are all the settings that are changeable in this project.

#### DAC channel Num (Buzzer)

ESP32 DAC contains two channels:
 * **DAC_CHANNEL_1 (GPIO25), selected by default.**
 * DAC_CHANNEL_2 (GPIO26)

#### Wave form

This example demonstrates one of the following waveforms:
* **Sine, selected by default.**
* Triangle
* Sawtooth
* Square

#### Enable output voltage log

**Disabled selected by default.**

If enabled, expected voltage of each points will be shown in terminal. It's intuitive for debugging and testing. If output example is needed, read **Example Output** chapter below.