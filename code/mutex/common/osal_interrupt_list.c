/**

  @file    mutex/common/osal_interrupt_list.c
  @brief   Maintain list of interrupts that these can be disabled.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.4.2020

  On FreeRTOS/ESP32 we need to be able to disable interrupts when writing data to flash.
  To be able to do this for used libraries, we maintain list of interrupts which need
  to be disabled or enabled.

  Define OSAL_INTERRUPT_LIST_SUPPORT controls compiler's code generation for supporting interrupt
  lists. At this time interrupt lists are needed only for ESP32.

  Current implementation uses dynamic memory allocation. This is fine with ESP32.
  Static memory allocation with fixed maximum number of interrupts can be implemented,
  and may be better choice for other microcontrollers.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#if OSAL_INTERRUPT_LIST_SUPPORT

/**
****************************************************************************************************

  @brief Add pointer to interrupt control function to enable or disable interrupt to list.
  @anchor osal_add_interrupt_to_list

  The osal_add_interrupt_to_list() function...

  @param   func Pointer to interrupt control function, callback to enable or disable the interrupt.
  @param   context Callback context, can be used for custom purpose.
  @return  OS_TRUE if interrupts are enabled at this moment. This function should be within
           os_lock() and os_unlock() to rely on return value after board initialization.

****************************************************************************************************
*/
os_boolean osal_add_interrupt_to_list(
    osal_control_interrupt_func *func,
    void *context)
{
    osalInterruptInfo *item;
    os_boolean interrupts_enabled;

    item = (osalInterruptInfo*)os_malloc(sizeof(osalInterruptInfo), OS_NULL);
    if (item == OS_NULL) return OS_TRUE;
    os_memclear(item, sizeof(osalInterruptInfo));

    os_lock();

    item->func = func;
    item->context = context;
    item->prev = osal_global->last_listed_interrupt;
    if (osal_global->last_listed_interrupt) {
        osal_global->last_listed_interrupt->next = item;
    }
    else {
        osal_global->first_listed_interrupt = item;
    }
    osal_global->last_listed_interrupt = item;

    interrupts_enabled = (os_boolean)(osal_global->interrupts_disable_count == 0);
    os_unlock();
    return interrupts_enabled;
}


/**
****************************************************************************************************

  @brief Remove pointer to interrupt control function from list.
  @anchor osal_remove_interrupt_to_list

  The osal_remove_interrupt_to_list() function...

  @param   func Pointer to interrupt control function to remove.
  @param   context Callback context, both context and func must match to remove item from list.
  @return  None.

****************************************************************************************************
*/
void osal_remove_interrupt_to_list(
    osal_control_interrupt_func *func,
    void *context)
{
    osalInterruptInfo *item, *next_item;

    os_lock();

    for (item = osal_global->first_listed_interrupt;
         item;
         item = next_item)
    {
        next_item = item->next;

        if (item->func == func && item->context == context)
        {
            if (item->prev) {
                item->prev->next = next_item;
            }
            else {
                osal_global->first_listed_interrupt = next_item;
            }
            if (item->next) {
                item->next->prev = item->prev;
            }
            else {
                osal_global->last_listed_interrupt = item->prev;
            }
            os_free(item, sizeof(osalInterruptInfo));
        }
    }

    os_unlock();
}


/**
****************************************************************************************************

  @brief Enable or disable interrupts.
  @anchor osal_control_interrupts

  The osal_control_interrupts() function...

  @param   enable OS_TRUE to enable interrupts or OS_FALSE to disable those.
  @return  None.

****************************************************************************************************
*/
void osal_control_interrupts(
    os_boolean enable)
{
    osalInterruptInfo *item;

    os_lock();

    if (enable)
    {
        if (osal_global->interrupts_disable_count-- != 1) goto getout;
    }
    else
    {
        if (osal_global->interrupts_disable_count++ != 0) goto getout;
    }

    for (item = osal_global->first_listed_interrupt;
         item;
         item = item->next)
    {
        item->func(enable, item->context);
    }

getout:
    os_unlock();
}

#endif
