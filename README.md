# Passive buzzer

This component implements functions to make surtain tones and music via bluetooth.

This component uses the classic bluetooth on the esp32 and the dac and timer drivers from esp-idf.

## How to use this component

### Hardware Required

* A ESP32
* A USB cable for power supply and programming
* Two cables
* A passive buzzer

Make sure DAC output pin which is GPIO25 if channel 1 set, GPIO26 if channel 2 set, is connected to the passive buzzer correctly.

### Configure the project
To get this component working you have to change a few settings.

Execute following statements in terminal:

```
idf.py menuconfig
```

Follow the steps to enable bluetooth.
In `Component config` go to `Bluetooth`.
Enable bluetooth by pressing enter.

Then go to `Bluetooth controller` and then `Bluetooth controller mode`.
Here you would want to enable `BR/EDR Only`.

Then go back to the page where you enabled bluetooth.
Here go to `Bluedroid Options` and enable the following in order.
`Classic Bluetooth` 
`SPP`
.

If you want to change other settings you can do that in the root menuconfig. here go to `Example  Configuration`.
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