/**

  @file    mutex/common/osal_interrupt_list.h
  @brief   Maintain list of interrupts that these can be disabled.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.4.2020

  On FreeRTOS/ESP32 we need to be able to disable interrupts when writing data to flash.
  To be able to do this for used libraries, we maintain list of interrupts which need
  to be disabled or enabled.

  Define OSAL_INTERRUPT_LIST_SUPPORT controls compiler's code generation for supporting interrupt
  lists. At this time interrupt lists are needed only for ESP32.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#if OSAL_INTERRUPT_LIST_SUPPORT

/* Callback function type to enable or disable an interrupt.
 */
typedef void osal_control_interrupt_func(os_boolean enable, void *context);

typedef struct osalInterruptInfo
{
    osal_control_interrupt_func *func;
    void *context;
    struct osalInterruptInfo *next, *prev;
}
osalInterruptInfo;


/* Add pointer to interrupt control function to enable or disable interrupt to list.
 */
os_boolean osal_add_interrupt_to_list(
    osal_control_interrupt_func *func,
    void *context);

/* Remove pointer to interrupt control function from list.
 */
void osal_remove_interrupt_to_list(
    osal_control_interrupt_func *func,
    void *context);

/* Enable or disable interrupts.
 */
void osal_control_interrupts(
    os_boolean enable);

#endif
