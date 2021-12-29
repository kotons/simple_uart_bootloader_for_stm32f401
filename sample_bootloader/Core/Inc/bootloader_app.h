/*
 * bootloader_app.h
 *
 *  Created on: Dec 9, 2021
 *      Author: sarveshd
 */

#ifndef INC_BOOTLOADER_APP_H_
#define INC_BOOTLOADER_APP_H_
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define FLASH_START_ADDR 0x08000000
#define ETX_APP_FLASH_ADDR 0x08040000

#define TOTAL_PAGES		(256)
#define ONE_PAGE_SIZE 	(2047)
#define SET_VECTOR_TABLE 1
#define RAM_BASE SRAM1_BASE     /*!< Start address of RAM */
#define RAM_SIZE SRAM1_SIZE_MAX /*!< RAM size in bytes */


void go_to_application(void);
uint8_t bootloader_clear_app_sector(void);
uint8_t bootloader_flash_begin(void);
uint8_t bootloader_flash_app(uint64_t data);
uint8_t bootloader_flash_end(void);
uint8_t Bootloader_CheckForApplication(void);
uint8_t bootloader_process_received_packet_frame(void);
void ota_callback_function(uint8_t callback_for_cmd, uint8_t status);
void bootloader_process_fsm(void);
enum eBootloaderErrorCodes
{
    BL_OK = 0,      /*!< No error */
    BL_NO_APP,      /*!< No application found in flash */
    BL_SIZE_ERROR,  /*!< New application is too large for flash */
    BL_CHKS_ERROR,  /*!< Application checksum error */
    BL_ERASE_ERROR, /*!< Flash erase error */
    BL_WRITE_ERROR, /*!< Flash write error */
    BL_OBP_ERROR,   /*!< Flash option bytes programming error */
	BL_REINIT_FLAG,  /*!< Bootloader whole process needs to be re-initiated */
	BL_PARAMETER_INVALID,
	BL_INVALID_FLASH_CONTENT
};
enum OTA_FRAME_STRUCTURE{
	START_FRAME_PACKET = 1,
	END_FRAME_PACKET
};
enum ota_request_states{
	OTA_IDEAL,
	OTA_UPDATE_REQUEST,
	OTA_WRITE_REQUEST,
	OTA_CHECK_FLASH_PROCESS,
	OTA_END_PROCESS,
	OTA_JUMP_TO_APP_REQUEST,
	OTA_CALLBACK};
#endif /* INC_BOOTLOADER_APP_H_ */
