/*============================= (C) Copyright 2015 Autel ===================================
文件名: bootcfg.h
作  者: ZZH
版  本: V0.1
日  期: 2015/05/06
描  述: boot area configuration include file
维  护: None
================================ All rights reserved =====================================*/
#ifndef __BOOTCFG_H__
#define __BOOTCFG_H__ 

/* Includes ------------------------------------------------------------------------------*/
#include <stdio.h>

/* Exported macros -----------------------------------------------------------------------*/
#define MAGIC_DO_UPGRADE			0xAA55AA01		/* 进入Bootloader模式	*/
#define MAGIC_APP_VALID				0xBEEF1234		/* 当前App有效			*/	//0xAA55AA02

/* Exported typedef ----------------------------------------------------------------------*/
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;


/* 配置参数区数据结构 */
typedef struct {
#if 1
	uint32_t magic; 			/*													*/
	uint32_t device_id; 		/* 设备ID，用于区分不同的设备，具有唯一性			*/
	uint32_t hw_version;		/* 硬件版本号										*/	
	uint32_t boot_version;		/* Bootloader软件版本号 							*/
	uint32_t boot_size;				/* 保留 											*/
	uint32_t boot_crc32;				/* 保留 											*/
	uint32_t rev3;				/* 保留 											*/		
	uint32_t app_version;		/* App软件版本号									*/
	uint32_t app_size;			/* App大小（字节）									*/
	uint32_t app_crc32; 		/* App区CRC32检验值 								*/
	uint32_t min_hw_version;	/* 固件支持的最低硬件版本号（包含） 				*/
	uint32_t max_hw_version;	/* 固件支持的最高硬件版本号 （包含）				*/
	uint8_t  manufacture[64];			/* 生产厂商信息 									*/
	uint8_t  serial_number[96]; 		/* 产品序列号										*/
#else

	uint32_t magic;				/* 													*/
	uint32_t device_id;			/* 设备ID，用于区分不同的设备，具有唯一性			*/
	uint32_t boot_hw_version;	/* Bootloader硬件版本号								*/	
	uint32_t boot_sw_version;	/* Bootloader软件版本号								*/
	uint32_t boot_size;			/* Bootloader大小（字节）							*/
	uint32_t boot_crc32;		/* Bootloaser区CRC32检验值							*/
	uint32_t app_hw_version;	/* App硬件版本号(一般与Bootloader硬件版本号一样	)	*/		
	uint32_t app_sw_version;	/* App软件版本号									*/
	uint32_t app_size;			/* App大小（字节）									*/
	uint32_t app_crc32;			/* App区CRC32检验值									*/
	uint32_t rls_hw_version;	/* 统一发布硬件版本号（对外发布版本信息）			*/
	uint32_t rls_sw_version;	/* 统一发布软件版本号（对外发布版本信息）			*/
	uint8_t  manufacture[64];	/* 生产厂商信息										*/
	uint8_t  serial_number[96];	/* 产品序列号										*/
#endif
} boot_config_t;

/* Exported functions --------------------------------------------------------------------*/

#endif /*__BOOTCFG_H__*/
/*--------------------------------- The End ----------------------------------------------*/

