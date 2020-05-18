/*
 *  startup.c
 *
 */
 #include <stdio.h>
 #include "header.h"
 
 __attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");      /* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");                 /* call main */
__asm__ volatile(".L1: B .L1\n");               /* never return */
}

__attribute__((naked)) setControlReg(unsigned int x)
{
	__asm(
	"	MSR CONTROL,R0\n"
	"	BX	LR\n"
	);
}
 
 /* ---- Registers - SysTick ---- */
#define STK          0xE000E010
#define STK_CTRL     ((volatile unsigned int*) STK)         // (De)activation and status of SysTick
#define STK_CTRL_END ((volatile unsigned char*) (STK + 2))  // (De)activation and status of SysTick
#define STK_LOAD     ((volatile unsigned int*) (STK + 4))   // Start value
#define STK_VAL      ((volatile unsigned int*) (STK + 8))   // Current value
 
/* ---- Registers - GPIO ---- */
#define GPIO_D          0x40020C00
#define GPIO_D_IDR      ((volatile unsigned short*) (GPIO_D+0x10))  // The entire IDR
#define GPIO_D_IDR_HIGH ((volatile unsigned char*)  (GPIO_D+0x11))  // The HIGH values un the IDR
#define GPIO_D_ODR      ((volatile unsigned short*) (GPIO_D+0x14))  // The entire ODR
#define GPIO_D_ODR_LOW  ((volatile unsigned char*)  (GPIO_D+0x14))  // The LOW values in the ODR, D0-7
#define GPIO_D_ODR_HIGH ((volatile unsigned char*)  (GPIO_D+0x15))  // The HIGH values in the ODR, D8-15
#define GPIO_D_MODER    ((volatile unsigned int*)   GPIO_D)         // I/O config
#define GPIO_D_OTYPER   ((volatile unsigned char*)  (GPIO_D+5))     // Push-pull/open-drain config
#define GPIO_D_PUPDR    ((volatile unsigned short*) (GPIO_D+0x0E))  // Pull-up/pull-down config
 
/*- --- Registers - Other ---- */
#define SIMULATOR

#ifdef SIMULATOR
#define DELAY_COUNT 1000
#else
#define DELAY_COUNT 1000000
#endif

/* ---- Global variables ---- */
static volatile int systick_flag;
static volatile int delay_count;
 
 void init_app(void){
    *GPIO_D_MODER = 0x55555555; // Pin D15-0 outports
	
	*((void (**)(void) ) 0x2001C03C ) = systick_irq_handler; // Assign custom interupt handler
}
 
void systick_irq_handler(void){
    *STK_CTRL = 0;	// bit0 = 0 deactivates the counter
    delay_count--;	
	
    if(delay_count > 0){ // Run the micro delay more times if requested
        delay_1mikro();
    }
    else{
        systick_flag = 1; // 1 = delay is done
    }
}
 
void delay_1mikro(void){
    *STK_CTRL = 0; 			// bit0 = 0 deactivates the counter
    *STK_LOAD = (168 - 1);	// N = 168 cycles requires N-1 to be sent to STK_LOAD
    *STK_VAL = 0;			// Reset count register
    *STK_CTRL = 7;			// bit0 = 1, activate counter; bit1 = 1, generate interupt request; bit2 = 1, use regular systemclock time
}
 
void delay(unsigned int count){
    if(count == 0){
        return;
    }
	
    delay_count = count;
    systick_flag = 0; // 0 means a delay is active
    delay_1mikro();
}

void main(void){
    init_app();

	delay(DELAY_COUNT); // Use globally defined delay value
	*GPIO_D_ODR_LOW = 0xFF; // Low dioderamp is on while delay is active
	unsigned int diode_light = 1; // To be used with High dioderamp
	
	while(1){
		if(systick_flag){ // systick_flag = 1 when delay is finished
			break;
		}
		
		*GPIO_D_ODR_HIGH = diode_light++; // Assign first, then increase value of diode_light
	}
	
	*GPIO_D_ODR_LOW = 0; // Low dioderamp is off when delay is finished
}