# Passive buzzer via http

This project implements a http server where you can edit the tone of the passive buzzer.

For this project are the following examples used:
- esp-idf\examples\peripherals\wave_gen
- esp-idf\examples\wifi\getting_started\softAP

## How to use this project

### Hardware Required

* A ESP32
* A USB cable for power supply and programming
* Two cables
* A passive buzzer

Make sure DAC output pin which is GPIO25 if channel 1 set, GPIO26 if channel 2 set, is connected to the passive buzzer correctly.

### Useage

To use this project you have to connect to the wifi acces point called "BuzzerController" with the password "password".
After this you have to go to the following ip address with the selected port (by default 3500).
If done with mobile browser make sure you use desktop perview otherwise the esp will throw an error.
So the correct link would be as follows:

```
http://192.168.4.1:3500/
```



### Configure the project
To get change settings in this project you have to use the following things.

Execute following statements in terminal:

```
idf.py menuconfig
```

Go to 
```
Component config
```

Here at the bottom you can find the two components with all the possible settings.

### Http controller

#### WiFi SSID

This is the place to set the name of the network acces point
**Default BuzzerController**

#### WiFi Password

This is the place to set the password of the network acces point.
The passsword must be longer then 8 characters.
**Default password**

#### WiFi port

Port used to communicate with the http server.
**Default 3500**

#### WiFi channel

The wifi channel is on what frequency the WiFi is. 
**Default channel 1**

#### Maximal STA connections

The amount of clients that can connect to the network
**Default 4**

#### Request header length

Variable used to define how large the request headers can be in bytes
**Default 512**

#### URI header length

Variable used to define how large the uri headers can be in bytes
**Default 512**

### Buzzer controller

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