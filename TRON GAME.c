#include <unistd.h>
#include <stdio.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
/*******************************************************************************
 * This file provides address values that exist in the DE10-Lite Computer
 * This file also works for DE1-SoC, except change #define DE10LITE to 0
 ******************************************************************************/

#ifndef __SYSTEM_INFO__
#define __SYSTEM_INFO__

#define DE10LITE 0 // change to 0 for CPUlator or DE1-SoC, 1 for DE10-Lite

/* do not change anything after this line */

#if DE10LITE
 #define BOARD				"DE10-Lite"
 #define MAX_X		160
 #define MAX_Y		120
 #define YSHIFT		  8
#else
 #define MAX_X		320
 #define MAX_Y		240
 #define YSHIFT		  9
#endif


/* Memory */
#define SDRAM_BASE			0x00000000
#define SDRAM_END			0x03FFFFFF
#define FPGA_PIXEL_BUF_BASE		0x08000000
#define FPGA_PIXEL_BUF_END		0x0800FFFF
#define FPGA_CHAR_BASE			0x09000000
#define FPGA_CHAR_END			0x09001FFF

/* Devices */
#define LED_BASE			0xFF200000
#define LEDR_BASE			0xFF200000
#define HEX3_HEX0_BASE			0xFF200020
#define HEX5_HEX4_BASE			0xFF200030
#define SW_BASE				0xFF200040
#define KEY_BASE			0xFF200050
#define JP1_BASE			0xFF200060
#define ARDUINO_GPIO			0xFF200100
#define ARDUINO_RESET_N			0xFF200110
#define JTAG_UART_BASE			0xFF201000
#define TIMER_BASE			0xFF202000
#define TIMER_2_BASE			0xFF202020
#define MTIMER_BASE			0xFF202100
#define RGB_RESAMPLER_BASE    		0xFF203010
#define PIXEL_BUF_CTRL_BASE		0xFF203020
#define CHAR_BUF_CTRL_BASE		0xFF203030
#define ADC_BASE			0xFF204000
#define ACCELEROMETER_BASE		0xFF204020

/* Nios V memory-mapped registers */
#define MTIME_BASE             		0xFF202100

#endif
	
typedef uint16_t pixel_t;

volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

int playerPoints = 0;
int botPoints = 0;

int x = MAX_X/3;
int y = MAX_Y/2;

int dx = 1;
int dy = 0;
int left = 1;
int right = 3;

int direction = 4; // 1=up, 2=left, 3=down, 4=right

int playerdirection = 2;
int i = MAX_X*2/3;
int j = MAX_Y/2;
int di = -1;
int dj = 0;
int l = 3;
int r = 1;

int SEG7[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

volatile int *HEX = (volatile int *)HEX3_HEX0_BASE;
volatile int *KEY = (volatile int *)KEY_BASE;
volatile int *SW = (volatile int *)SW_BASE;
volatile int pending_turn = 0;

int key0_now  = 0;    // current state of KEY0
int key1_now  = 0;    // current state of KEY1
int key0_last = 0;    // previous state of KEY0
int key1_last = 0;
int counter = 0;

uint64_t PERIOD = (uint64_t)1000000000;// 100M
int timeMultiplier = 1;

//==================================================================================
static void handler(void) __attribute__ ((interrupt ("machine")));

void enable_key_interrupts() {
    *(KEY + 2) = 0x3;   // enable IRQ for KEY0 and KEY1
    *(KEY + 3) = 0x3;   // clear any pending edges
}

void set_mtimer( volatile uint32_t *time_ptr, uint64_t new_time64 )
{
	*(time_ptr+0) = (uint32_t)0;
	 // prevent hi from increasing before setting lo
	*(time_ptr+1) = (uint32_t)(new_time64>>32);
	// set hi part
	*(time_ptr+0) = (uint32_t)new_time64;
	// set lo part
}

uint64_t get_mtimer( volatile uint32_t *time_ptr)
{
	uint32_t mtime_h, mtime_l;
	// can only read 32b at a time
	// hi part may increment between reading hi and lo
	// if it increments, re-read lo and hi again
	do {
		mtime_h = *(time_ptr+1); // read mtime-hi
		mtime_l = *(time_ptr+0); // read mtime-lo
	} while( mtime_h != *(time_ptr+1) );
	// if mtime-hi has changed, repeat
	// return 64b result
	return ((uint64_t)mtime_h << 32) | mtime_l ;
}

volatile uint32_t *mtime_ptr = (uint32_t *) MTIMER_BASE;

void setup_mtimecmp() {
	uint64_t mtime64 = get_mtimer( mtime_ptr );
	// read current mtime (counter)
	mtime64 = (mtime64/PERIOD+1) * PERIOD;
	// compute end of next time PERIOD
	set_mtimer( mtime_ptr+2, mtime64 );
	// write first mtimecmp ("+2" == mtimecmp)
}

void setup_cpu_irqs( uint32_t new_mie_value )
{
	 uint32_t mstatus_value, mtvec_value, old_mie_value;
	 mstatus_value = 0b1000; // interrupt bit mask
	 mtvec_value = (uint32_t) &handler; // set trap address
	 __asm__ volatile( "csrc mstatus, %0" :: "r"(mstatus_value) );
	// master irq disable
	 __asm__ volatile( "csrw mtvec, %0" :: "r"(mtvec_value) );
	// sets handler
	 __asm__ volatile( "csrr %0, mie" : "=r"(old_mie_value) );
	 __asm__ volatile( "csrc mie, %0" :: "r"(old_mie_value) );
	 __asm__ volatile( "csrs mie, %0" :: "r"(new_mie_value) );
	// reads old irq mask, removes old irqs, sets new irq mask
	 __asm__ volatile( "csrs mstatus, %0" :: "r"(mstatus_value) );
	// master irq enable
}

//======================================================================================

void delay( int N )
{
	for( int i=0; i<N; i++ ) 
		*pVGA; // read volatile memory location to waste time
}

/* STARTER CODE BELOW. FEEL FREE TO DELETE IT AND START OVER */

void drawPixel( int x, int y, pixel_t colour )
{
	if(y>=MAX_Y || x>=MAX_X || y<0 || x<0 ) return;
	*(pVGA + (y<<YSHIFT) + x ) = colour;
	return;
}

pixel_t getPixel( int x, int y)
{
	if(y>=MAX_Y || x>=MAX_X || y<0 || x<0 ) return wht;
	return *(pVGA + (y<<YSHIFT) + x );
}

pixel_t makePixel( uint8_t r8, uint8_t g8, uint8_t b8 )
{
	// inputs: 8b of each: red, green, blue
	const uint16_t r5 = (r8 & 0xf8)>>3; // keep 5b red
	const uint16_t g6 = (g8 & 0xfc)>>2; // keep 6b green
	const uint16_t b5 = (b8 & 0xf8)>>3; // keep 5b blue
	return (pixel_t)( (r5<<11) | (g6<<5) | b5 );
}

void rect( int x1, int x2, int y1, int y2, pixel_t c )
{
	for( int y=y1; y<y2; y++ )
		for( int x=x1; x<x2; x++ )
			drawPixel( x, y, c );
}

void setUp() {
	rect(0, MAX_X, 0, MAX_Y, wht);
    rect(1, MAX_X-1, 1, MAX_Y-1, blk);
	
	rect(40, MAX_X-40, MAX_Y/3-1, MAX_Y/3+1, wht);
	rect(40, MAX_X-40, MAX_Y*2/3-1, MAX_Y*2/3+1, wht);
	rect(80, MAX_X-80, 1, MAX_Y+1, blk);
	
	//rect(MAX_X/3-5,MAX_X/3+5,MAX_Y/2-5,MAX_Y/2+5,wht);
	//rect(MAX_X/3-4,MAX_X/3+4,MAX_Y/2-4,MAX_Y/2+4,blk);
	//rect(MAX_X*2/3-5,MAX_X*2/3+5,MAX_Y/2-5,MAX_Y/2+5,wht);
	//rect(MAX_X*2/3-4,MAX_X*2/3+4,MAX_Y/2-4,MAX_Y/2+4,blk);


	x = MAX_X/3;
	y = MAX_Y/2;
	dx = 1;
	dy = 0;
	left = 2;
	right = 4;

	direction = 4; // 1=up, 2=left, 3=down, 4=right

	playerdirection = 2;
	i = MAX_X*2/3;
	j = MAX_Y/2;
	di = -1;
	dj = 0;
	l = 3;
	r = 1;

    drawPixel(x, y, red);
	drawPixel(i, j, blu);
	return;
	
	update();
}

int move( int x, int y, pixel_t colour)
{
	if(y>=MAX_Y || x>=MAX_X || y<0 || x<0 || getPixel(x,y)!=blk) {
		return 1;
	}
		/*if(colour == blu) {
			botPoints++;
			printf("bot: %d \n", botPoints);
		}
		if(colour == red) {
			playerPoints++;
			printf("player: %d \n", playerPoints);

		}
		setUp();
		return;
	}*/
	
	else {
		
		*(pVGA + (y<<YSHIFT) + x ) = colour;
		return 0;
	}
}

void end(){
	while(1){
		delay(100000);
	}
}

int main()
{
    printf("start\n");
    setUp();
	
	playerPoints = 0;
    botPoints = 0;

    printf("running\n");
	
	volatile int *LEDR_ptr = (int *) LEDR_BASE;
	setup_mtimecmp();
	enable_key_interrupts();
	setup_cpu_irqs( 0x80 | 1 << 18);// enable mtimer IRQs
	
	int sw0 = 0;
	int sw1 = 0;
	int sw2 = 0;
	int sw3 = 0;
	int sw4 = 0;
	int sw5 = 0;

	
	while (1) {
		*LEDR_ptr = counter;
		sw0 = (*SW & 1) + 1;
		sw1 = ((*SW & 2) >> 1) + 1;
		sw2 = ((*SW & 4) >> 2) + 1;
		sw3 = ((*SW & 8) >> 3) + 1;
		sw4 = ((*SW & 16) >> 4) + 1;
		sw5 = ((*SW & 32) >> 5) + 1;
		
		timeMultiplier = sw0*sw1*sw2*sw3*sw4*sw5;
		PERIOD = (uint64_t)(1000000000/timeMultiplier);// 100M
		//printf("timeMultiplier: %d\n", timeMultiplier);

	}

    return 0;
}

void update(){
	switch(direction){
			case 1:
				dx=0;
				dy=-1;
				left=2;
				right=4;
				break;
			case 2:
				dx=-1;
				dy=0;
				left=3;
				right=1;
				break;
			case 3:
				dx=0;
				dy=1;
				left=4;
				right=2;
				break;
			case 4:
				dx=1;
				dy=0;
				left=1;
				right=3;
				break;
		}
	
	switch(playerdirection){
			case 1:
				di=0;
				dj=-1;
				l=2;
				r=4;
				break;
			case 2:
				di=-1;
				dj=0;
				l=3;
				r=1;
				break;
			case 3:
				di=0;
				dj=1;
				l=4;
				r=2;
				break;
			case 4:
				di=1;
				dj=0;
				l=1;
				r=3;
				break;
		}
	return;
}

/*void keypress_ISR(void)
{
	printf("KEYPRESSED");

	key0_now = (*KEY & 1);
	key1_now = (*KEY & 2) >> 1;

	if (key0_now == 1 && key0_last == 0) {
		playerdirection = r;
		update();
	}
	else if (key1_now == 1 && key1_last == 0) {
		playerdirection = l;
		update();
	}

	key0_last = key0_now;
	key1_last = key1_now;

	update(); // intended side effect
}*/

void mtimer_ISR(void)
{
	uint64_t mtimecmp64 = get_mtimer( mtime_ptr+2 );
	// read mtimecmp
	mtimecmp64 += PERIOD;
	// time of future irq = one period in future
	set_mtimer( mtime_ptr+2, mtimecmp64 );
	// write next mtimecmp
	//counter = counter + 1; // intended side effect
		
	update();
	
		if(playerPoints>=9){
		rect(0, MAX_X, 0, MAX_Y, blu);
		*HEX = (SEG7[9]  << 8)| (SEG7[botPoints]);  
		end();
	}

	if(botPoints>=9){
		rect(0, MAX_X, 0, MAX_Y, red);
		*HEX = (SEG7[playerPoints]  << 8)| (SEG7[9]);  
		end();
	}

	*HEX = (SEG7[playerPoints]  << 8) |   // HEX1
		(SEG7[botPoints]);         // HEX0


	if(getPixel(x+dx, y+dy) != blk) {

		direction = left;
		update();

		if(getPixel(x+dx, y+dy) != blk) {

			direction = right;
			update();
			direction = right;
			update();
		}
	}
        
	x+=dx;
	y+=dy;
	i+=di;
	j+=dj;
	
	if (move(x, y, red)) {
		playerPoints++;
		printf("Bot crashed at (%d,%d)\n", x, y);
		setUp();
	}

	else if (move(i, j, blu)) {
		botPoints++;
		printf("Player crashed at (%d,%d)\n", i, j);
		setUp();
	}
	
	if (pending_turn != 0) {
		if (pending_turn == 1) {           
			playerdirection = l;           
		}
		else if (pending_turn == 2) {      
			playerdirection = r;           
		}
		update();                          
		pending_turn = 0;
	}
}

void key_ISR(void) {
	printf("KEYPRESSED\n");
    uint32_t edge = *(KEY + 3);
    *(KEY + 3) = edge;  // clear interrupt
	

    if (edge & 0x1) {           // KEY0 request RIGHT turn
        if (pending_turn == 2) {
			pending_turn = 0;// cancel if already pending
			printf("right turn cancelled\n");
		}
        else pending_turn = 2;
    }
    if (edge & 0x2) {           // KEY request LEFT turn
        if (pending_turn == 1) {
			pending_turn = 0;
			printf("left turn cancelled\n");
		}

        else pending_turn = 1;
    }
	
}

// this attribute tells compiler to use "mret" rather than "ret"
// at the end of handler() function; also declares its type
void handler(void)
{
	 int mcause_value;
	 // inline assembly, links register %0 to mcause_value
	 __asm__ volatile( "csrr %0, mcause" : "=r"(mcause_value) );
	
	 if (mcause_value == 0x80000007) // machine timer
	 mtimer_ISR();
	 if (mcause_value == 0x80000012) // pushbutton KEY
	 key_ISR();
}