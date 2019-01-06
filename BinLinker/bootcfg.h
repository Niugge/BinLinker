/*============================= (C) Copyright 2015 Autel ===================================
�ļ���: bootcfg.h
��  ��: ZZH
��  ��: V0.1
��  ��: 2015/05/06
��  ��: boot area configuration include file
ά  ��: None
================================ All rights reserved =====================================*/
#ifndef __BOOTCFG_H__
#define __BOOTCFG_H__ 

/* Includes ------------------------------------------------------------------------------*/
#include <stdio.h>

/* Exported macros -----------------------------------------------------------------------*/
#define MAGIC_DO_UPGRADE			0xAA55AA01		/* ����Bootloaderģʽ	*/
#define MAGIC_APP_VALID				0xBEEF1234		/* ��ǰApp��Ч			*/	//0xAA55AA02

/* Exported typedef ----------------------------------------------------------------------*/
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;


/* ���ò��������ݽṹ */
typedef struct {
#if 1
	uint32_t magic; 			/*													*/
	uint32_t device_id; 		/* �豸ID���������ֲ�ͬ���豸������Ψһ��			*/
	uint32_t hw_version;		/* Ӳ���汾��										*/	
	uint32_t boot_version;		/* Bootloader����汾�� 							*/
	uint32_t boot_size;				/* ���� 											*/
	uint32_t boot_crc32;				/* ���� 											*/
	uint32_t rev3;				/* ���� 											*/		
	uint32_t app_version;		/* App����汾��									*/
	uint32_t app_size;			/* App��С���ֽڣ�									*/
	uint32_t app_crc32; 		/* App��CRC32����ֵ 								*/
	uint32_t min_hw_version;	/* �̼�֧�ֵ����Ӳ���汾�ţ������� 				*/
	uint32_t max_hw_version;	/* �̼�֧�ֵ����Ӳ���汾�� ��������				*/
	uint8_t  manufacture[64];			/* ����������Ϣ 									*/
	uint8_t  serial_number[96]; 		/* ��Ʒ���к�										*/
#else

	uint32_t magic;				/* 													*/
	uint32_t device_id;			/* �豸ID���������ֲ�ͬ���豸������Ψһ��			*/
	uint32_t boot_hw_version;	/* BootloaderӲ���汾��								*/	
	uint32_t boot_sw_version;	/* Bootloader����汾��								*/
	uint32_t boot_size;			/* Bootloader��С���ֽڣ�							*/
	uint32_t boot_crc32;		/* Bootloaser��CRC32����ֵ							*/
	uint32_t app_hw_version;	/* AppӲ���汾��(һ����BootloaderӲ���汾��һ��	)	*/		
	uint32_t app_sw_version;	/* App����汾��									*/
	uint32_t app_size;			/* App��С���ֽڣ�									*/
	uint32_t app_crc32;			/* App��CRC32����ֵ									*/
	uint32_t rls_hw_version;	/* ͳһ����Ӳ���汾�ţ����ⷢ���汾��Ϣ��			*/
	uint32_t rls_sw_version;	/* ͳһ��������汾�ţ����ⷢ���汾��Ϣ��			*/
	uint8_t  manufacture[64];	/* ����������Ϣ										*/
	uint8_t  serial_number[96];	/* ��Ʒ���к�										*/
#endif
} boot_config_t;

/* Exported functions --------------------------------------------------------------------*/

#endif /*__BOOTCFG_H__*/
/*--------------------------------- The End ----------------------------------------------*/

