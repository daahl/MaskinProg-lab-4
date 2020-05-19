/*
 * 	startup.c
 *
 */

__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");		/* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");					/* call main */
__asm__ volatile(".L1: B .L1\n");				/* never return */
}

__attribute__((naked)) setControlReg(unsigned int x)
{
	__asm(
	"	MSR CONTROL,R0\n"
	"	BX	LR\n"
	);
}
 
/* ---- Registers - SYSCFG ---- */
#define SYSCFG          	0x40013800
#define SYSCFG_EXTICR1      ((volatile unsigned short*) (SYSCFG + 8))
 
/* ---- Registers - GPIO ---- */
#define GPIO_D				0x40020C00
#define GPIO_D_MODER    	((volatile unsigned int*)   GPIO_D)         // I/O config
#define GPIO_D_ODR_LOW  	((volatile unsigned char*)  (GPIO_D+0x14))  // The LOW values in the ODR, D0-7

#define GPIO_E				0x40021000
#define GPIO_E_ODR_LOW		((volatile unsigned char*)  (GPIO_E+0x14)) // Low E ODR
#define GPIO_E_IDR_LOW		((volatile unsigned char*)  (GPIO_E+0x10)) // Low E IDR

/* ---- Registers - EXTI---- */
#define EXTI              0x40013C00
#define EXTI_IMR          ((volatile unsigned short*) (EXTI))
#define EXTI_RTSR         ((volatile unsigned short*) (EXTI + 8))
#define EXTI_FTSR         ((volatile unsigned short*) (EXTI + 0xC))
#define EXTI_PR           ((volatile unsigned short*) (EXTI + 0x14))

/* ---- Registers and definitions - Other ---- */
#define NVIC		((unsigned int *) 0xE000E100)
#define SCB_VTOR	0x2001C000

/* ---- Global variables---- */
static volatile int IRQ_count; // For the display count
static volatile int GPIO_E_IN;

void irq_handler(void){
	GPIO_E_IN = *GPIO_E_IDR_LOW; 
	
	if(*EXTI_PR & (1<<3)){ // IF IRQ on EXTI3, (1<<3)
		*EXTI_PR |= (1<<3); // Reset IRQ
		
		switch(GPIO_E_IN){ // 9 = IRQ0, 10 = IRQ1, 12 = IRQ2
			case 9: *GPIO_E_ODR_LOW &= (1<<5); // Reset IRQ0
					*GPIO_E_ODR_LOW &= ~(1<<5);
					IRQ_count++;
					break;
			case 10: *GPIO_E_ODR_LOW &= (1<<6); // Reset IRQ0
					 *GPIO_E_ODR_LOW &= ~(1<<6);
					 IRQ_count = 0;
					 break;
			case 12: *GPIO_E_ODR_LOW &= (1<<5); // Reset IRQ0
					 *GPIO_E_ODR_LOW &= ~(1<<5);
					 if(IRQ_count){
						 IRQ_count = 0; // Turn off disp
					 }else{
						 IRQ_count = 255; // Turn on disp
					 }
					 break;
			default: 
					break;
		}
	}
}

void app_init(void){
	*GPIO_D_MODER = 0x5555; // Set low D to output for the display
	
	*SYSCFG_EXTICR1 &= ~0xF000; // Zero EXTI3 (highest byte in SYSCFG)
	*SYSCFG_EXTICR1 |= 0x4000; // Connect PE3 to EXTI3 (low GPIO E)
	
	*EXTI_IMR |= (1<<3); // Allow EXTI3 to generate interuptions ...
	*EXTI_RTSR |= (1<<3); // ... on positive flank ...
	*EXTI_FTSR &= ~(1<<3); // ... not on negative flank
	
	*((void (**) (void) ) (SCB_VTOR + 0x64)) = irq_handler; // Assign irq handler with offset (0x64 for EXTI3)
	*NVIC |= (1<<9); // Pass irqs from EXTI3 to the cpu. 9 = EXTI3 in the vector table
}

void main(void)
{
	app_init();
	
	// Update the display forever
	while(1){
		*GPIO_D_ODR_LOW = IRQ_count;
	}		
}

