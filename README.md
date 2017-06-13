# LoRaWANWebService

1. Introduction
----------------

The aim of this project is to implement a web service via IPv6, UDP and CoAP on LoRaWAN devices. SCHC (as described in the Internet-Draft of the lpwan IETF WG: https://datatracker.ietf.org/doc/draft-ietf-lpwan-ipv6-static-context-hc/) is used to compress the header of the IPv6 and UDP protocol. LwIP is used to provide an IPv6 stack on the used hardware platform (SK-iM880A). The LoRaMAC code developed by Semtech is used for this project.

2. Interfaces & Code
----------------
Three extra LwIP interfaces were created for this project
-virtualloraif: is responsible for the connection between the LoRaMAC code and the LwIP (L2 <-> L3, L4, L5)
-schcCompressor: this interfaces provides the functionality for a SCHC compressor and a decompressor
-CoAP interface: this interface provides the CoAP client functionality.

They first two interfaces are located in the \LwIP\netif folder.

For the implementation of the CoAP client the PicoCoAP library was used. 
The code can be found in: \LwIP\apps\picocoap

3. Acknowledgments
-------------------
The mbed (https://mbed.org/) project was used at the beginning as source of
inspiration.

This program uses the AES algorithm implementation (http://www.gladman.me.uk/)
by Brian Gladman.

This program uses the CMAC algorithm implementation
(http://www.cse.chalmers.se/research/group/dcs/masters/contikisec/) by
Lander Casado, Philippas Tsigas.

This program uses the IP stack: LwIP
(https://savannah.nongnu.org/projects/lwip/)

4. Dependencies
----------------
This program depends on a specific hardware platform. Currently the supported platforms is:

The code is written for the:
    - SK-iM880A ( IMST starter kit )
        MCU     : STM32L151CB - 128K FLASH, 10K RAM, Timers, SPI, I2C,
                                USART,
                                USB 2.0 full-speed device/host/OTG controller,
                                DAC, ADC, DMA
        RADIO   : SX1272
        ANTENNA : Connector for external antenna
        BUTTONS : 1 Reset, 3 buttons + 2 DIP-Switch
        LEDS    : 3
        SENSORS : Potentiometer
        GPS     : Possible through pin header GPS module connection
        SDCARD  : No
        EXTENSION HEADER : Yes, all IMST iM880A module pins
        REMARK  : None
        
    
5. Getting started
---------
A Project for CooCox-CoIDE is available: coIDE\SK-iM880A\LoRaMac\classA\LoRaMacClassA-api-v3.coproj



