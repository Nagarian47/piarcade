#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

// MCP23S17 Registers

#define IOCON           0x0A

#define IODIRA          0x00
#define IPOLA           0x02
#define GPINTENA        0x04
#define DEFVALA         0x06
#define INTCONA         0x08
#define GPPUA           0x0C
#define INTFA           0x0E
#define INTCAPA         0x10
#define GPIOA           0x12
#define OLATA           0x14

#define IODIRB          0x01
#define IPOLB           0x03
#define GPINTENB        0x05
#define DEFVALB         0x07
#define INTCONB         0x09
#define GPPUB           0x0D
#define INTFB           0x0F
#define INTCAPB         0x11
#define GPIOB           0x13
#define OLATB           0x15

// Bits in the IOCON register

#define IOCON_BANK_MODE 0x80
#define IOCON_MIRROR    0x40
#define IOCON_SEQOP     0x20
#define IOCON_DISSLW    0x10
#define IOCON_HAEN      0x08
#define IOCON_ODR       0x04
#define IOCON_INTPOL    0x02
#define IOCON_UNUSED    0x01

// Default initialisation mode

#define IOCON_INIT      (IOCON_SEQOP)

#define BUTTON_PIN 0
#define N_MCP_ROWS 2

struct {
  int pin; // 0-15, 0-7 is GPIOA, 8-15 is GPIOB
  int key;
} io[] = {
//    Input    Output (from /usr/include/linux/input.h)
  { 0,      KEY_LEFT     },
  {  1,      KEY_RIGHT    }
};
#define IOLEN (sizeof(io) / sizeof(io[0])) // io[] table size

// MCP23017 GPIO 8-bit row context
typedef struct{
  int idx; // row idx, A=0, B=1
  int inmask;
  int lastvalue;
  int *key_value[8]; // 0 or 1
} mcp_row;

mcp_row mcp[N_MCP_ROWS];

int q2w;

void register_mcp_keys() {
  for (i=0; i<N_MCP_ROWS; i++) {
    mcp[i].inmask = 0;
  }

  for(i=0; i<IOLEN; i++) {
    printf("Configuring pin %d\n", io[i].pin);
    int idx = (floor(io[i].pin));
    mcp[idx].inmask |= 1<<(io[i].pin % 8);
  }

  printf("Row A=%d, Row B=%d", mcp[0].inmask, mcp[0].inmask);
}

void myInterrupt5 (void) { 
struct wiringPiNodeStruct *myNode ;

//if((myNode = wiringPiFindNode (BUTTON_PIN)) == 0) {
//printf("wiringPiFindNode failed\n\r");
//exit(0);
//}
printf("I've been hit!!\n\r");
//wiringPiI2CReadReg8(q2w, INTCAPA);
while (wiringPiI2CReadReg8 (q2w, GPIOA ) != 0x01) {
//	printf("%d\n", wiringPiI2CReadReg8 (q2w, GPIOA ));
}
}

int main (int argc, char *argv [])
{
  int gpiofd;
  // Init
  register_mcp_keys();
  gpiofd = wiringPiSetup();

  if ((q2w = wiringPiI2CSetup (0x20)) == -1)
    { fprintf (stderr, "q2w: Unable to initialise I2C: %s\n", strerror (errno)) ; return 1 ; }

  // Ensure IOCON is in its reset mode. Specifically, the Bank bit (MSB) should be 0. Otherwise registers table is defined differently.
  wiringPiI2CWriteReg8 (q2w, GPINTENB, 0x00); // If BANK == 1, then IOCON_idx == GPINTENB_idx
  wiringPiI2CWriteReg8 (q2w, IOCON, 0x00);

  // Turn off (optional: enable them) interrupt pins
  wiringPiI2CWriteReg8 (q2w, GPINTENA, 0x03);
  wiringPiI2CWriteReg8 (q2w, GPINTENB, 0x00);

  wiringPiI2CWriteReg8 (q2w, INTCONA, 0x03);
  wiringPiI2CWriteReg8 (q2w, DEFVALA, 0x03);

  // Ensure GPIOs are set to 'input'
  wiringPiI2CWriteReg8 (q2w, IODIRA, 0x03);
  wiringPiI2CWriteReg8 (q2w, IODIRB, 0x00);
  // Enable pull-up resistors
  wiringPiI2CWriteReg8 (q2w, GPPUA, 0x03);
  wiringPiI2CWriteReg8 (q2w, GPPUB, 0x00);
  // Don't reverse polarity
  wiringPiI2CWriteReg8 (q2w, IPOLA, 0x00);
  wiringPiI2CWriteReg8 (q2w, IPOLB, 0x00);
  // Init GPIOs to read 'low'
  wiringPiI2CWriteReg8 (q2w, GPIOA, 0x00);
  wiringPiI2CWriteReg8 (q2w, GPIOB, 0x00);

  if(wiringPiISR(BUTTON_PIN, INT_EDGE_FALLING, &myInterrupt5) < 0 )
  {
    fprintf(stderr, "Could not setup ISR: %s\n", strerror(errno));
    return 1;
  }

  for (;;)
  {
//    printf ("Waiting ... ") ; fflush (stdout) ;
	}
  // Poll
/*
  int byte_a;
  int byte_b;
  for (;;)
  {
    byte_a = wiringPiI2CReadReg8 (q2w, GPIOA);
    byte_b = wiringPiI2CReadReg8 (q2w, GPIOA);
    printf("a=%d, b=%d\n", byte_a, byte_b);
    delay (500);
  }
  */
  return 0 ;
}