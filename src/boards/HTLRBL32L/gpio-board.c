/*!
 * \file      gpio-board.c
 *
 * \brief     Target board GPIO driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include "utilities.h"
#include "sysIrqHandlers.h"
#include "board-config.h"
#include "rtc-board.h"
#include "gpio-board.h"
#if defined( BOARD_IOE_EXT )
#include "gpio-ioe.h"
#endif

#include "bluenrg_lpx.h"
#include "rf_driver_hal_gpio.h"
#include "rf_driver_hal_gpio_ex.h"
#include "rf_driver_hal_exti.h"
#include "rf_driver_hal_rcc.h"
#include "rf_device_hal_conf.h"
#include <stdio.h>

EXTI_HandleTypeDef HEXTI_InitStructure;
static Gpio_t *GpioIrq[16];

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value )
{
    if( pin < IOE_0 )
    {
        GPIO_InitTypeDef GPIO_InitStructure;

        obj->pin = pin;

        if( pin == NC )
        {
            return;
        }

        obj->pinIndex = ( 0x01 << ( obj->pin & 0x0F ) );

        if( ( obj->pin & 0xF0 ) == 0x00 )
        {
            obj->port = GPIOA;
            __HAL_RCC_GPIOA_CLK_ENABLE( );
        }
        else if( ( obj->pin & 0xF0 ) == 0x10 )
        {
            obj->port = GPIOB;
            __HAL_RCC_GPIOB_CLK_ENABLE( );
        }
        else
        {
            assert_param( LMN_STATUS_ERROR );
        }

        GPIO_InitStructure.Pin =  obj->pinIndex ;
        GPIO_InitStructure.Pull = obj->pull = type;
        //GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStructure.Speed =GPIO_SPEED_FREQ_LOW;

        if( mode == PIN_INPUT )
        {
            GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
        }
        else if( mode == PIN_ANALOGIC )
        {
            GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
        }
        else if( mode == PIN_ALTERNATE_FCT )
        {
            if( config == PIN_OPEN_DRAIN )
            {
                GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
            }
            else
            {
                GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
            }
            GPIO_InitStructure.Alternate = value;
        }
        else // mode output
        {
            if( config == PIN_OPEN_DRAIN )
            {
                GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
            }
            else
            {
                GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
            }
        }

        // Sets initial output value
        if( mode == PIN_OUTPUT )
        {
            GpioMcuWrite( obj, value );
        }

        HAL_GPIO_Init( obj->port, &GPIO_InitStructure );
    }
    else
    {
#if defined( BOARD_IOE_EXT )
        // IOExt Pin
        GpioIoeInit( obj, pin, mode, config, type, value );
#endif
    }
}

void GpioMcuSetContext( Gpio_t *obj, void* context )
{
    obj->Context = context;
}

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler )
{

     GPIO_InitTypeDef   GPIO_InitStructure;
  
  EXTI_ConfigTypeDef EXTI_Config_InitStructure;
  
  /* Enable GPIOC clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
  /* Configure PB.4 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

  EXTI_Config_InitStructure.Line =    EXTI_LINE_PB4;
  EXTI_Config_InitStructure.Trigger = EXTI_TRIGGER_RISING_EDGE;
  EXTI_Config_InitStructure.Type =    EXTI_TYPE_EDGE;
   
  HAL_EXTI_SetConfigLine(&HEXTI_InitStructure, &EXTI_Config_InitStructure);
  HAL_EXTI_RegisterCallback(&HEXTI_InitStructure, HAL_EXTI_COMMON_CB_ID, (void(*) (uint32_t))irqHandler);
  HAL_EXTI_Cmd(&HEXTI_InitStructure , ENABLE);
  
  HAL_EXTI_ClearPending(&HEXTI_InitStructure);
  
  /* Enable and set line 10 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(GPIOB_IRQn,2);
  HAL_NVIC_EnableIRQ(GPIOB_IRQn);

	GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = /*GPIO_PIN_2|GPIO_PIN_5|*/GPIO_PIN_1;//GPIO_PIN_9|GPIO_PIN_12|GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*
    if( obj->pin < IOE_0 )
    {
        uint32_t priority = 0;

        IRQn_Type IRQnb = GPIOA_IRQn;
        GPIO_InitTypeDef   GPIO_InitStructure;

        if( irqHandler == NULL )
        {
            return;
        }

        obj->IrqHandler = irqHandler;

        GPIO_InitStructure.Pin =  obj->pinIndex;

        if( irqMode == IRQ_RISING_EDGE )
        {
            GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;
        }
        else if( irqMode == IRQ_FALLING_EDGE )
        {
            GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
        }
        else
        {
            GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;
        }

        GPIO_InitStructure.Pull = obj->pull;
        GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

        HAL_GPIO_Init( obj->port, &GPIO_InitStructure );

        switch( irqPriority )
        {
        case IRQ_VERY_LOW_PRIORITY:
        case IRQ_LOW_PRIORITY:
            priority = 3;
            break;
        case IRQ_MEDIUM_PRIORITY:
            priority = 2;
            break;
        case IRQ_HIGH_PRIORITY:
            priority = 1;
            break;
        case IRQ_VERY_HIGH_PRIORITY:
            priority = 0;
            break;
        }

        if(obj->port == GPIOA) IRQnb = GPIOA_IRQn;
        else if (obj->port == GPIOB) IRQnb = GPIOB_IRQn;

        GpioIrq[( obj->pin ) & 0x0F] = obj;

        HAL_NVIC_SetPriority( IRQnb , priority);
        HAL_NVIC_EnableIRQ( IRQnb );
        
    }
    else
    {
#if defined( BOARD_IOE_EXT )
        // IOExt Pin
        GpioIoeSetInterrupt( obj, irqMode, irqPriority, irqHandler );
#endif
    }
    */
}

void GpioMcuRemoveInterrupt( Gpio_t *obj )
{
    if( obj->pin < IOE_0 )
    {
        // Clear callback before changing pin mode
        GpioIrq[( obj->pin ) & 0x0F] = NULL;

        GPIO_InitTypeDef   GPIO_InitStructure;

        GPIO_InitStructure.Pin =  obj->pinIndex ;
        GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init( obj->port, &GPIO_InitStructure );
    }
    else
    {
#if defined( BOARD_IOE_EXT )
        // IOExt Pin
        GpioIoeRemoveInterrupt( obj );
#endif
    }
}

void GpioMcuWrite( Gpio_t *obj, uint32_t value )
{
    if( obj->pin < IOE_0 )
    {
        if( obj == NULL )
        {
            assert_param( LMN_STATUS_ERROR );
        }
        // Check if pin is not connected
        if( obj->pin == NC )
        {
            return;
        }
        HAL_GPIO_WritePin( obj->port, obj->pinIndex , ( GPIO_PinState )value );
    }
    else
    {
#if defined( BOARD_IOE_EXT )
        // IOExt Pin
        GpioIoeWrite( obj, value );
#endif
    }
}

void GpioMcuToggle( Gpio_t *obj )
{
    if( obj->pin < IOE_0 )
    {
        if( obj == NULL )
        {
            assert_param( LMN_STATUS_ERROR );
        }

        // Check if pin is not connected
        if( obj->pin == NC )
        {
            return;
        }
        HAL_GPIO_TogglePin( obj->port, obj->pinIndex );
    }
    else
    {
#if defined( BOARD_IOE_EXT )
        // IOExt Pin
        GpioIoeToggle( obj );
#endif
    }
}

uint32_t GpioMcuRead( Gpio_t *obj )
{
    if( obj->pin < IOE_0 )
    {
        if( obj == NULL )
        {
            assert_param( LMN_STATUS_ERROR );
        }
        // Check if pin is not connected
        if( obj->pin == NC )
        {
            return 0;
        }
        return HAL_GPIO_ReadPin( obj->port, obj->pinIndex );
    }
    else
    {
#if defined( BOARD_IOE_EXT )
        // IOExt Pin
        return GpioIoeRead( obj );
#else
        return 0;
#endif
    }
}

void GPIOB_IRQHandler( void )
{
    if(HAL_EXTI_GetPending( &HEXTI_InitStructure )){
        HAL_EXTI_IRQHandler( &HEXTI_InitStructure );
    }
}

void HAL_GPIO_EXTI_Callback(GPIO_TypeDef* GPIOx, uint16_t gpioPin ) //TODO
{
    printf("gpiob irq callback\n");

    uint8_t callbackIndex = 0;

    if( gpioPin > 0 )
    {
        while( gpioPin != 0x01 )
        {
            gpioPin = gpioPin >> 1;
            callbackIndex++;
        }
    }

    if( ( GpioIrq[callbackIndex] != NULL ) && ( GpioIrq[callbackIndex]->IrqHandler != NULL ) )
    {
        GpioIrq[callbackIndex]->IrqHandler( GpioIrq[callbackIndex]->Context );
    }
}
void gpio_irq(void);
void IRQHandler_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;
  
  EXTI_ConfigTypeDef EXTI_Config_InitStructure;
  
  /* Enable GPIOC clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
  /* Configure PB.4 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_4;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

  EXTI_Config_InitStructure.Line =    EXTI_LINE_PB4;
  EXTI_Config_InitStructure.Trigger = EXTI_TRIGGER_RISING_EDGE;
  EXTI_Config_InitStructure.Type =    EXTI_TYPE_EDGE;
   
  HAL_EXTI_SetConfigLine(&HEXTI_InitStructure, &EXTI_Config_InitStructure);
  HAL_EXTI_RegisterCallback(&HEXTI_InitStructure, HAL_EXTI_COMMON_CB_ID, (void(*) (uint32_t))gpio_irq);
  HAL_EXTI_Cmd(&HEXTI_InitStructure , ENABLE);
  
  HAL_EXTI_ClearPending(&HEXTI_InitStructure);
  
  /* Enable and set line 10 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(GPIOB_IRQn,2);
  HAL_NVIC_EnableIRQ(GPIOB_IRQn);

	GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = /*GPIO_PIN_2|GPIO_PIN_5|*/GPIO_PIN_1;//GPIO_PIN_9|GPIO_PIN_12|GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}
void gpio_irq(void){
printf("CALLBACK IRQ\n");

}