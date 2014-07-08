#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <linux/input.h>

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
#define N_ROW_PINS 8

struct {
  int pin; // 0-15, 0-7 is GPIOA, 8-15 is GPIOB
  int key;
} io[] = {
//    Input    Output (from /usr/include/linux/input.h)
//{   0,	KEY_A		},
  {   1,	KEY_Y		},//Button B1
  {   2,	KEY_U		},//Button B2
  {   3,	KEY_J		},//Button B3
  {   4,	KEY_H		},//Button B4
  {   5,	KEY_I		},//Button B5
  {   6,	KEY_V		},//Button B6
//{   7,	KEY_H		},//Not used
//{   8,	KEY_I		},//Not used
  {   9,	KEY_K		}, //Joystick gauche
  {  10,	KEY_L		},//Joystick Bas
  {  11,	KEY_M		},//Joystick Droit
  {  12,	KEY_N		},//Joystick Haut
  {  13,	KEY_ENTER	},//Button N1
  {  14,	KEY_T		},//Button N2
  {  15,	KEY_ESC		}//Button Rouge
};
#define IOLEN (sizeof(io) / sizeof(io[0])) // io[] table size

// MCP23017 GPIO 8-bit row context
struct{
  int idx; // row idx, A=0, B=1
  int inmask; // bits mask to identify pin used
  int lastvalue;
  int key_value[N_ROW_PINS]; // -1=unused, 0=low, 1=high
  int key_char[N_ROW_PINS]; // Code caracter 
} mcp[N_MCP_ROWS];

int q2w;

void register_mcp_keys()
{
	int i;
	for (i=0; i<N_MCP_ROWS; i++)
	{
		memset(mcp[i].key_value, -1, sizeof(mcp[i].key_value));
		memset(mcp[i].key_char, -1, sizeof(mcp[i].key_char));
	}
		mcp[i].inmask = 0;

	for(i=0; i<IOLEN; i++)
	{
		// printf("Configuring pin %d\n", io[i].pin);
		int idx = (floor(io[i].pin/N_ROW_PINS));
		int pin = io[i].pin % N_ROW_PINS;
		mcp[idx].inmask |= (1<<pin);
		mcp[idx].key_value[pin] = 1; // default is high
		mcp[idx].key_char[pin] = io[i].key;
	}

	for (i=0; i<N_MCP_ROWS; i++)
	{
		mcp[i].lastvalue = mcp[i].inmask;
	}

	// printf("Row A=%d, Row B=%d\n", mcp[0].inmask, mcp[1].inmask);
}

void init_Key_value()
{
	int val[N_MCP_ROWS], i;
	val[0] = wiringPiI2CReadReg8 (q2w, GPIOA);
	val[1] = wiringPiI2CReadReg8 (q2w, GPIOB);
	//printf ("mcpA : %d ; mcpB : %d\n", val[0], val[1]);
	fflush (stdout) ;
	for (i=0; i<N_MCP_ROWS; i++)
	{
		mcp[i].lastvalue = val[i];
		for(i=0; i<IOLEN; i++)
		{
			int idx = (floor(io[i].pin/N_ROW_PINS));
			int pin = io[i].pin % N_ROW_PINS;
			mcp[idx].key_value[pin] = ((mcp[idx].lastvalue & (1<<pin)) >> pin);
		}
	}
}

void mcp_interrupt_handler ()
{
	int val[N_MCP_ROWS], ival[N_MCP_ROWS], xval[N_MCP_ROWS], i, j, x, f;

	// read values
	val[0] = wiringPiI2CReadReg8 (q2w, GPIOA);
	val[1] = wiringPiI2CReadReg8 (q2w, GPIOB);
	
//	printf ("mcpA : %d ; mcpB : %d\r\n", val[0], val[1]);
//	fflush (stdout) ;

	// collect events
	for (i=0; i<N_MCP_ROWS; i++)
	{
	
		ival[i] = val[i] & mcp[i].inmask; // current value
		xval[i] = ival[i] ^ mcp[i].lastvalue; // changes; a bit=1 if value changed
		
		//printf("%d %d\n", ival[i], registerInit[i]);
		if (xval[i] != 0) // if at least one pin changes value
		{
			//printf("%d %d\n", ival[i], registerInit[i]);
			
			for (j=0; j<N_ROW_PINS; j++)
			{
				if (mcp[i].key_value[j] > -1)
				{// pin is enabled
					
					if (( ((ival[i] & (1 << j)) >> j) ^ mcp[i].key_value[j]))
					{ // pin have toggle
						sendKey(mcp[i].key_char[j], 1); //we turn on the key
						//printf ("cote %d, pin %d pressed, mcp current value : %d, mcp lastval %d, keyValue : %d, newValue : %d\n", i, j, ival[i], mcp[i].lastvalue, mcp[i].key_value[j], (ival[i] & (1 << j)));
						//fflush (stdout);
					}
					else
					{
						sendKey(mcp[i].key_char[j], 0); //on eteint la touche
						// printf ("cote %d, pin %d relache\n", i, j);
						// fflush (stdout) ;
					}
					// registerInit[i]  = ival[i];
					delayMicroseconds(20000);
				}
			}
			mcp[i].lastvalue = ival[i];
		}
	}
}

int init_i2c()
{
	int gpiofd;
	// Init
	register_mcp_keys();
	if(init_uinput() == 0)
	{
		sleep(1);
	}

	gpiofd = wiringPiSetup();

	if ((q2w = wiringPiI2CSetup (0x20)) == -1)
	{
		fprintf (stderr, "q2w: Unable to initialise I2C: %s\n", strerror (errno));
		return 0;
	}
	// Ensure IOCON is in its reset mode. Specifically, the Bank bit (MSB) should be 0. Otherwise registers table is defined differently.
	wiringPiI2CWriteReg8 (q2w, GPINTENB, 0x00); // If BANK == 1, then IOCON_idx == GPINTENB_idx
	wiringPiI2CWriteReg8 (q2w, IOCON, 0x00);

	// Enable appropriate interrupt pins
	wiringPiI2CWriteReg8 (q2w, GPINTENA, mcp[0].inmask);
	wiringPiI2CWriteReg8 (q2w, GPINTENB, mcp[1].inmask);

	wiringPiI2CWriteReg8 (q2w, INTCONA, mcp[0].inmask);
	wiringPiI2CWriteReg8 (q2w, INTCONB, mcp[1].inmask);

	wiringPiI2CWriteReg8 (q2w, DEFVALA, mcp[0].inmask);
	wiringPiI2CWriteReg8 (q2w, DEFVALB, mcp[1].inmask);

	// Ensure GPIOs are set to 'input'
	wiringPiI2CWriteReg8 (q2w, IODIRA, mcp[0].inmask);
	wiringPiI2CWriteReg8 (q2w, IODIRB, mcp[1].inmask);
	// Enable pull-up resistors
	wiringPiI2CWriteReg8 (q2w, GPPUA, mcp[0].inmask);
	wiringPiI2CWriteReg8 (q2w, GPPUB, mcp[1].inmask);
	// Don't reverse polarity
	wiringPiI2CWriteReg8 (q2w, IPOLA, 0x00);
	wiringPiI2CWriteReg8 (q2w, IPOLB, 0x00);
	// Init GPIOs to read 'low'
	wiringPiI2CWriteReg8 (q2w, GPIOA, 0x00);
	wiringPiI2CWriteReg8 (q2w, GPIOB, 0x00);

	wiringPiI2CReadReg8 (q2w, GPIOA);
	wiringPiI2CReadReg8 (q2w, GPIOB);

	init_Key_value();
	/* if(wiringPiISR(BUTTON_PIN, INT_EDGE_BOTH, &mcp_interrupt_handler) < 0 ) //FUCKING FUNCTION WAS NOT ABLE TO WORK ><
	{
		fprintf(stderr, "Could not setup ISR: %s\n", strerror(errno));
		return 1;
	} */
	return 1;
}

int main (int argc, char *argv [])
{
	if (init_i2c() == 0) return 1;
	
	mcp_interrupt_handler();
	for (;;)
	{
		delayMicroseconds(20000);
		mcp_interrupt_handler();
	}

	return 0;
}
