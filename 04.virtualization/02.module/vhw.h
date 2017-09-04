#ifndef _VHW_H_
#define _VHW_H_

#include "vhw_def.h"

/*
 * @bref virtual hardware set UDP data
 *
 * @param buffer data point
 * @param n data size
 * 
 * @return the result
 *       0 : OK
 *   other : fail
 */
int vhw_send_data(const void *buffer, int n);

/*
 * @bref virtual hardware set gpio state
 *
 * @param gpio gpio number
 * @param set gpio state
 * 
 * @return the result
 *       0 : OK
 *   other : fail
 */
int vhw_set_gpio(int gpio, bool set);

/*
 * @bref virtual hardware register a IRQ
 *
 * @param id id number
 * @param func IRQ callback function
 * @arg 
 * 
 * @return the result
 *       0 : OK
 *   other : fail
 */
int vhw_register_irq(int id, void (*func)(int id, int val, void *arg), void *arg);

/*
 * @bref virtual hardware unregister a IRQ
 *
 * @param id id number
 * 
 * @return none
 */
void vhw_unregister_irq(int id);

#endif /* _VHW_H_ */
