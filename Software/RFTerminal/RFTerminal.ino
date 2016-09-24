#include <Enrf24.h>
#include <nRF24L01.h>
#include <string.h>
#include <SPI.h>
#include <SoftwareSerial.h>

SoftwareSerial serial(P2_7, P2_6); // RX, TX

Enrf24 radio(P2_0, P2_1, P2_2);  // P2.0=CE, P2.1=CSN, P2.2=IRQ
const uint8_t address[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01 };

void dump_radio_status_to_serialport(uint8_t);

volatile const unsigned long DT = 1000; // ms
volatile unsigned long r1_last_time = 0;
volatile unsigned long r2_last_time = 0;
volatile unsigned long r3_last_time = 0;

uint8_t RX_LED = P1_1;
uint8_t PIR = P1_2;

uint8_t FD1 = P1_3;
uint8_t FD2 = P1_4;

uint8_t R1 = P2_5;
uint8_t R2 = P2_4;
uint8_t R3 = P2_3;

void Polling(void);
void R1_Event(void);
void R2_Event(void);
void R3_Event(void);
void End(void);

void People(void);

const char *is_connect_start = "ICS";
const char *is_connect_end = "ICE";
const char *is_people_yes = "IPY";
const char *is_people_no = "IPN";
const char *state[] = {"R1","R2","R3"};
static void (*Event[])(void) = {R1_Event,R2_Event,R3_Event};

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
  
  pinMode(RX_LED,OUTPUT);
  pinMode(PIR,INPUT);
  pinMode(R1,OUTPUT);
  pinMode(R2,OUTPUT);
  pinMode(R3,OUTPUT);
  
  digitalWrite(RX_LED,LOW);
  digitalWrite(R1,LOW);
  digitalWrite(R2,LOW);
  digitalWrite(R3,LOW);
  
  boolean init_flag = false;
  for(;!init_flag;){
    while (radio.available(true)){
      char inbuf[5];
      if(radio.read(inbuf)){
        if(!strcmp(inbuf,is_connect_start)){
          radio.print(is_connect_end);
          radio.flush();  // Force transmit (don't wait for any more data)
          dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
          init_flag = true;
          digitalWrite(RX_LED,HIGH);
        }
      }
    }
  }
}

void loop()
{ 
  dump_radio_status_to_serialport(radio.radioState());
  
  People();
  Polling();
  End();
  
}

void People(void)
{
  if(digitalRead(PIR)){
    radio.print(is_people_yes);
    radio.flush();  // Force transmit (don't wait for any more data)
    dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
  }
  else{
    radio.print(is_people_no);
    radio.flush();  // Force transmit (don't wait for any more data)
    dump_radio_status_to_serialport(radio.radioState());  // Should report IDLE
  }
}

void Polling(void)
{
  char inbuf[5];
  while (radio.available(true)){
    if(radio.read(inbuf)){
      for(int i = 0;i < sizeof(state)/sizeof(char*);++i){
        if(!strcmp(inbuf,state[i])){
          Event[i]();
        }
      }
    }
  }
}

void R1_Event(void)
{
  digitalWrite(R1,HIGH);
  r1_last_time = millis();
}

void R2_Event(void)
{
  digitalWrite(R2,HIGH);
  r2_last_time = millis();  
}

void R3_Event(void)
{
  digitalWrite(R3,HIGH);
  r3_last_time = millis();
}

void End(void)
{
  if((millis() - r1_last_time) > DT){
    digitalWrite(R1,LOW);
  }
  if((millis() - r2_last_time) > DT){
    digitalWrite(R2,LOW);
  }  
  if((millis() - r3_last_time) > DT){
    digitalWrite(R3,LOW);
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
