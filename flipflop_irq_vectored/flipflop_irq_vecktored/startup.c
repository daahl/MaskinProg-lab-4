/*
 *  startup.c
 *
 */
 #include <stdio.h>

 
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
 

 /* ---- Registers - SYSCFG ---- */
#define SYSCFG              0x40013800
#define SYSCFG_PMC          ((volatile unsigned short*) (SYSCFG + 4))
#define SYSCFG_EXTICR1      ((volatile unsigned short*) (SYSCFG + 8))
#define SYSCFG_EXTICR2      ((volatile unsigned short*) (SYSCFG + 0xC))
#define SYSCFG_EXTICR3      ((volatile unsigned short*) (SYSCFG + 0x10))
#define SYSCFG_EXTICR4      ((volatile unsigned short*) (SYSCFG + 0x14))
#define SYSCFG_CMPCR        ((volatile unsigned short*) (SYSCFG + 0x20))
 
  /* ---- Registers - EXTI ---- */
#define EXTI                0x40013C00
#define EXTI_IMR            ((volatile unsigned short*) (EXTI))
#define EXTI_EMR            ((volatile unsigned short*) (EXTI + 4))
#define EXTI_RTSR           ((volatile unsigned short*) (EXTI + 8))
#define EXTI_FTSR           ((volatile unsigned short*) (EXTI + 0xC))
#define EXTI_SWIER          ((volatile unsigned short*) (EXTI + 0x10))
#define EXTI_PR             ((volatile unsigned short*) (EXTI + 0x14))
#define EXTI3_IRQVEC		((volatile unsigned int *) (SCB_VTOR+0x64))
#define EXTI2_IRQVEC		((volatile unsigned int *) (SCB_VTOR+0x60))
#define EXTI1_IRQVEC		((volatile unsigned int *) (SCB_VTOR+0x5C))
#define EXTI0_IRQVEC		((volatile unsigned int *) (SCB_VTOR+0x58))
#define EXTI3_IRQ_BPOS		(1<<3)
#define EXTI2_IRQ_BPOS		(1<<2)
#define EXTI1_IRQ_BPOS		(1<<1)
#define EXTI0_IRQ_BPOS		(1<<0)

  /* ---- Registers - random stuff ---- */
#define SCB_VTOR			0x2001C000
#define RST0				0x10
#define RST1				0x20
#define RST2				0x40

  /* ---- Registers - NVIC ---- */
#define NVIC				0xE000E100
#define NVIC_ISER0			((volatile unsigned int *) (NVIC))	
#define NVIC_IABR0 			((volatile unsigned int *) (NVIC + 0x200))
#define NVIC_EXTI3_IRQ_BPOS	(1<<9)
#define NVIC_EXTI2_IRQ_BPOS	(1<<8)
#define NVIC_EXTI1_IRQ_BPOS	(1<<7)
#define NVIC_EXTI0_IRQ_BPOS	(1<<6)
 
/* ---- Registers - GPIO ---- */
#define GPIO_E				0x40021000
#define GPIO_D              0x40020C00
#define GPIO_D_IDR          ((volatile unsigned short*) (GPIO_D+0x10))  // The entire IDR
#define GPIO_D_IDR_HIGH     ((volatile unsigned char*)  (GPIO_D+0x11))  // The HIGH values un the IDR
#define GPIO_D_ODR_LOW      ((volatile unsigned char*)  (GPIO_D+0x14))  // The LOW values in the ODR, D0-7
#define GPIO_E_ODR_LOW		((volatile unsigned char *) (GPIO_E+0x14))
#define GPIO_E_IDR_LOW  	((volatile unsigned char *) (GPIO_E+0x10))

#define SIMULATOR

#ifdef SIMULATOR
#define DELAY_COUNT 1000
#else
#define DELAY_COUNT 1000000
#endif

/* ---- Global variables ---- */
static volatile int IRQ_count;

void irq_handler0(void){
			if(*EXTI_PR & EXTI0_IRQ_BPOS){
		IRQ_count++;
		if(*EXTI_PR & EXTI1_IRQ_BPOS)
            *( (unsigned int *) EXTI_PR) |= EXTI0_IRQ_BPOS; 
			}
}
void irq_handler1(void){
	if(*EXTI_PR & EXTI1_IRQ_BPOS){
        IRQ_count = 0;
		if(*EXTI_PR & EXTI1_IRQ_BPOS)
            *((unsigned int *) EXTI_PR) |= EXTI1_IRQ_BPOS; 
    }
}
void irq_handler2(void){
	    if(*EXTI_PR & EXTI2_IRQ_BPOS){
			if(IRQ_count ==0){
				IRQ_count = 255;
			}else{
				IRQ_count = 0;
        }
		if(*EXTI_PR & EXTI2_IRQ_BPOS)
            *( (unsigned int *) EXTI_PR) |= EXTI2_IRQ_BPOS; 
    }
}
 
 void init_app(void){
	*((unsigned short *) GPIO_D) = 0x5555;
	*((unsigned int *) SYSCFG_EXTICR1) &= ~0x00FF;
	*((unsigned int *) SYSCFG_EXTICR1) |= 0x0444;
	*((unsigned int *) EXTI_IMR) |= EXTI2_IRQ_BPOS + EXTI1_IRQ_BPOS + EXTI0_IRQ_BPOS;
	*((unsigned int *) EXTI_RTSR) |= EXTI2_IRQ_BPOS +EXTI1_IRQ_BPOS + EXTI0_IRQ_BPOS; 
	*((unsigned int *) EXTI_FTSR) &= ~(EXTI2_IRQ_BPOS +EXTI1_IRQ_BPOS + EXTI0_IRQ_BPOS); 
    *((void (**) (void) ) (EXTI2_IRQVEC)) = irq_handler2;
	*((void (**) (void) ) (EXTI1_IRQVEC)) = irq_handler1;
	*((void (**) (void) ) (EXTI0_IRQVEC)) = irq_handler0;
	*((unsigned int *) NVIC) |= NVIC_EXTI2_IRQ_BPOS + NVIC_EXTI1_IRQ_BPOS + NVIC_EXTI0_IRQ_BPOS; 
}

void main(void){
    init_app();
    while(1)
        *GPIO_D_ODR_LOW = IRQ_count;
}