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
with integrated OTA functionality. Furthermore we have integrated the Wifi Onboarding mechanism based on the [ESP32-Wifi-Manager](https://github.com/tonyp7/esp32-wifi-manager)

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
###  <a name="configure_ESPIDF"></a>configure your ESP-IDF
After succesfully installation of the ESP-IDF goto your home directory and run the export script
```console
. $HOME/esp/esp-idf/export.sh
```
now you are able to execute idf.py to setup the type: 
```console
idf.py set-target esp32
```
now you are ready to launch the idf.py UI to configure the ESP-IDF.
```console
idf.py menuconfig  
```
configure the partition Table, with factory app and two OTA definition and set the offset to 0x10000. 
(Due to the fact that we are depolying a signed Bootloader image the have to expand the offset)
![](/resources/PartitionTable.png)

next we have to activate the security Feature by: 
* enable hardware Secure Boot in bootloader
  * please select secure Boot version 1
* change file at "signed binaries during build " to "ESPTest01.key"
  * > **_Note_**: To get the signing Key follow the instruction at: [Instruction how to get the Key](#getSigningKey)
![](/resources/SecurityFeature.png)

next we configure the OTA Download Server with the following url: https://trust-point.com/resources/OTA/OTABasic.bin
![](/resources/DownloadServer.png)


### <a name="onboarding"></a>Onboarding to your Network
To onboard your ESP32 board to your preferred wifi network we are providing an Onboarding procedure.

>**_Note:_**
This process is only active while no SSID and PW is stored in the ESP flash. If crendentials
are found in the falsh memory the ESP is booting directly into the active app.

Please follow the steps below:
1. Connect your ESP Board with the USB cable to your USB port.
2. The ESP Board will open a Wifi-AccessPoint called esp32. Please connect. 
3. The Wifi manager will auto launch: 
   1. Select your preferred Network
   2. Enter your Access-code
4. Press Join the network. 

The ESP will store the Network credential in the flash memory, and he will automaticaly reboot.
To verify that everything worked well go back to your Terminal and start the monitor
````console
idf.py idf.py -p <yourPortID>  monitor 
````
You will see the following boot sequence: 
![](/resources/firstStart.png)

### <a name="monitorApplication"></a>Monitor Application
To Monitor the debug Message you have to connect your Computer with the ESP with the USB cable. 
[Configure your ESP-IDF](#configure_ESPIDF) and than start the monitor with: 
````console
idf.py idf.py -p <yourPortID>  monitor 
````

### <a name="OTAprocess"></a>OTA process
Over-the-air (OTA) firmware updates are a Over-the-air (OTA) firmware updates are an important component of any IoT system. Over-the-air firmware updates refer to the practice of updating an embedded device's code remotely.
In our sample application, we use a static link to a download server with a user-triggered activation. 
* We entered the URL of the download server as part of the ESP-IDF configuration. ([Configure your ESP-IDF](#configure_ESPIDF))
* connect our Monitor to the ESP ([monitor your Application](#monitorApplication))
* Pressing the **Boot Button** at the ESP will trigger the Download Process. You will see the following sequence:
![](/resources/OTAStart.png)

The last step of the download process is the verification of the App Signatur
![](/resources/OTASigVerified.png)

After rebooting the ESP you the new image is and activated: 
![](/resources/OTAReboot.png)


### <a name="flashBootloader"></a>Flash Bootloader
Due to the activated Secure Boot process and the signed app feature wer are providing a preconfigured second stage Bootloader Image
You can only flash the second stage bootloader because it is loaded in flash memory. 
Please follow the steps below: 
1. Erase the whole flash memory
>**_Note:_**
This will also delete the stored wifi credentials and you have to run through the onboarding process ([onboarding to you Network](#onboarding))
````console
idf.py -p <yourPortID>  erase_flash
````
2. Load the signed Bootloader with the public signing key
````console
esptool.py write_flash  0x0 bootloaderSignedESPTest01.bin   
````
>**_Note:_**
If you reload the boot loader, you have to delete the flash memory completely in any case. Since it can possibly come to address range overlaps.

### <a name="generateApplication"></a>generate Application

1. clean your Build directory
````console
idf.py fullclean   
````
2. Rebuild the application
````console
idf.py build    
````
 

### <a name="flashApplication"></a>flash Application
1. Flash application and start the Monitor
````console
idf.py flash monitor    
````


#### <a name="getSigningKey"></a>Get the code Signing Key
So that you are able to integrate extension and new functions into the provided app you need the signature key for the app.
Please send an email to info(at)keyfactor.com with the subject: "ESP32 Use case - Signing Key". 
We will be happy to provide you with the signing key for this project.


