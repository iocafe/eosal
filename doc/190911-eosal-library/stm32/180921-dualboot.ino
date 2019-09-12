/*
  dualboot
  For STM32 chips with two flash banks, tested with STM32F429 using linux
  dual boot to load switch between flash banks 1 and 2 to boot from.

  - Arduino IDE creates bin in somewhere like: /tmp/arduino_build_176781/Blink.ino.bin
  - flash to bank 1: st-flash write Blink.ino.bin 0x8000000
  - flash to bank 2: st-flash write Blink.ino.bin 0x8100000

  Build two different version of this sketch. Load one to bank 1 and other to bank 2
  Every time board is rebooted, it switches the bank from which it boots.
 */
 #include "stm32f4xx_hal_flash_ex.h"
 
#define N_LEDS 3
const int leds[N_LEDS] = {PB0, PB14, PB7};


static void toggle_leds(void)
{
    static int lednr = 0;

    digitalWrite(leds[lednr], HIGH);
    delay(50);
    digitalWrite(leds[lednr], LOW);
    delay(300);
    if (++lednr >= N_LEDS) lednr = 0;
}


// the setup routine runs once when you press reset:
void setup() 
{
    FLASH_OBProgramInitTypeDef    OBInit; 
    FLASH_AdvOBProgramInitTypeDef AdvOBInit;
    int lednr;
  
    // initialize the digital pin as an output.
    for (lednr = 0; lednr < N_LEDS; lednr++)
    {
        pinMode(leds[lednr], OUTPUT);
    }

    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
      toggle_leds();
    }

    Serial.println("Dual boot test");

    /* Set BFB2 bit to enable boot from Flash Bank2 */
    /* Allow Access to Flash control registers and user Falsh */
    HAL_FLASH_Unlock();
  
    /* Allow Access to option bytes sector */ 
    HAL_FLASH_OB_Unlock();
    
    /* Get the Dual boot configuration status */
    AdvOBInit.OptionType = OBEX_BOOTCONFIG;
    HAL_FLASHEx_AdvOBGetConfig(&AdvOBInit);
    
    /* Enable/Disable dual boot feature */
    if (((AdvOBInit.BootConfig) & (FLASH_OPTCR_BFB2)) == FLASH_OPTCR_BFB2)
    {
      AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;
      HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
      Serial.println("Set next boot to bank 1");
    }
    else
    {
      AdvOBInit.BootConfig = OB_DUAL_BOOT_ENABLE;
      HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
      Serial.println("Set next boot to bank 2");
    } 
    
    /* Start the Option Bytes programming process */  
    if (HAL_FLASH_OB_Launch() != HAL_OK)
    {
        /* User can add here some code to deal with this error */
        while (1)
        {
            Serial.println("sky is falling`");
        }
    }
   
    /* Prevent Access to option bytes sector */ 
    HAL_FLASH_OB_Lock();
    
    /* Disable the Flash option control register access (recommended to protect 
    the option Bytes against possible unwanted operations) */
    HAL_FLASH_Lock();   
}

// the loop routine runs over and over again forever:
void loop() {
    toggle_leds();
}


