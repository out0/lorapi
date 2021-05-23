# lorapi
Implementation of a MQTT&lt;-->LoRa single channel bridge to Raspberry Pi.

This is a simple implementation of a MQTT&lt;-->LoRa bridge for a single channel

It's based on a lot of projects, and documentation, such as:

* The packets forwarder project: https://github.com/tftelkamp/single_chan_pkt_fwd
* The lorawan parser: https://github.com/JiapengLi/lorawan-parser.git
* LMIC library for Arduino https://github.com/matthijskooijman/arduino-lmic
* The PaHo project: https://www.eclipse.org/paho/index.php?page=downloads.php
* MQTT RFC documentation: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/errata01/os/mqtt-v3.1.1-errata01-os-complete.html#_Figure_2.2_-
* Semtech UDP Packet Forwarder protocol specification: https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT

The "packets forwarder project" aims to provide a way for translating from LoRa to UDP only

As for this project, we intend to translate back-and-forth between LoRa and TCP/IP (currently using MQTT, but you can modify it as you please)

Warning:

  The LoRaWAN protocol does not support single channel communication. This is to be used as a test tool only, or as a proof-of-concept tool,
  never to be connected to The Things Network.
  
  To be able to use single chan, the  the method responsible for sending the data starts by sleep-waiting for the correct
  window time to transmit the frame back to the end-device. To keep things simple, we fixed the default window for the LMIC 
  implementation, since the LoRaWAN specifies that those timings could be controlled by commands from the gateway. 
  
  Doing so, we were able to avoid implementing commands from the brigde to the end-device, as it is out of the scope of this project, and not
  recommended for a single channel small project.
    
  The next thing we do is to put the radio in stand-by mode, in order to modify it’s parameters. We then set modulation parameters in sync with 
  what our end-device expects, change the I/Q on the radio, shifting the phase of the carrier to make it look like a gateway transmiting back, 
  set the register flags for the IRQ to control the TxDone event, write the phy-payload to the radio’s internal buffer and put the radio in the
  transmit mode by setting the proper register.
  
  At the end of the transmission, we need to put the device back to receive mode, as we have a transceiver and not a concentrator as a hardware.
