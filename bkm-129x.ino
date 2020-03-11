/* A replication of the workings of the Sony BKM-129X card
 * (2020) Martin Hejnfelt (martin@hejnfelt.com)
 *
 * Version 1.0
 *
 */

#include <SPI.h>
#include "digitalWriteFast.h"

#define BX_OE_n (PD6) // Output enable (active low)
#define EXT_SYNC_OE_n (PD7) // External sync enable (active low)
#define SLOT_ID (PD2) // The hacker gpio that does some handshake magic and shit...

// Serial 5001337 (make up your own), ascii numbers 0x30-0x39 although letters seem to work, but not asciitable
byte serial[7] = { 0x35, 0x30, 0x30, 0x31, 0x33, 0x33, 0x37 };

enum Command {
  READ_MEM    = 0x9E,
  READ_MEM2   = 0xBE,
  SEL_EXT_ON  = 0xF4,
  DESELECT2   = 0xF5,
  SEL_EXT_OFF = 0xF6,
  DESELECT    = 0xF7,
};

enum State {
  IDLE              = 0x00,
  GET_MEM_ADDR      = 0x01,
  GET_MEM_ADDR2     = 0x02,
  PROCESS_MEM_REQ   = 0x03,
  PROCESS_MEM2_REQ  = 0x04,
  SET_STATE         = 0x05,
  PROCESS_SET_STATE = 0x06
};

byte currentState = IDLE;
bool activeSlotID = false;
byte mem[256];
byte mem2[256];

// Like read during powerup
void init_mem() {
  mem[0x00]=0xC8;
  mem[0x02]=serial[0];
  mem[0x03]=serial[1];
  mem[0x04]=serial[2];
  mem[0x05]=serial[3];
  mem[0x06]=serial[4];
  mem[0x07]=serial[5];
  mem[0x08]=serial[6];
  mem[0x0E]=0xFF;
  mem[0x0F]=0xFF;
  mem[0x10]=0xFF;
  mem[0x11]=0xFF;
  mem[0x12]=0xFF;
  mem[0x13]=0xFF;
  mem[0x14]=0xFF;
  mem[0x15]=0xFF;
  mem[0x18]=0xF7;
  mem[0x19]=0x7F;
  mem[0x1A]=0xBB;
  mem[0x1B]=0xFF;
  mem[0x1C]=0xFF;
  mem[0x1D]=0xFF;
  mem[0x1E]=0xFF;
  mem[0x1F]=0xFF;
  mem[0x32]=0x01;
  mem[0x45]=0x18;
  mem[0x58]=0x19;
  mem[0x6B]=0x02;
  mem[0x7E]=0x28;
  mem[0x91]=0x29;
  mem[0xA4]=0x03;
  mem[0xB7]=0x38;
  mem[0xCA]=0xFF;
  mem[0xDD]=0xFF;
  mem[0xF0]=0x39;

  mem2[0x03]=0x04;
  mem2[0x16]=0x48;
  mem2[0x29]=0x49;
  mem2[0x3C]=0x05;
  mem2[0x4F]=0x58;
  mem2[0x62]=0x59;
  mem2[0x75]=0x06;
  mem2[0x88]=0x68;
  mem2[0x9B]=0x07;
  mem2[0xAE]=0x78;
  mem2[0xC1]=0x08;
  mem2[0xD4]=0x88;
  mem2[0xE7]=0xFF;
  mem2[0xFA]=0xFF; 
};

void setup() {
  pinModeFast(BX_OE_n,OUTPUT);
  pinModeFast(EXT_SYNC_OE_n,OUTPUT);
  digitalWriteFast(BX_OE_n,1);
  digitalWriteFast(EXT_SYNC_OE_n,1);

  init_mem();

  pinModeFast(SLOT_ID,INPUT);
  pinModeFast(MISO, OUTPUT);
  
  // set SPI in slave mode
  SPCR |= _BV(SPE);

  // enable SPI interrupts
  SPCR |= _BV(SPIE);

  // Clock is high in idle
  SPCR |= _BV(CPOL);
  SPCR |= _BV(CPHA);

  // initialize SPI Data Register
  SPDR = 0x55;
}

void checkState(byte c) {
  switch(c) {
    case READ_MEM:
      currentState = GET_MEM_ADDR;
    break;
    case READ_MEM2:
      currentState = GET_MEM_ADDR2;
    break;
    case 0x0:
      currentState = SET_STATE;
    break;
  }
}

void select_ext_sync_on() {
      digitalWriteFast(EXT_SYNC_OE_n,0);
      digitalWriteFast(BX_OE_n,0);
}

void select_ext_sync_off() {
      digitalWriteFast(EXT_SYNC_OE_n,1);
      digitalWriteFast(BX_OE_n,0);
}

void deselect() {
      digitalWriteFast(EXT_SYNC_OE_n,1);
      digitalWriteFast(BX_OE_n,1);
}

void deselect2() {
      digitalWriteFast(EXT_SYNC_OE_n,1);
      digitalWriteFast(BX_OE_n,1);
}

// Interrupt routine
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // get byte sent from master
  switch(currentState) {
    case IDLE:
      checkState(c);
    break;
    case SET_STATE:
      if(0x0 == c) {
        currentState = PROCESS_SET_STATE;
      }
    break;
    case PROCESS_SET_STATE:
      switch(c) {
          case DESELECT:
            deselect();
          break;
          case DESELECT2:
            deselect2();
          break;
          case SEL_EXT_ON:
            select_ext_sync_on();
          break;
          case SEL_EXT_OFF:
            select_ext_sync_off();
          break;
      }
      currentState = IDLE;
    break;
    case GET_MEM_ADDR:
      SPDR = mem[c];
      currentState = PROCESS_MEM_REQ;
    break;
    case GET_MEM_ADDR2:
      SPDR = mem2[c];
      currentState = PROCESS_MEM2_REQ;
    break;
    case PROCESS_MEM_REQ:
    case PROCESS_MEM2_REQ:
      currentState = IDLE;
    break;
    default:
      SPDR = 0x0;
      currentState = IDLE;
    break;
  }
}

void loop() {
  if(currentState != IDLE) {
    switch(currentState) {
      case PROCESS_MEM_REQ:
      case PROCESS_MEM2_REQ:
      case PROCESS_SET_STATE:
        if(0 == digitalReadFast(SLOT_ID)) {
          pinModeFast(SLOT_ID,OUTPUT);
          digitalWriteFast(SLOT_ID,0);
          activeSlotID = true;
        }
        break;
    }
  } else if(activeSlotID) {
    digitalWriteFast(SLOT_ID,1);
    pinModeFast(SLOT_ID,INPUT);
    activeSlotID = false;
  }
}
