# ESP-SDK
**Paasmer IoT SDK** for **ESP based Devices** like **NodeMCU** and **Adafruit Huzzah**.

## Overview

The **Paasmer SDK** for **ESP based Devices** is a collection of source files that enables you to connect to the Paasmer IoT Platform. It includes the trasnport client for **MQTT** with **TLS** support.  It is distributed in source form and intended to be built into customer firmware along with application code, other libraries and RTOS.

## Featuers

The **ESP-SDK** simplifies access to the Pub/Sub functionality of the **Paasmer IoT** broker via **MQTT**. The SDK has been tested to work with the **Paasmer IoT Platform** and **ESP based Devices** like **NodeMCU** and **Adafruit Huzzah**.

## MQTT Connection

The **ESP-SDK** provides functionality to create and maintain a mutually authenticated TLS connection over which it runs **MQTT**. This connection is used for any further publish operations and allow for subscribing to **MQTT** topics which will call a configurable callback function when these topics are received.

## Pre Requisites

Registration on the portal http://developer.paasmer.co is necessary to connect the device to the **Paasmer IoT Platform** .The SDK has been tested on the Ubuntu 16.10 LTS host with **NodeMCU** and **Adafruit Huzzah**.

* The following support SDK's and toolchain must be installed on the host device.
* ESP OPEN SDK from https://github.com/pfalcon/esp-open-sdk/.
* Dependencies for the above ESP OPEN SDK are highlighted in the above link. 
* Xtensa Tool Chain would be created if ESP OPEN SDK is installed correctly.

## Installation

* Download the SDK or clone it using the command below.

```
$ git clone github.com/PaasmerIoT/ESP-SDK.git
$ cd ESP-SDK
```

* Supply your WiFi credentials, edit `include/ssid_config.h` defining the two macro defines.

```c
#define WIFI_SSID "my wifi ssid"
#define WIFI_PASS "my secret password"
```

* To connect the device to Paasmer IoT Platfrom, the following steps need to be performed.

```
$ cd examples/paasmer_iot
$ sudo ./install.sh
```

* Upon successful completion of the above command, the following commands need to be executed.

```
$ sudo su
$ source ~/.bashrc
$ PAASMER
$ sed -i 's/alias PAASMER/#alias PAASMER/g' ~/.bashrc
$ exit
```

* Edit the config.h file to include the user name(Email), device name, feed names and GPIO pin details.

```c
#define UserName "Email Address" //your user name used in developer.paasmer.co for registration
#define DeviceName "" //your device name
#define feedname1 "feed1" //feed name used for display in the developer.paasmer.co
#define sensorpin1 gpio-pin-no-for-sensor-1 //modify with the pin number which you connected the sensor, eg 6 or 7 or 22
#define feedname2 "feed2" //feed name used for display in the developer.paasmer.co
#define sensorpin2 gpio-pin-no-for-sensor-2 //modify with the pin number which you connected the sensor, eg 6 or 7 or 22
#define feedname3 "feed3" //feed name used for display in the developer.paasmer.co
#define sensorpin3 gpio-pin-no-for-sensor-3 //modify with the pin number which you connected the sensor, eg 6 or 7 or 22
#define analogfeedname "feed4" //feed name you use in the website for analog readings
#define controlfeedname1 "controlfeed1" //feed name used for display in the developer.paasmer.co
#define controlpin1 3 //modify with the pin number which you connected the control device (eg.: motor)
#define controlfeedname2 "controlfeed2" //feed name used for display in the developer.paasmer.co
#define controlpin2 4 //modify with the pin number which you connected the control device (eg.: fan)
#define timePeriod 15000 //change the time delay as you required for sending sensor values to paasmer cloud
```

* Compile the code and flash the code onto the ESP based device.

```
cd ../../
make flash -j4 -C examples/passmer_iot ESPPORT=/dev/ttyUSB0
```

* The device would now be connected to the Paasmer IoT Platfrom and publishing sensor values at specified intervals.

## Support

The support forum is hosted on the GitHub where issues can be identified and the Team from Paasmer would be taking up requstes and resolving them. You could also send a mail to support@paasmer.co with the issue details for resolution.

## Note

The Paasmer IoT ESP-SDK utilizes the features provided by ESP-OPEN-SDK and ESP-OPEN-RTOS
