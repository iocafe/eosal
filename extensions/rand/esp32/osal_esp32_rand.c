/**

  @file    rand/esp32/osal_esp32_rand.c
  @brief   Random numbers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  ESP32 contains a hardware random number generator, values from it can be obtained
  using esp_random().

  When Wi-Fi or Bluetooth are enabled, numbers returned by hardware random number generator
  can be considered true random numbers. Without Wi-Fi or Bluetooth enabled, hardware RNG is
  a pseudo-random number generator. At startup, ESP-IDF bootloader seeds the hardware RNG with
  entropy, but care must be taken when reading random values between the start of app_main and
  initialization of Wi-Fi or Bluetooth drivers.  In future this can be replaced with better
  alternative.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ESP32
#if OSAL_RAND_SUPPORT == OSAL_RAND_PLATFORM

#include "esp_system.h"
#include "esp_random.h"


/**
****************************************************************************************************

  @brief Set pseudo random number generator seed.
  @anchor osal_rand_seed

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

  ESP32 specific: Not needed, wifi or bloetooth initialization sets up random numbers.

  @param   ent Entropy (from physical random source) to seed the random number generator.
  @param   ent_sz Entropy size in bytes.
  @return  None.

****************************************************************************************************
*/
void osal_rand_seed(
    const os_char *ent,
    os_memsz ent_sz)
{
    OSAL_UNUSED(ent);
    OSAL_UNUSED(ent_sz);
}


/**
****************************************************************************************************

  @brief Get a pseudo random number.
  @anchor osal_rand

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

  @param   min_value Minimum value for random number.
  @param   max_value Maximum value for random number.
  @return  Random number from min_value to max_value. If min_value equals max_value, all
           64 bits of return value are random data.

****************************************************************************************************
*/
os_long osal_rand(
    os_long min_value,
    os_long max_value)
{
    os_long x;
    os_ulong n;

    esp_fill_random(&x, sizeof(x));
    if (max_value != min_value)
    {
        n = (os_ulong)(max_value - min_value + 1);
        x = (os_long)((os_ulong)x % n);
        x += min_value;
    }
    return x;
}

#endif
#endif