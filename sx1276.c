#include "sx1276.h"
#include "base64.h"

using namespace std;

sx1276::sx1276(int channel, int nssPin, int rst, int di0, int rxFreq, int txFreq, int sf, int bw, int cr)
{
    this->channel = channel;
    ssPin = nssPin;
    dio0Pin = di0;
    rstPin = rst;
    this->rxFreq = rxFreq;
    this->txFreq = txFreq;
    this->sf = sf;

    wiringPiSetup () ;
    pinMode(ssPin, OUTPUT);
    pinMode(dio0Pin, INPUT);
    pinMode(rstPin, OUTPUT);

    //int fd =
    wiringPiSPISetup(channel, 500000);

    setupRadio(rxFreq, txFreq, sf, bw, cr);
}

sx1276::~sx1276()
{
    //dtor
}

void sx1276::die(const char *s)
{
    perror(s);
    exit(1);
}

void sx1276::setFrequency(int freq) {
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    writeRegister(REG_FRF_MSB, (uint8_t)(frf>>16) );
    writeRegister(REG_FRF_MID, (uint8_t)(frf>> 8) );
    writeRegister(REG_FRF_LSB, (uint8_t)(frf>> 0) );
    writeRegister(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word
}
void sx1276::setupLoRaModulation(uint8_t sf, uint8_t bw, uint8_t  cr)
{
    uint8_t regdata;

    //Spreading factor - same for SX1272 and SX127X - reg 0x1D
    regdata = (readRegister(REG_MODEM_CONFIG2) & (~READ_SF_AND_X));
    writeRegister(REG_MODEM_CONFIG2, (regdata + (sf << 4)));
  
    //bandwidth
    regdata = (readRegister(REG_MODEM_CONFIG) & (~READ_BW_AND_X));
    writeRegister(REG_MODEM_CONFIG, (regdata + bw));

    //Coding rate
    regdata = (readRegister(REG_MODEM_CONFIG) & (~READ_CR_AND_X));
    writeRegister(REG_MODEM_CONFIG, (regdata + (cr)));
 
}


void sx1276::setupRadio(int rxFreq, int txFreq, int sf, int bw, int cr) {
    digitalWrite(rstPin, HIGH);
    delay(100);
    digitalWrite(rstPin, LOW);
    delay(100);

    byte version = readRegister(REG_VERSION);
    sx1272 = version == 0x22;

    if (sx1272) {
        // sx1272
        cout << "SX1272 detected...";
    } else {
        digitalWrite(rstPin, LOW);
        delay(100);
        digitalWrite(rstPin, HIGH);
        delay(100);
        version = readRegister(REG_VERSION);
        if (version == 0x12) {
            // sx1276
            cout << "SX1276 detected...";
        } else {
            printf("Unrecognized transceiver: 0x%x\n", version);
            exit(1);
        }
    }

    writeRegister(REG_OPMODE, SX72_MODE_SLEEP);

    // set frequency
    setFrequency(rxFreq);

    uint8_t bwp = 0;
    switch (bw)
    {
    case 125:
    default:
        bwp = LORA_BW_125;
        break;
    case 250:
        bwp = LORA_BW_250;
        break;
    case 500:
        bwp = LORA_BW_500;
        break;
    }

    uint8_t crp = 0;
    switch (crp)
    {
    case 4:
    case 5:
    default:
        crp = LORA_CR_4_5;
        break;
    case 6:
        crp = LORA_CR_4_6;
        break;
    case 7:
        crp = LORA_CR_4_7;
        break;
    case 8:
        crp = LORA_CR_4_8;
        break;
    }

    setupLoRaModulation(sf, bwp, crp);

    setContinousReceiveMode();
    cout << "ok" << endl;

    setGatewayID(NULL);

    printf("Listening at SF%d - BW%d Khz - CR%d\n", sf, bw, cr);
    printf("Rx @%.6lf Mhz.\n", (double)rxFreq/1000000);
    printf("Tx @%.6lf Mhz.\n", (double)txFreq/1000000);
    printf("------------------\n");
}

void sx1276::setContinousReceiveMode() {
    writeRegister(REG_OPMODE, SX72_MODE_STANDBY);
    invertIQ(false);

    if (sf == 10 || sf == 11 || sf == 12) {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x08);
    }
    
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);
    writeRegister(REG_HOP_PERIOD,0xFF);
    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD));
    writeRegister(REG_LNA, LNA_MAX_GAIN);  // max lna gain

    writeRegister(REG_DIOMAPPING1, MAP_DIO0_LORA_RXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    writeRegister(REG_IRQFLAGS, 0xFF);
    writeRegister(REG_IRQ_FLAGS_MASK, ~IRQ_LORA_RXDONE_MASK);

    setFrequency(rxFreq);
    setupLoRaModulation(sf, LORA_BW_125, LORA_CR_4_5);

    writeRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);
}

void sx1276::setGatewayID (char *id) {
    if (id == NULL) {
        ifr = new ifreq();

        int s = 0;
        if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            die("socket");
        }

        ifr->ifr_addr.sa_family = AF_INET;
        strncpy(ifr->ifr_name, "eth0", IFNAMSIZ-1);  // can we rely on eth0?
        ioctl(s, SIOCGIFHWADDR, ifr);

        for (int i = 0; i < 6; i++)
            gatewayID[i] = (unsigned char)(ifr->ifr_hwaddr).sa_data[i];
        gatewayID[6] = 0x0a;
        gatewayID[7] = 0xff;
    } else {
        int n = strlen(id);
        if (n < 8) {
            die("invalid gatewayID. It must have at least 8 bytes");
        }
        for (int i = 0; i < strlen(id); i++)
            gatewayID[i] = id[i];
    }

    printf("Gateway ID: ");
    for (int i=0; i < 8; i++) {
        printf("%.2x", gatewayID[i]);
    }
    printf("\n");
}

signalQuality * sx1276::readSignalQuality() {
    signalQuality *q = new signalQuality();

    int rssicorr;
    byte value = readRegister(REG_PKT_SNR_VALUE);
    
    // The SNR sign bit is 1
    if( value & 0x80 ) {
        // Invert and divide by 4
        value = ( ( ~value + 1 ) & 0xFF ) >> 2;
        q->snr = -value;
    } else {
        // Divide by 4
        q->snr = ( value & 0xFF ) >> 2;
    }
    
    
    if (sx1272) {
        rssicorr = 139;
    } else {
        rssicorr = 157;
    }

    q->packetRssi = readRegister(0x1A)-rssicorr;
    q->rssi = readRegister(0x1B)-rssicorr;
    return q;
}

char * sx1276::receivePacketRaw() {

    long int SNR;
    int rssicorr;

    //di0 pin is used to check for data available
    if(digitalRead(dio0Pin) == 1)
    {
        if(readPayload(message)) {
           
           signalQuality *sq = readSignalQuality();

            printf("Packet RSSI: %d, ", sq->packetRssi);
            printf("RSSI: %d, ", sq->rssi);
            printf("SNR: %li, ", sq->snr);
            printf("Length: %i", (int)receivedbytes);
            printf("\n");

            return message;
        } 

    } 
    return NULL;
}

char * sx1276::receivePacket() {
    char *data = receivePacketRaw();
    
    if (data == NULL)
        return data;

    bin_to_b64((uint8_t *)message, receivedbytes, (char *)(messageB64), 341);
    
    int len = strlen(messageB64);
    char *p = (char *)malloc(sizeof(char)*len+1);
    strncpy(p, messageB64, len);    
    return p;
}

void sx1276::invertIQ(bool invert) {
  if (invert)  {
  	writeRegister(REG_INVERTIQ, 0x66);
  	writeRegister(REG_INVERTIQ2, 0x19);
  }  else  {
  	writeRegister(REG_INVERTIQ, 0x27);
  	writeRegister(REG_INVERTIQ2, 0x1D);  
  }
}


bool sx1276::opModeLora() {
    uint8_t u = OPMODE_LORA;
    u |= 0x8;   // TBD: sx1276 high freq
    writeRegister(REG_OPMODE, u);
    
    return (readRegister(REG_OPMODE) & OPMODE_LORA) != 0;
}


void sx1276::configPower (int txPow) {
    // no boost used for now
    int pw = txPow;
    if(txPow >= 17) {
        pw = 15;
    } else if(pw < 2) {
        pw = 2;
    }
    // check board type for BOOST pin
    writeRegister(RegPaConfig, (uint8_t)(0x80|(pw&0xf)));
    writeRegister(RegPaDac, readRegister(RegPaDac)|0x4);
}

int sx1276::sendPacket(uint8_t *data, int len, int waitBeforeTransmit, int timeout, int window) {

    if (waitBeforeTransmit > 0) {
        delay(waitBeforeTransmit);
    }

    if (!opModeLora()) {
        setContinousReceiveMode();
        return -1;
    }
   
    writeRegister(REG_OPMODE, (readRegister(REG_OPMODE) & ~OPMODE_MASK) | OPMODE_STANDBY);
    if (window == 1) {
        setFrequency(txFreq);
        setupLoRaModulation(sf, LORA_BW_500, LORA_CR_4_5);
    } else {
        setFrequency(txFreq);
        setupLoRaModulation(sf, LORA_BW_125, LORA_CR_4_5);
    }

    writeRegister(RegPaRamp, (readRegister(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
    configPower(14);
    writeRegister(REG_SYNC_WORD, 0X34);
    writeRegister(REG_DIOMAPPING1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    writeRegister(REG_IRQFLAGS, 0xFF);
    writeRegister(REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

    writeRegister(REG_FIFO_TX_BASE_AD, 0x00);
    writeRegister(REG_FIFO_ADDR_PTR, 0x00);
    writeRegister(REG_PAYLOAD_LENGTH, len);
    
    writeBuffer(REG_FIFO, data, len);
    invertIQ(true);

    writeRegister(REG_OPMODE, (readRegister(REG_OPMODE) & ~OPMODE_MASK) | OPMODE_TX);

    unsigned long j = millis() + (unsigned long)timeout;
    uint8_t p = 0;
    do {
        p = readRegister(REG_IRQ_FLAGS);
        delay(1);
        j++;
    } while ((bitRead(p, 3) == 0) && (millis() < j));

    setContinousReceiveMode();
    return bitRead(p, 3) == 1 ? 0 : -2;
} 


/*
void clearIrqStatus(uint16_t irqMask) {
    //clear standard IRQs
                   
}*/

/*
void sx1276::sendStat() {

    static char status_report[STATUS_SIZE]; // status report as a JSON object
    char stat_timestamp[24];
    time_t t;

    int stat_index=0;

    // pre-fill the data buffer with fixed fields
    status_report[0] = PROTOCOL_VERSION;
    status_report[3] = PKT_PUSH_DATA;

    status_report[4] = (unsigned char)(ifr->ifr_hwaddr).sa_data[0];
    status_report[5] = (unsigned char)(ifr->ifr_hwaddr).sa_data[1];
    status_report[6] = (unsigned char)(ifr->ifr_hwaddr).sa_data[2];
    status_report[7] = 0xFF;
    status_report[8] = 0xFF;
    status_report[9] = (unsigned char)(ifr->ifr_hwaddr).sa_data[3];
    status_report[10] = (unsigned char)(ifr->ifr_hwaddr).sa_data[4];
    status_report[11] = (unsigned char)(ifr->ifr_hwaddr).sa_data[5];

    // start composing datagram with the header
    uint8_t token_h = (uint8_t)rand(); // random token
    uint8_t token_l = (uint8_t)rand(); // random token
    status_report[1] = token_h;
    status_report[2] = token_l;
    stat_index = 12; // 12-byte header

    // get timestamp for statistics
    t = time(NULL);
    strftime(stat_timestamp, sizeof stat_timestamp, "%F %T %Z", gmtime(&t));

    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index, "{\"stat\":{\"time\":\"%s\",\"lati\":%.5f,\"long\":%.5f,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%.1f,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}", stat_timestamp, lat, lon, (int)alt, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, (float)0, 0, 0,platform,email,description);
    stat_index += j;
    status_report[stat_index] = 0; // add string terminator, for safety

    printf("stat update: %s\n", (char *)(status_report+12)); // DEBUG: display JSON stat

    //send the update
    //sendudp(status_report, stat_index);
}
*/
#define RegLna                                     0x0C // common
#define LNA_RX_GAIN (0x20|0x1)
#define LORARegPayloadMaxLength                    0x23

bool sx1276::readPayload(char *payload) {
    // clear rxDone
    writeRegister(REG_IRQ_FLAGS, 0x40);

    int irqflags = readRegister(REG_IRQ_FLAGS);

    cp_nb_rx_rcv++;

    //  payload crc: 0x20
    if((irqflags & 0x20) == 0x20)
    {
        printf("CRC error\n");
        writeRegister(REG_IRQ_FLAGS, 0x20);
        return false;
    } else {

        cp_nb_rx_ok++;

        byte currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
        byte receivedCount = readRegister(REG_RX_NB_BYTES);
        receivedbytes = receivedCount;

        writeRegister(REG_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; i++)
        {
            payload[i] = (char)readRegister(REG_FIFO);
        }
    }
    return true;
}



byte sx1276::readRegister(byte addr)
{
    unsigned char spibuf[2];

    spibuf[0] = addr & 0x7F;
    spibuf[1] = 0x00;

    digitalWrite(ssPin, LOW);
    wiringPiSPIDataRW(channel, spibuf, 2);
    

    return spibuf[1];
}

void sx1276::writeRegister(byte addr, byte value)
{
    unsigned char spibuf[2];
    spibuf[0] = addr | 0x80;
    spibuf[1] = value;

    digitalWrite(ssPin, LOW);
    wiringPiSPIDataRW(channel, spibuf, 2);
    digitalWrite(ssPin, HIGH);
}
void sx1276::writeBuffer(byte addr, uint8_t * buffer, uint8_t len)
{
    unsigned char spibuf[2];
    spibuf[0] = addr | 0x80;

    for (int i =0; i < len; i++) {
        spibuf[1] = buffer[i];
        digitalWrite(ssPin, LOW);
        wiringPiSPIDataRW(channel, spibuf, 2);
        digitalWrite(ssPin, HIGH);
    }
    
}