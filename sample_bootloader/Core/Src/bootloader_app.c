/*
 * bootloader_app.c
 *
 *  Created on: Dec 9, 2021
 *      Author: sarveshd
 */

#include <bootloader_app.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <main.h>

extern UART_HandleTypeDef huart3;
static uint32_t flash_ptr = ETX_APP_FLASH_ADDR;
uint16_t  MAX_BYTES_TO_RECIVE = 8;
uint16_t ota_process_data[8] = {0};
uint16_t bootloader_fsm_cmd = 0;
uint16_t bootloader_fsm_data = 0;
uint16_t bootloader_chunk_size = 0;
bool START_OTA_PROCESS = 0;
uint8_t ota_update_command = 0;
uint8_t ota_callback_data[4] = {0};
uint32_t total_size_of_app = 0;
uint32_t total_app_packet_expected_count = 0;
uint32_t current_app_packet_count = 1;
uint32_t current_received_byte_chunk_count = 1;
uint64_t flash_data = 0x0;
bool flash_ready_for_app_writing = 0;

void go_to_application(void){

	  void (*app_reset_handler)(void) = (void*)(*((volatile uint32_t*) (ETX_APP_FLASH_ADDR + 4U)));
	    HAL_RCC_DeInit();
	    HAL_DeInit();

	    SysTick->CTRL = 0;
	    SysTick->LOAD = 0;
	    SysTick->VAL  = 0;

	#if(SET_VECTOR_TABLE)
	    SCB->VTOR = ETX_APP_FLASH_ADDR;
	#endif
	  __set_MSP(*(volatile uint32_t*) ETX_APP_FLASH_ADDR);// setting the value in MSP register

	  // Turn OFF the Green Led to tell the user that Bootloader is not running
	  app_reset_handler();    //call the app reset handler
}

uint8_t bootloader_clear_app_sector(void){
	uint32_t               PageError  = 0;
	FLASH_EraseInitTypeDef pEraseInit;
	HAL_StatusTypeDef      status = HAL_OK;
/* Unlock the flash */
	HAL_FLASH_Unlock();
/* Calculate no of sectors for erasing*/
    pEraseInit.Banks     	= FLASH_BANK_1;

    pEraseInit.Page = (ETX_APP_FLASH_ADDR - FLASH_START_ADDR) / ONE_PAGE_SIZE;
    pEraseInit.NbPages = TOTAL_PAGES -  pEraseInit.Page  ;
    pEraseInit.TypeErase 	= FLASH_TYPEERASE_PAGES;
    status               	= HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    HAL_FLASH_Lock();

    return (status == HAL_OK) ? BL_OK : BL_ERASE_ERROR;

}
/*
 * Function will unlock the flash
 * Call it before starting the app writing process
 * */
uint8_t bootloader_flash_begin(void){
	flash_ptr = ETX_APP_FLASH_ADDR;
	HAL_FLASH_Unlock();
	return BL_OK;
}

/* Function will write the application code chunks in the flash */
uint8_t bootloader_flash_app(uint64_t data){
	if(!(flash_ptr <= (FLASH_END - 8)) || (flash_ptr < ETX_APP_FLASH_ADDR))
    {
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }
	HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_ptr, data);
    if(status == HAL_OK)
    {
        /* Check the written value */
    	if(*(uint64_t*)flash_ptr != data)
    	// if(*(uint64_t*)flash_ptr != data)
        {
            /* Flash content doesn't match source content */
            HAL_FLASH_Lock();
            return BL_WRITE_ERROR;
        }
        /* Increment Flash destination address */
//    	flash_ptr += 1;
    	flash_ptr += 8;
    }
	return BL_OK;
}

/*
 * Function will lock the flash
 * call it after the completion process of app writing
 * */
uint8_t bootloader_flash_end(void){
	HAL_FLASH_Lock();
	return BL_OK;
}
uint8_t Bootloader_CheckForApplication(void)
{
    return (((*(uint32_t*)ETX_APP_FLASH_ADDR) - RAM_BASE) <= RAM_SIZE) ? BL_OK : BL_NO_APP;
}
/*
 * Function will process the received frame from the UART
 **/

uint8_t bootloader_process_received_packet_frame(void){
	bool end_of_frame_flag = 0;
	if(START_OTA_PROCESS == 1) {
		START_OTA_PROCESS = 0;
/* Check for starting of packet frame */
		if(ota_process_data[0] != START_FRAME_PACKET){
			return BL_PARAMETER_INVALID;
		}
/* Check for end of packet frame */
		for(uint8_t i = 1; i < 8; i++){
			if(ota_process_data[i] == END_FRAME_PACKET){
				end_of_frame_flag = 1;
				break;
			}
		}
/* If end_of_frame not received */
		if(end_of_frame_flag != 1){
			return BL_PARAMETER_INVALID;
		}
		/* Set the fsm cmd and data here */
		bootloader_fsm_cmd = ota_process_data[1];
		bootloader_fsm_data = ota_process_data[2];
		bootloader_chunk_size = ota_process_data[3];
		return BL_OK;
	}
}
void ota_callback_function(uint8_t callback_for_cmd, uint8_t status){
	ota_callback_data[0] = START_FRAME_PACKET;
	ota_callback_data[1] = callback_for_cmd;
	ota_callback_data[2] = status;
	ota_callback_data[3] = END_FRAME_PACKET;
	HAL_UART_Transmit(&huart3, ota_callback_data, 4, 500);
	memset(ota_callback_data,0,4);
}

void bootloader_process_fsm(void){
	  switch(bootloader_fsm_cmd){
	  case OTA_IDEAL:
		  break;
	  case OTA_UPDATE_REQUEST:
		  /*Clear the memory for the size of total app size
		   * initialize the packet count variable*/
		  total_app_packet_expected_count = bootloader_fsm_data;
		  current_app_packet_count = 0;
		  current_received_byte_chunk_count = 0;
		  MAX_BYTES_TO_RECIVE = bootloader_chunk_size;
		  memset(ota_process_data,0,2);
		  if(bootloader_clear_app_sector() == BL_OK){
			  bootloader_flash_begin();
			  flash_ready_for_app_writing = 1;
			  HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
			  ota_callback_function(OTA_UPDATE_REQUEST,BL_OK);
		  }
		  bootloader_fsm_cmd = OTA_IDEAL;
		  break;
		  //Write for success here
		  break;
	  case OTA_WRITE_REQUEST:
		  if(flash_ready_for_app_writing){
			flash_data = (flash_data | bootloader_fsm_data);
			current_received_byte_chunk_count ++;
			MAX_BYTES_TO_RECIVE = bootloader_chunk_size;

			if(current_received_byte_chunk_count < MAX_BYTES_TO_RECIVE){
				flash_data = flash_data << 8;
			}

			if(current_received_byte_chunk_count == MAX_BYTES_TO_RECIVE){
				current_received_byte_chunk_count = 0;
				uint8_t write_status = bootloader_flash_app(flash_data);
				current_app_packet_count ++;
				flash_data = 0x0;
				ota_callback_function(OTA_WRITE_REQUEST, write_status);
			  }
		  }
		  bootloader_fsm_cmd = OTA_CHECK_FLASH_PROCESS;
		  //write transmit for status
		  break;
	  case OTA_CHECK_FLASH_PROCESS:
		  if(current_app_packet_count == total_app_packet_expected_count ){
			  flash_ready_for_app_writing = 0;
			  bootloader_fsm_cmd = OTA_END_PROCESS;
			  break;
		  }
		  else{
			  bootloader_fsm_cmd = OTA_IDEAL;
			  break;
		  }
		  break;
	  case OTA_END_PROCESS:
//		  if()
		  bootloader_flash_end();
		  HAL_GPIO_WritePin (GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		  ota_callback_function(OTA_END_PROCESS, BL_OK);
		  bootloader_fsm_cmd = OTA_IDEAL;
		  break;
	  case OTA_JUMP_TO_APP_REQUEST:
		  go_to_application();
		  break;
	  }
}
