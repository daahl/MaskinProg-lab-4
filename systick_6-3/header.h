#ifndef HEADER_H
#define HEADER_H

/* ---- Delay prototypes---- */
void init_app();
void systick_irq_handler(void);
void delay_1mikro(void);
void delay(unsigned int);

#endif