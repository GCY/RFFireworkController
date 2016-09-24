#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
#include <SoftwareSerial.h>

SoftwareSerial serial(P2_7, P2_6); // RX, TX

Enrf24 radio(P2_0, P2_1, P2_2);  // P2.0=CE, P2.1=CSN, P2.2=IRQ
const uint8_t address[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01 };

void dump_radio_status_to_serialport(uint8_t);

uint8_t ACK = P1_1;
uint8_t H = P1_2;

uint8_t B_1 = P1_3;
uint8_t B_2 = P1_4;
uint8_t B_3 = P2_5;
uint8_t B_4 = P2_4;
uint8_t B_5 = P2_3;

const char *is_connect_start = "ICS";
const char *is_connect_end = "ICE";
const char *is_people_yes = "IPY";
const char *is_people_no = "IPN";
const char *state[] = {"R1","R2","R3","",""};
const uint8_t button_states[] = {B_1,B_2,B_3,B_4,B_5};

void Polling(void);
void People(void);

void setup()
{
  serial.begin(9600);
  
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  radio.begin();  // Defaults 1Mbps, channel 0, max TX power
  dump_radio_status_to_serialport(radio.radioState());

  radio.setTXaddress((void*)address);
  radio.setRXaddress((void*)address);
  radio.enableRX();  // Start listening
  
  pinMode(ACK,OUTPUT);
  pinMode(H,OUTPUT);
  pinMode(B_1,INPUT_PULLUP);
  pinMode(B_2,INPUT_PULLUP);
  pinMode(B_3,INPUT_PULLUP);
  pinMode(B_4,INPUT_PULLUP);
  pinMode(B_5,INPUT_PULLUP);
  
  digitalWrite(ACK,LOW);
  digitalWrite(H,LOW);
  
  boolean init_flag = false;
  for(;!init_flag;){
    radio.print(is_connect_start);
    radio.flush();  // Force transmit (don't wait for any more data)
    dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
    while (radio.available(true)){
      char inbuf[5];
      if(radio.read(inbuf)){
        if(!strcmp(inbuf,is_connect_end)){
          digitalWrite(ACK,HIGH);
          init_flag = true;
        }
      }
    }
  }
}

void loop()
{
  Polling();
  People();
}

void People(void)
{
  while (radio.available(true)){
    char inbuf[5];
    if(radio.read(inbuf)){
      if(!strcmp(inbuf,is_people_yes)){
        digitalWrite(H,HIGH);
      }
      else if(!strcmp(inbuf,is_people_no)){
        digitalWrite(H,LOW);
      }
    }
  }
}

void Polling(void)
{
  for(int i = 0;i < sizeof(button_states)/sizeof(uint8_t);++i){
    if(!(digitalRead(button_states[i]))){
      radio.print(state[i]);
      radio.flush();  // Force transmit (don't wait for any more data)
      dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
    }
  }
}

void dump_radio_status_to_serialport(uint8_t status)
{
  serial.print("Enrf24 radio transceiver status: ");
  switch (status) {
    case ENRF24_STATE_NOTPRESENT:
      serial.println("NO TRANSCEIVER PRESENT");
      break;

    case ENRF24_STATE_DEEPSLEEP:
      serial.println("DEEP SLEEP <1uA power consumption");
      break;

    case ENRF24_STATE_IDLE:
      serial.println("IDLE module powered up w/ oscillators running");
      break;

    case ENRF24_STATE_PTX:
      serial.println("Actively Transmitting");
      break;

    case ENRF24_STATE_PRX:
      serial.println("Receive Mode");
      break;

    default:
      serial.println("UNKNOWN STATUS CODE");
  }
}
