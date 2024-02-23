#ifndef LORA_H
#define LORA_H

#include <SPI.h>       //the lora device is SPI based so load the SPI library
#include <SX126XLT.h>  //include the appropriate library

//*******  Setup hardware pin definitions here ! ***************//
#ifdef ESP32
#define SCK 18                                  //SCK on SPI3
#define MISO 19                                 //MISO on SPI3 
#define MOSI 23                                 //MOSI on SPI3 

#define NSS 5                                   //select pin on LoRa device
#define NRESET 27                               //reset pin on LoRa device
#define RFBUSY 25                               //busy line

#define DIO1 35                                 //DIO1 pin on LoRa device, used for RX and TX done 
#define VCCPOWER 14                             //pin controls power to external devices
#else 
#define NSS 10                                  //select pin on LoRa device
#define NRESET 8                                //reset pin on LoRa device
#define RFBUSY 4                                //busy pin on LoRa device 
#define DIO1 7                                  //DIO1 pin on LoRa device, used for RX and TX done 
#define DIO2 -1                                 //DIO2 pin on LoRa device, normally not used so set to -1 
#define DIO3 -1                                 //DIO3 pin on LoRa device, normally not used so set to -1
#define RX_EN -1                                //pin for RX enable, used on some SX126X devices, set to -1 if not used
#define TX_EN -1                                //pin for TX enable, used on some SX126X devices, set to -1 if not used 
#endif

#define SW -1                                   //SW pin on Dorji devices is used to turn RF switch on\off, set to -1 if not used    

#define LORA_DEVICE DEVICE_SX1262               //we need to define the device we are using

SX126XLT LT;  //create a library class instance called LT




//*******  Setup LoRa Parameters Here ! ***************

//LoRa Modem Parameters
const uint32_t Frequency = 867000000;           //frequency of transmissions in hertz
const uint32_t Offset = 0;                      //offset frequency for calibration purposes

const uint8_t Bandwidth = LORA_BW_125;          //LoRa bandwidth
const uint8_t SpreadingFactor = LORA_SF7;       //LoRa spreading factor
const uint8_t CodeRate = LORA_CR_4_5;           //LoRa coding rate
const uint8_t Optimisation = LDRO_AUTO;         //low data rate optimisation setting, normally set to auto

const int8_t TXpower = 10;                       //LoRa transmit power in dBm

#define RXBUFFER_SIZE 32                        //RX buffer size  


uint8_t RXBUFFER[RXBUFFER_SIZE];  //create the buffer that received packets are copied into

uint8_t RXPacketL;  //stores length of packet received


void setuplora(){
  Serial.println("****************************");
  Serial.println("Lora gateway");
  Serial.println("****************************");
  SPI.begin();


#ifdef ESP32
  //setup hardware pins used by device, then check if device is found
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, SW, LORA_DEVICE)) {
    Serial.println(F("LoRa Device found using esp based mcu"));
  } else {
    Serial.println(F("No device responding"));
  }
#else
  if ( LT.begin(NSS, NRESET, RFBUSY, DIO1, DIO2, DIO3, RX_EN, TX_EN, SW, LORA_DEVICE)) {
    Serial.println(F("LoRa Device found using other chip based mcu"));
  } else {
    Serial.println(F("No device responding"));
  }
#endif

    //***************************************************************************************************
  //Setup LoRa device
  //***************************************************************************************************
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
  LT.setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);  //set for IRQ on TX done and timeout on DIO1
  LT.setHighSensitivity();                                                     //set for maximum gain
  LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);
  //***************************************************************************************************

  Serial.print("Gateway ready");
  delay(1000);
}


#endif LORA_H
