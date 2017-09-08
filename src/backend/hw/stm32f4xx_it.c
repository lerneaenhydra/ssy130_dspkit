/**
  ******************************************************************************
  * @file    Audio_playback_and_record/src/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    28-October-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************/ 

#include "stm32f4xx_it.h"
#include "stm32f4_discovery.h"

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void){}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void){
  /* Go to infinite loop when Memory Manage exception occurs */
  for(;;){};
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void){
  /* Go to infinite loop when Bus Fault exception occurs */
  for(;;){};
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void){
  /* Go to infinite loop when Usage Fault exception occurs */
  for(;;){};
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void){}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void){}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void){}


/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/
/**
  * @brief  This function handles External line 1 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI1_IRQHandler(void){
  if(EXTI_GetITStatus(EXTI_Line1) != RESET){
    EXTI_ClearITPendingBit(EXTI_Line1);
  }
}

/**
  * @brief  EXTI0_IRQHandler
  *         This function handles External line 0 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void){
  /* Clears the EXTI's line pending bit.*/ 
  EXTI_ClearITPendingBit(EXTI_Line0);
}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/


/**
  * @}
  */ 
  
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
