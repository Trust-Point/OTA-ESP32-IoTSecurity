# IoT Device Security in practice
Enabling security features for IoT devices, especially when you want to take advantage of the microcontroller's built-in functions
and features of the microcontroller, can be challenging. This project will help you understand the built-in security feature 
of an ESP32 MCU and provide you with tools and mechanisms to enable these functions at scale.

### <a name="prerequisites"></a>Prerequisites
To build and run Demo system you need the following tools and systemconfiguration: 
* installed an runnig ESP-IDF (Espressif IoT Development Framework). More information at [ESPRESSIF ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
* The Code Signing Private Key.  [Instruction how to get the Key](#getSigningKey)

Hardware: 
* An ESP32 board with ESP Revision 1; 
* USB cable - USB A / micro USB B.
* Computer running Windows, Linux, or macOS.

## Getting Started 
### <a name="What we provide"></a>What we provide
Within this repository we provide you with a development framework, which allows you to build applications (app)
with integrated OTA functionality. Furthermore we have integrated the Wifi Onboarding mechanism based on the [ESP32-Wifi-Manager](https://github.com/tonyp7/esp32-wifi-manager).

###Getting started with a your personal Board:
You have visited us at our booths at Hannover Messe or RSA Show and you are holding one of the ESP32 NodeMCU kit?
Then follow the steps below:


* [configure your ESP-IDF](#configure_ESPIDF):
  * setup your Development enviroment
  * configure the ESP-IDF with the idf.py tool
  * Add partifion Table configuration and offset
* [onboarding to you Network](#onboarding):
  * Connect your ESP Board 
  * Launch the Onboarding Application
  * configure your Network
* [Monitor Application](#monitorApplication):
  * monitor your app
* [OTA process](#OTAprocess):
  * start the OTA process
  * verfiy signatur and change partition table and 

###Getting started with a clean development Board
* [configure your ESP-IDF](#configure_ESPIDF):
  * setup your Development enviroment
  * configure the ESP-IDF with the idf.py tool
  * Add partifion Table configuration and offset
* [Flash Bootloader](#flashBootloader):
  * How to load the provides Bootloader.
  * How to load the root key and burn the eFuse
* [generate Application](#generateApplication):
  * build a signed application from scratch 
* [flash Application](#flash Application:
  * Flash the application and start the monitor
* [onboarding to you Network](#onboarding):
  * Connect your ESP Board 
  * Launch the Onboarding Application
  * configure your Network
* [Monitor Application](#monitorApplication):
  * monitor your app
* [OTA process](#OTAprocess):
  * start the OTA process
  * verfiy signatur and change partition table and 

###Development Process: 

####Generate new Application without cleaning Wifi Parameter
build and flash process

####Generate new Application with clean Wifi Paramter
erase Flash 
Flash Bootloader
build and flash process


## Information
####  <a name="configure_ESPIDF"></a>configure your ESP-IDF
After succesfully installation of the ESP-IDF goto your home directory and run the export script
```console
. $HOME/esp/esp-idf/export.sh
```
now you are able to execute idf.py to setup the type: 
```console
idf.py set-target esp32
```
now we are ready to launch the idf.py UI to configure the ESP-IDF.
```console
idf.py menuconfig  
```
configure the partition Table, with factory app and two OTA definition and set the offset to 0x10000. 
(Due to the fact that we are depoying a signed Bootloader image the have to expand the offse)
![](/resources/PartitionTable.png)

next we have to activate the security Feature by: 
* enable hardware Secure Boot in bootloader
  * please select secure Boot version 1
* change file at "signe binaries durign build " to ESPTest01.key
  * To get the signing Key follow the instruction at [Instruction how to get the Key](#getSigningKey)
![](/resources/SecurityFeature.png)

next we configure the OTA Download Server url: https://trust-point.com/resources/OTA/OTABasic.bin
![](/resources/DownloadServer.png)


### <a name="onboarding"></a>onboarding to you Network
To onboard your ESP32 board to your prefered wifi we are providing an auto-onboarding tool.

Note: This process is only activated since no SSID and PW is stored in the ESP flash. If crendential s
are found in the falsh memory the esp is booting directly into the active app.

Please connect your ESP Board with the USB cable to your port.
The ESP Board will open a wifi called esp32. Please connect. 
The Wifi manager will auto launch: 


* Connect your ESP Board 
* Launch the Onboarding Application
* configure your Network 

### <a name="monitorApplication"></a>Monitor Application
Monitor Application

### <a name="OTAprocess"></a>OTA process
* OTA process

### <a name="flashBootloader"></a>Flash Bootloader
Flash Bootloader

### <a name="generateApplication"></a>generate Application
generate Application 

### <a name="flashApplication"></a>flash Application
flash Application

#### <a name="getSigningKey"></a>Get the code Signing Key
So that you are able to integrate extension and new functions into the provided app you need the signature key for the app.
Please send an email to info(at)keyfactor.com with the subject: "ESP32 Use case - Signing Key". 
We will be happy to provide you with the signing key for this project.


