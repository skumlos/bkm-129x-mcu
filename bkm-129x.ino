#include <SPI.h>

#define BX_OE_n (PD6) // Output enable (active low)
#define EXT_SYNC_OE_n (PD7) // External sync enable (active low)
#define SLOT_ID (PD2)

enum Command {
  READ_MEM    = 0x9E,
  READ_MEM2   = 0xBE,
  SEL_EXT_ON  = 0xF4,
  SEL_EXT_OFF = 0xF6,
  DESELECT    = 0xF7  
};

enum State {
  IDLE              = 0x00,
  GET_MEM_ADDR      = 0x01,
  GET_MEM_ADDR2     = 0x02,
  PROCESS_MEM_REQ   = 0x03,
  PROCESS_MEM2_REQ  = 0x04
};

bool process = false;
byte readAddr = 0x0;

byte currentState = IDLE;
byte mem[256];
byte mem2[256];

// Like read during powerup
void init_mem() {
  mem[0x00]=0xC8;
  mem[0x02]=0x32;
  mem[0x03]=0x30;
  mem[0x04]=0x30;
  mem[0x05]=0x31;
  mem[0x06]=0x39;
  mem[0x07]=0x31;
  mem[0x08]=0x31;
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
  init_mem();
  pinMode(SLOT_ID,INPUT);
  pinMode(BX_OE_n,OUTPUT);
  pinMode(EXT_SYNC_OE_n,OUTPUT);
  pinMode(MISO, OUTPUT);
  
  digitalWrite(BX_OE_n,1);
  digitalWrite(EXT_SYNC_OE_n,1);

  delay(1000);
  
  // set SPI in slave mode
  SPCR |= _BV(SPE);

  // enable SPI interrupts
  SPCR |= _BV(SPIE);

  // Clock is high in idle
  SPCR |= _BV(CPOL);
  SPCR |= _BV(CPHA);

  // initialize SPI Data Register
  SPDR = 0x55;

  Serial.begin (115200);   // debugging
  Serial.print(SPCR,BIN);
  Serial.print('\n');
}

// Interrupt routine
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // get byte sent from master
//  Serial.print(c,HEX);
  switch(currentState) {
    case IDLE:
      switch(c) {
        case READ_MEM:
          SPDR = c;
          currentState = GET_MEM_ADDR;
        break;
        case READ_MEM2:
          SPDR = c;
          currentState = GET_MEM_ADDR2;
        break;
        case DESELECT:
        case SEL_EXT_ON:
        case SEL_EXT_OFF:
          SPDR = c;
          currentState = c;
        break;
      }
    break;
    case DESELECT:
      digitalWrite(EXT_SYNC_OE_n,1);
      digitalWrite(BX_OE_n,1);
      currentState = IDLE;
    break;
    case SEL_EXT_ON:
      digitalWrite(EXT_SYNC_OE_n,0);
      digitalWrite(BX_OE_n,0);
      currentState = IDLE;
    break;
    case SEL_EXT_OFF:
      digitalWrite(EXT_SYNC_OE_n,1);
      digitalWrite(BX_OE_n,0);
      currentState = IDLE;
    break;
    case GET_MEM_ADDR:
      //readAddr = c;
      SPDR = mem[c];
      currentState = PROCESS_MEM_REQ;
    break;
    case GET_MEM_ADDR2:
      //readAddr = c;
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

void req() {
    pinMode(SLOT_ID,OUTPUT);
    digitalWrite(SLOT_ID,0);
    while(currentState != IDLE) delay(1);
    digitalWrite(SLOT_ID,1);
    pinMode(SLOT_ID,INPUT);
}

void loop() {
  if(currentState != IDLE) {
    switch(currentState) {
      case PROCESS_MEM_REQ:
        while(digitalRead(SLOT_ID) != 1) delay(1);
//        SPDR = mem[readAddr];
        req();
      break;
      case PROCESS_MEM2_REQ:
        while(digitalRead(SLOT_ID) != 1) delay(1);
  //      SPDR = mem2[readAddr];
        req();
      break;
    }
  }
}
