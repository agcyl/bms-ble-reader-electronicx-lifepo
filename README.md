# bmsreader
A description to readout Electronicx LiFoPo batteries by BLE. 
This repository contains the description to readout the status of LiFoPo batteries from the seller Electronicx - the manufacturer of the builtin BMS (battery management system) ist unknown, but maybe some other resellers use the same BMS.

In general it´s a good idea to allow the monitoring of BMS system with Bluetooth Low Energy with Android or IOS Phones. The downside is, you must be very near to the device. This is fine if you use the battery e.g. in a caravan, but what if it´s somewere in a cellar, garage... ?
To allow the "remote" monitoring, we need a device that can read and transmit the data to other systems, eg. house automation systems. This can be easily done by using a linux-box with an USB-Bluetooth stick supporting BLE and the knowledge how to receive and interpret the data from the BMS. 

The findings in this repository were retrieved by sniffing the protocol between the Android app from the seller and the battery and implementing the communication with the linux bluetooth "gatttool".

The program in this repository describes the process, to trigger the BMS to send the status and to analyze the response to get "useful data" like remaining capacity, current, voltage, soc and temperature.

The C-program is just a quick & dirty reference - all findings are also explained here in detail.

## Prerequisites

First you need a working "bluez" stack on a linux-box, which comes with the programs "hcitool" and most important "gatttool" (there are a lot of howtos in the internet for this setup).

## How to get BLE-data via gatttool

The general workflow is quite simple:

1. Identify the BMS using 

     **```sudo hcitool lescan```**
  
  
   this will show a list of "advertising" BLE devices nearby.
   the BMS will showup like below for a 12V BMS
   
     `LT-12V12345  12:34:56:78:90:01`
      
   the important part to note is the mac-address of the device 12:34... 

2. Using gatttool we have to write a special value to one of the advertised characteristic to tell the BMS, that we want to get notifications: 
   
   to identify the characteristic we do 
   
   **```sudo gatttool -b 12:34:56:78:90:01 --char-desc```** 
   
   
   which will give us an output like
   
   ```
   handle = 0x0001, uuid = 00002800-0000-1000-8000-00805f9b34fb
   handle = 0x0002, uuid = 00002803-0000-1000-8000-00805f9b34fb
   handle = 0x0003, uuid = 00002a05-0000-1000-8000-00805f9b34fb
   handle = 0x0004, uuid = 00002902-0000-1000-8000-00805f9b34fb
   handle = 0x0005, uuid = 00002800-0000-1000-8000-00805f9b34fb
   handle = 0x0006, uuid = 00002803-0000-1000-8000-00805f9b34fb
   handle = 0x0007, uuid = 00002a00-0000-1000-8000-00805f9b34fb
   handle = 0x0008, uuid = 00002803-0000-1000-8000-00805f9b34fb
   handle = 0x0009, uuid = 00002a01-0000-1000-8000-00805f9b34fb
   handle = 0x000a, uuid = 00002803-0000-1000-8000-00805f9b34fb
   handle = 0x000b, uuid = 00002a04-0000-1000-8000-00805f9b34fb
   handle = 0x000c, uuid = 00002800-0000-1000-8000-00805f9b34fb
   handle = 0x000d, uuid = 00002803-0000-1000-8000-00805f9b34fb
   handle = 0x000e, uuid = 6e400003-b5a3-f393-e0a9-e50e24dcca9e
   handle = 0x000f, uuid = 00002902-0000-1000-8000-00805f9b34fb
   handle = 0x0010, uuid = 00002803-0000-1000-8000-00805f9b34fb
   handle = 0x0011, uuid = 6e400002-b5a3-f393-e0a9-e50e24dcca9e 
   ```
   next we use the handle of the uuid 00002902-0000-1000-8000-00805f9b34fb to enable notifications:
   
   **```sudo gatttool -mtu 23  -b 12:34:56:78:90:01 --char-write-req --handle=0x000f  --value=0100```** 
      
   but this is only half of the story - the important part is the next step:
   
   we need to send a "request" to the BMS, to send out the data via notifications.This request consists of an ASCII-string with hex values:
   ":000250000E03~\r\n". 
   This "command" looks like an ASCII-Modbus protocol (start with ':' end with crlf), it is not ! 
   The funny thing is, after playing around this this command it turned out, the only relevant bytes if the last one '03' before '~'.
   you could also send ```:000000000003~\r\n``` and it works.
   
   **```sudo timeout 0.5 gatttool -mtu 23 -b 02:00:07:BD:93:39 --char-write-req --handle=0x0011  --value=3a3030303235303030304530337e0d0a  --listen```** 
      
   and if everything works we will get:
   
   ```
   Characteristic value was written successfully
   Notification handle = 0x000e value: 3a 30 30 38 32 33 31 30 30 38 43 30 30 30 30 30 30 30 30 30
   Notification handle = 0x000e value: 30 30 30 30 30 30 43 42 39 30 43 42 41 30 43 42 42 30 43 42
   Notification handle = 0x000e value: 36 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 
   Notification handle = 0x000e value: 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 
   Notification handle = 0x000e value: 30 30 30 30 30 30 30 30 30 30 30 42 34 30 30 30 30 33 44 33 
   Notification handle = 0x000e value: 44 33 44 33 44 46 30 30 30 30 30 31 33 37 36 30 30 33 31 30 
   Notification handle = 0x000e value: 30 30 30 31 31 30 31 34 30 30 37 44 30 30 37 44 30 42 36 7e 
   ```
   
   The above gatttool command with --listen, will listen endlessly, but we will only receive one answer - so the timout command with .5 second is used. 
   The process will be killed after 0.5 second - this is enough time to receive the answer - if it takes longer it will very likely be not a valid answer!    
   
   The above response is the ASCII representation of 70 bytes payload - each byte is a character e.g. 3a 30 30 38 32 33 31  is :008231
   
   so the whole payload translates to
     ```  
    :008231008C000000000   
    000000CB20CBA0CBB0CB   
    60000000000000000000   
    00000000000000000000   
    00000000000B400003D3   
    D3D3DF00000137600310   
    00011014007D007D0B6~
    ```  
   ## Interpreting the payload
   
   ### Packet framing 
   Every answer starts with ':' and ends with '~'
   
   ### Checksum
   Even if the start and stop bytes are correct, it happens very often with BLE-packets, that there are transmission errors! 
   To be (relatively) sure, that we received what the BMS sent, the last byte before the stop-character represents a checksum.
   
   The "algorithm" used is the same that MODBUS-ASCII uses (again a similarity): **longitudinal redundancy check**:
   
   All ASCII-bytes of the payload, except the start-byte ':' and the last three bytes (2 digits for 1 byte checksum) and the stop-byte '~' are added into one byte (skipping the overflow) the 'result' will then be XOR-ed with 0xFF.
   
   
   
   
   
   # to be continued

   
