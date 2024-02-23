#ifndef LORA_H
#define LORA_H

#include <SPI.h>       //the lora device is SPI based so load the SPI library
#include <SX126XLT.h>  //include the appropriate library

SX126XLT LT;

uint8_t TXPacketL;
uint32_t TXPacketCount, startmS, endmS;
uint8_t buff[20];


//***  Setup hardware pin definitions here ! *****
#ifdef ESP32
#define SCK 18     //SCK on SPI3
#define MISO 19    //MISO on SPI3
#define MOSI 23    //MOSI on SPI3
#define NSS 5      //select pin on LoRa device
#define NRESET 27  //reset pin on LoRa device
#define RFBUSY 25  //busy line
#define DIO1 35    //DIO1 pin on LoRa device, used for RX and TX done
#define SW -1      //SW pin on Dorji devices is used to turn RF switch on\off, set to -1 if not used
#else              //it will be arduino
#define NSS 10     //select pin on LoRa device
#define NRESET 8   //reset pin on LoRa device
#define RFBUSY 4   //busy pin on LoRa device
#define DIO1 7     //DIO1 pin on LoRa device, used for RX and TX done
#define DIO2 -1    //DIO2 pin on LoRa device, normally not used so set to -1
#define DIO3 -1    //DIO3 pin on LoRa device, normally not used so set to -1
#define RX_EN -1   //pin for RX enable, used on some SX126X devices, set to -1 if not used
#define TX_EN -1   //pin for TX enable, used on some SX126X devices, set to -1 if not used
#define SW -1      //SW pin on some Dorji LoRa devices, used to power antenna switch, set to -1 if not used
#endif

#define LORA_DEVICE DEVICE_SX1262  //we need to define the device we are using


//***  Setup LoRa Parameters Here ! *****

//LoRa Modem Parameters
const uint32_t Frequency = 867000000;  //frequency of transmissions in hertz
const uint32_t Offset = 0;             //offset frequency for calibration purposes

const uint8_t Bandwidth = LORA_BW_125;     //LoRa bandwidth
const uint8_t SpreadingFactor = LORA_SF7;  //LoRa spreading factor
const uint8_t CodeRate = LORA_CR_4_5;      //LoRa coding rate
const uint8_t Optimisation = LDRO_AUTO;    //low data rate optimisation setting, normally set to auto

//for SX1262, SX1268 power range is +22dBm to -9dBm
//for SX1261, power range is +15dBm t0 -9dBm

const int8_t TXpower = 10;  //LoRa transmit power in dBm

const uint16_t packet_delay = 5000;  //mS delay between packets

void setuplora() {
  Serial.println("****************************");
  Serial.println("Lora node");
  Serial.println("****************************");
  SPI.begin();


#ifdef ESP32
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, SW, LORA_DEVICE)) {
    Serial.println(F("LoRa Device found with esp based mcu"));
    delay(1000);
  } else {
    Serial.println(F("No device responding"));
  }
#else
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, SW, LORA_DEVICE)) {
    Serial.println(F("LoRa Device found with other chip based mcu"));
    delay(1000);
  } else {
    Serial.println(F("No device responding"));
  }
#endif


  //*********************************
  //Setup LoRa device
  //*********************************
  LT.setMode(MODE_STDBY_RC);
  LT.setRegulatorMode(USE_DCDC);
  LT.setPaConfig(0x04, PAAUTO, LORA_DEVICE);
  LT.setDIO3AsTCXOCtrl(TCXO_CTRL_3_3V);
  LT.calibrateDevice(ALLDevices);  //is required after setting TCXO
  LT.calibrateImage(Frequency);
  LT.setDIO2AsRfSwitchCtrl();
  LT.setPacketType(PACKET_TYPE_LORA);
  LT.setRfFrequency(Frequency, Offset);
  LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate, Optimisation);
  LT.setBufferBaseAddress(0, 0);
  LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on TX done and timeout on DIO1
  LT.setHighSensitivity();                                                     //set for maximum gain
  LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);
  //*********************************

  Serial.println("Transmitter ready");

}

void send_data() {

  TXPacketL = sizeof(buff);  //set TXPacketL to length of array


  LT.printASCIIPacket(buff, TXPacketL);  //print the buffer (the sent packet) as ASCII

  startmS = millis();                                         //start transmit timer
  if (LT.transmit(buff, TXPacketL, 10000, TXpower, WAIT_TX))  //will return packet length sent if OK, otherwise 0 if transmit error
  {
    endmS = millis();  //packet sent, note end time
    TXPacketCount++;
  } else {
    Serial.println("packet sending failed");  //transmit packet returned 0, there was an error
  }
}



#endif LORA_H
