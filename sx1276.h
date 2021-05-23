#ifndef SX1276_CPP_H
#define SX1276_CPP_H

#include <iostream>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <net/if.h>
#include <cstring>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <math.h>

#define REG_FIFO                    0x00
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_SYMB_TIMEOUT_LSB  		0x1F
#define REG_PKT_SNR_VALUE			0x19
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_MAX_PAYLOAD_LENGTH 		0x23
#define REG_HOP_PERIOD              0x24
#define REG_SYNC_WORD				0x39
#define REG_VERSION	  				0x42
#define REG_INVERTIQ                0x33
#define REG_INVERTIQ2               0x3B

#define SX72_MODE_RX_CONTINUOS      0x85
#define SX72_MODE_TX                0x83
#define SX72_MODE_SLEEP             0x80
#define SX72_MODE_STANDBY           0x81

#define OPMODE_LORA                 0x80

#define PAYLOAD_LENGTH              0x40

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN		    	0x20

// CONF REG
#define REG1                        0x0A
#define REG2                        0x84

#define RegPaConfig                                0x09 // common
#define RegPaRamp                                  0x0A // common
#define RegPaDac                                   0x5A // common


#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE  0x01 // mandated for SF11 and SF12

// FRF
#define        REG_FRF_MSB              0x06
#define        REG_FRF_MID              0x07
#define        REG_FRF_LSB              0x08

#define        FRF_MSB                  0xD9 // 868.1 Mhz
#define        FRF_MID                  0x06
#define        FRF_LSB                  0x66


#define    LORA_BW_500                  144
#define    LORA_BW_250                  128
#define    LORA_BW_125                  112
#define    LORA_CR_4_5                  0x02
#define    LORA_CR_4_6                  0x04
#define    LORA_CR_4_7                  0x06
#define    LORA_CR_4_8                  0x08
#define READ_BW_AND_2                               0xC0  //register 0x1D
#define READ_BW_AND_X                               0xF0  //register 0x1D

#define READ_CR_AND_2                               0x38  //register 0x1D
#define READ_CR_AND_X                               0x0E  //register 0x1D

#define READ_SF_AND_2                               0xF0  //register 0x1E
#define READ_SF_AND_X                               0xF0  //register 0x1E
#define LNA_RX_GAIN (0x20|0x1)

#define BUFLEN 2048  //Max length of buffer

#define PROTOCOL_VERSION  1
#define PKT_PUSH_DATA 0
#define PKT_PUSH_ACK  1
#define PKT_PULL_DATA 2
#define PKT_PULL_RESP 3
#define PKT_PULL_ACK  4

#define TX_BUFF_SIZE  2048
#define STATUS_SIZE	  1024
#define REG_FIFOADDRPTR        0x0D
#define MAP_DIO0_LORA_RXDONE   0x00  // 00------
#define MAP_DIO0_LORA_TXDONE   0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT   0x00  // --00----
#define MAP_DIO1_LORA_NOP      0x30  // --11----
#define MAP_DIO2_LORA_NOP      0xC0  // ----11--
#define IRQ_LORA_TXDONE_MASK    0x08
#define REG_FIFORXBASEADDR  0x0F
#define WREG_FIFO           0x80
#define REG_PAYLOADLENGTH   0x22
#define RADIO_RAMP_DEFAULT  0x09
#define IRQ_RADIO_ALL       0xFFFF
#define IRQ_TX_DONE         0x08  //active on DIO0 
#define REG_IRQFLAGSMASK    0x11
#define REG_DIOMAPPING1     0x40
#define REG_IRQFLAGS        0x12
#define LORA_MODE_TX        0x83
#define OPMODE_TX           0x03
#define MAP_DIO0_LORA_RXDONE   0x00  // 00------
#define MAP_DIO0_LORA_TXDONE   0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT   0x00  // --00----
#define MAP_DIO1_LORA_NOP      0x30  // --11----
#define MAP_DIO2_LORA_NOP      0xC0  // ----11--
#define RegLna                                     0x0C // common
#define LORARegPayloadMaxLength                    0x23
#define OPMODE_MASK      0x07
#define OPMODE_STANDBY   0x01
#define IRQ_LORA_RXDONE_MASK 0x40



#define bitRead(value, bit) (((value) >> (bit)) & 0x01) 

typedef unsigned char byte;

typedef struct signalQuality {
    long int snr;
    int rssi;
    int packetRssi;
} signalQuality;

class sx1276 {
    public:
        sx1276(int channel, int nssPin, int rst, int di0, int rxFreq, int txFreq, int sf, int bw, int cr);
        virtual ~sx1276();
        char * receivePacketRaw();
        char * receivePacket();
        int sendPacket(uint8_t *data, int len, int waitBeforeTransmit, int timeout, int window);
        void setGatewayID(char *id);
        uint8_t gatewayID[8];
    protected:

    private:
        int channel;
        int ssPin;
        int dio0Pin;
        int rstPin;
        int rxFreq;
        int txFreq;
        int sf;
        bool sx1272;
        void selectreceiver();
        void unselectreceiver();
        byte readRegister(byte addr);
        void writeRegister(byte addr, byte value);
        bool readPayload(char *payload);
        void setupRadio(int rxFreq, int txFreq, int sf, int bw, int cr);
        void die(const char *s);
        void invertIQ(bool invert);
        void setContinousReceiveMode();
        void setFrequency(int freq);
        void setupLoRaModulation(uint8_t sf, uint8_t bw, uint8_t cr);
        signalQuality * readSignalQuality();
        void setOpModeLoRa();
        void configPower (int txPow);
        void writeBuffer(byte addr, uint8_t * buffer, uint8_t len);
        bool opModeLora();

        byte currentMode = 0x81;
        char message[256];
        char messageB64[256];

        byte receivedbytes;
        struct ifreq *ifr;

        uint32_t cp_nb_rx_rcv;
        uint32_t cp_nb_rx_ok;
        uint32_t cp_nb_rx_bad;
        uint32_t cp_nb_rx_nocrc;
        uint32_t cp_up_pkt_fwd;
};

#endif // SX1276_CPP_H
