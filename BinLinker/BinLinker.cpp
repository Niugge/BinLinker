// BinLinker.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "string.h"
#include "bootcfg.h"
#include "time.h"
#include "aes.h"
#include "iostream"

#pragma comment( lib, "cryptlib.lib")

using namespace std;
using namespace CryptoPP;

/* Private macro -------------------------------------------------------------------------*/
#define MIN(a,b)				((a)<(b)?(a):(b))
#define DATA_BUF_MAX			1024
#define DATA_BLOCK_SIZE			AES::BLOCKSIZE      //16
#define BOOTCFG_BLOCK_SIZE		240	/* �����ļ�ǰ240�ֽ������������ݣ������ܣ���С����λ���·�һ�����ݴ�С��ͬ */


/* Private variables ---------------------------------------------------------------------*/
/* ������Կ */
const unsigned char AES_Key[AES::DEFAULT_KEYLENGTH] = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
	0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static boot_config_t g_bootcfg;

static uint32_t g_app_crc32;

/* Private function prototypes -----------------------------------------------------------*/
static void AES_EnCode(unsigned char *Key, unsigned char *InBlock, unsigned char *OutBlock);
static void AES_DeCode(unsigned char *Key, unsigned char *InBlock, unsigned char *OutBlock);
static int make_image_RF_file(int argc, char *argv[]);
static int make_image_RC_file(int argc, char *argv[]);
static int make_upgrade_file(int argc, char *argv[]);
static int make_image_RC_Loader_file(int argc, char *argv[]);
static uint32_t CRC_CalcBlack(uint8_t *ucBuf, uint32_t uiLen, uint32_t uiSeed = 0x00000000);

/* Private functions ---------------------------------------------------------------------*/

static void AES_EnCode(unsigned char *Key, unsigned char *InBlock, unsigned char *OutBlock)
{
	AESEncryption aesEncryptor; //������
	unsigned char xorBlock[AES::BLOCKSIZE]; //�����趨Ϊȫ��
	memset(xorBlock, 0, AES::BLOCKSIZE); //����
	aesEncryptor.SetKey(Key, AES::DEFAULT_KEYLENGTH);  //�趨������Կ
	aesEncryptor.ProcessAndXorBlock(InBlock, xorBlock, OutBlock);  //����
	return;
}

static void AES_DeCode(unsigned char *Key, unsigned char *InBlock, unsigned char *OutBlock)
{
	AESDecryption aesDecryptor;
	unsigned char xorBlock[AES::BLOCKSIZE]; //�����趨Ϊȫ��

	memset(xorBlock, 0, AES::BLOCKSIZE); //����
	aesDecryptor.SetKey(Key, AES::DEFAULT_KEYLENGTH);
	aesDecryptor.ProcessAndXorBlock(InBlock, xorBlock, OutBlock);
}

static uint32_t CRC_CalcBlack(uint8_t *ucBuf, uint32_t uiLen, uint32_t uiSeed)
{
	//unsigned int crc = 0xFFFFFFFF;
	unsigned int crc = uiSeed;/*0x00000000;*/
	if (NULL == ucBuf || uiLen <= 0) { return crc; }
	//�������unsigned int������������0xff����
	unsigned char *ucData = new unsigned char[uiLen + 4];
	if (NULL == ucData) { return crc; }
	memcpy_s(ucData, uiLen + 4, ucBuf, uiLen);
	unsigned char* ucOld = ucData;
	int rem = uiLen % sizeof(unsigned int);
	if (rem > 0) {
		int n = sizeof(unsigned int)-rem;
		for (int i = 0; i < n; i++) {
			ucData[uiLen + i] = 0xff;
		}
		uiLen += n;
	}

	unsigned int uiCount = uiLen / sizeof(unsigned int);
	for (unsigned int i = 0; i < uiCount; i++) {
		unsigned int uiTemp = *(unsigned int*)ucData;
		for (unsigned int j = 0; j < 32; j++) {
			if ((crc^uiTemp) & 0x80000000) {
				crc = 0x04C11DB7 ^ (crc << 1);
			}
			else {
				crc <<= 1;
			}
			uiTemp <<= 1;
		}
		ucData += sizeof(unsigned int);
	}

	if (NULL != ucOld) { delete ucOld; ucOld = NULL; }
	return crc;
}
#if 1
/******************************************************************************************
*										������ڣ�MAIN��								  *
*******************************************************************************************/
int _tmain(int argc, char *argv[])
{
	int ret = 0;


	printf("\n\r");
	printf("***********************************************************************\n\r");
	printf("***               Embedded Software Release Tool v7.1              ****\n\r");
	printf("***                    (C) Copyright 2017 Autel                    ****\n\r");
	printf("***                      All rights reserved                       ****\n\r");
	printf("***                           By Autel		                   ****\n\r");
	printf("***                          2017-10-19                            ****\n\r");
	printf("***********************************************************************\n\r");
	printf("\n\r");

	/* �������������ݽṹ��ʼ�� */
	memset((void *)&g_bootcfg, 0, sizeof(g_bootcfg));
	g_bootcfg.magic = MAGIC_APP_VALID;

	/* ��鴫������Ϸ��ԣ������ݲ�ͬ�Ĵ�������������ɾ����ļ����������ļ� */
	if (argc == 15){
		ret = make_image_RC_file(argc, argv);	//Ŀ���ļ���RC�����ļ�
	}
	else if (argc == 11){
		//ret = make_image_RF_file(argc, argv);	//Ŀ���ļ���RF�����ļ�
	}
	else if (argc == 10){
		ret = make_upgrade_file(argc, argv);    //Ŀ���ļ��������ļ�
	}
	else if (argc == 14){
		ret = make_image_RC_Loader_file(argc, argv);	//Ŀ����bootloader�ļ�
	}
	else{
		printf("\n\r");
		printf("[ERROR] Input parameters are invalid!\n\r");
		return 0;
	}
	/*
	FILE *pAppFile;
	char *pAppPath = ".\src\remote.bin";
	pAppFile = fopen(pAppPath, "rb"); //pAppPath

	if (pAppFile == NULL)
	{
		printf("FILE IS NULL!\n");
	}
*/
	return ret;
}
#endif
/******************************************************************************************
*										���������ļ�								      *
*******************************************************************************************/
static int make_upgrade_file(int argc, char *argv[])
{
	char *pAppPath, *pTargetPath;
	char *pDeviceID, *pHwVersion, *pBootloaderVersion, *pAppSwVersion, *pRlsHwVersion, *pRlsSwVersion, *pManufacture;
	FILE *pAppFile, *pTargetFile;
	int vAppSize=0, vTargetSize=0, vReadLen, vWriteLen, vBlocks, vBytes, vAppendBytes=0;
	int version[4], tmp;
	unsigned char EncodeBuf[DATA_BUF_MAX], Buf[DATA_BUF_MAX];

	char ImageName[60];
	time_t now;
	struct tm curTime;


	/*��ȡϵͳʱ����Ϣ*/
	time(&now);
	localtime_s(&curTime, &now);


	/* ��ȡ������� */
	pAppPath = argv[1];
	pTargetPath = argv[2];
	pDeviceID = argv[3];
	pHwVersion = argv[4];
	pBootloaderVersion = argv[5];
	pAppSwVersion = argv[6];
	pRlsSwVersion = argv[7];
	pRlsHwVersion = argv[8];
	pManufacture = argv[9];

	/* ���汾��Ϣ���ַ���ת�������� */
	sscanf_s(pDeviceID, "%d", &version[0]);
	g_bootcfg.device_id = (uint32_t)version[0];

	sscanf_s(pHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pBootloaderVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.boot_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pAppSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.app_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pRlsSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.max_hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pRlsHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.min_hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	tmp = MIN(strlen(pManufacture), sizeof(g_bootcfg.manufacture));
	if (pManufacture != NULL){
		strncpy((char *)g_bootcfg.manufacture, (const char *)pManufacture, tmp);
	}
	//printf("%s\n", pAppPath);
	/* ��ֻ����ʽ��Application�ļ� */
	pAppFile = fopen(pAppPath, "rb");
	if (pAppFile == NULL){
		printf("[ERROR] Open the application file failed,please check the path and file name.\n\r");
		return 0;
	}
#if 1
	/* ����Ŀ���ļ� */
	sprintf(ImageName, "./%s_V%s_%04d%02d%02d.upg", pTargetPath, pAppSwVersion,
		curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday);
	pTargetPath = ImageName;
	pTargetFile = fopen(pTargetPath, "wb");
	if (pTargetFile == NULL){
		printf("[ERROR] Create the target file failed,please check the path and file name.\n\r");
		fclose(pAppFile);
		return 0;
	}

	/* ��ȡApplication�ļ���С */
	fseek(pAppFile, 0, SEEK_END);
	vAppSize = ftell(pAppFile);
	g_bootcfg.app_size = vAppSize;
	if (vAppSize < 0){
		printf("[ERROR] Application get size failed.\n\r");
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}
	printf("[OK] Application file size: %d bytes.\n\r", vAppSize);

	/* ����Application CRC32 */
	fseek(pAppFile, 0, SEEK_SET);
	g_app_crc32 = 0x00000000; //����ֵ
	for (int i = 0; i<vAppSize; i += tmp){
		tmp = fread(Buf, 1, sizeof(Buf), pAppFile);
		if (tmp > 0){
			g_app_crc32 = CRC_CalcBlack(Buf, tmp, g_app_crc32);
		}
	}
	g_bootcfg.app_crc32 = g_app_crc32;
	/* д��Bootconfig������ */
	fseek(pTargetFile, 0, SEEK_SET);
	vTargetSize = 0; //��ʱĿ���ļ���СΪ0
	vWriteLen = fwrite((void *)&g_bootcfg, 1, sizeof(boot_config_t), pTargetFile);
	vTargetSize += vWriteLen;
	if (vWriteLen == -1){
		printf("[ERROR] Write data to file failed.\n\r");
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}

	/* Bootconfig�����沿��ȫ��д0xFF */
	if (vTargetSize < BOOTCFG_BLOCK_SIZE){
		vBlocks = (BOOTCFG_BLOCK_SIZE - vTargetSize) / sizeof(Buf);
		vBytes = (BOOTCFG_BLOCK_SIZE - vTargetSize) % sizeof(Buf);
		memset(Buf, 0xFF, sizeof(Buf));
		while (vBlocks--){
			vWriteLen = fwrite(Buf, 1, sizeof(Buf), pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
		if (vBytes){
			vWriteLen = fwrite(Buf, 1, vBytes, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ��Application�ļ�������Target�ļ� */
	fseek(pAppFile, 0, SEEK_SET);
	while (vTargetSize < (vAppSize + BOOTCFG_BLOCK_SIZE)){
		vReadLen = fread(Buf, 1, sizeof(Buf), pAppFile);
		if (vReadLen > 0){
			/* �����ݷֿ����(ÿ�����СΪDATA_BLOCK_SIZE,�����0xFF) */
			vBytes = vReadLen % DATA_BLOCK_SIZE;
			if (vBytes != 0){
				memset(&Buf[vReadLen], 0xFF, (DATA_BLOCK_SIZE - vBytes));
				vAppendBytes = DATA_BLOCK_SIZE - vBytes;
				vReadLen += vAppendBytes;
			}
			vBlocks = vReadLen / DATA_BLOCK_SIZE;
			for (int i = 0; i<vBlocks; i++){
				AES_EnCode((unsigned char *)AES_Key, (unsigned char *)&Buf[i*DATA_BLOCK_SIZE], (unsigned char *)&EncodeBuf[i*DATA_BLOCK_SIZE]);
			}

			vWriteLen = fwrite(EncodeBuf, 1, vReadLen, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ���Application�ļ��Ƿ��Ѿ���ȫ������Target�ļ��� */
	if (vTargetSize != (vAppSize + vAppendBytes + BOOTCFG_BLOCK_SIZE)){
		printf("[ERROR] Copy application file to Target file failed.\n\r");
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}

	/* �ɹ�����Target�ļ� */
	printf("[OK] Target file size: %d bytes.\n\r", vTargetSize);
	printf("[OK] Create target file: %s\n\r\n\r", pTargetPath);
	printf("\n\rCreate Applicaton file successfully!!!\n\r\n\r");

	if (pAppFile != NULL){
		fclose(pAppFile);
	}
	if (pTargetFile != NULL){
		fclose(pTargetFile);
	}
#endif
	return 0;
}

/******************************************************************************************
*										����RC�����ļ�								      *
*******************************************************************************************/
static int make_image_RC_file(int argc, char *argv[])
{
	int vFlashAppOffset = 0, vFlashConfigOffset;
	char *pBootloaderPath, *pAppPath, *pTargetPath, *pAppAddressOffset, *pCfgAddressOffset;
	char *pDeviceID, *pBootHwVersion, *pBootSwVersion, *pAppHwVersion, *pAppSwVersion, *pRlsHwVersion, *pRlsSwVersion;
	char *pManufacture, *pSerialNumber;
	FILE *pBootloaderFile, *pAppFile, *pTargetFile;
	int vBootloaderSize, vAppSize, vTargetSize, vReadLen, vWriteLen, vBlocks, vBytes;
	int version[4], tmp;
	unsigned char buf[DATA_BUF_MAX];

	char ImageName[60];
	time_t now;
	struct tm curTime;


	/*��ȡϵͳʱ����Ϣ*/
	time(&now);
	localtime_s(&curTime, &now);

	/* ��ȡӦ�ó�������� */
	pBootloaderPath		= argv[1];
	pAppPath			= argv[2];
	pTargetPath			= argv[3];
	pDeviceID			= argv[4];
	pCfgAddressOffset	= argv[5];
	pAppAddressOffset	= argv[6];
	pBootHwVersion		= argv[7];
	pBootSwVersion		= argv[8];
	pAppHwVersion		= argv[9];
	pAppSwVersion		= argv[10];
	pRlsHwVersion		= argv[11];
	pRlsSwVersion		= argv[12];
	pManufacture		= argv[13];
	pSerialNumber		= argv[14];

	/* ���汾��Ϣ���ַ���ת�������� */
	sscanf_s(pDeviceID, "%d", &version[0]);
	g_bootcfg.device_id = (uint32_t)version[0];

	sscanf_s(pBootHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pBootSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.boot_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pAppHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pAppSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.app_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pRlsHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.min_hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pRlsSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.max_hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);


	tmp = MIN(strlen(pManufacture), sizeof(g_bootcfg.manufacture));
	if (pManufacture != NULL){
		strncpy((char *)g_bootcfg.manufacture, (const char *)pManufacture, tmp);
	}

	tmp = MIN(strlen(pSerialNumber), sizeof(g_bootcfg.serial_number));
	if (pSerialNumber != NULL){
		strncpy((char *)g_bootcfg.serial_number, (const char *)pSerialNumber, tmp);
	}

	/* ��ȡBootconfig��Flash�е�ƫ�Ƶ�ַ�����ж��Ƿ�Ϸ� */
	if (pCfgAddressOffset == NULL){
		printf("[ERROR] Get the bootconfig offset address failed.\n\r");
		return 0;
	}
	sscanf_s(pCfgAddressOffset, "%X", &vFlashConfigOffset);
	if (vFlashConfigOffset > 0){
		if ((vFlashConfigOffset % 4) == 0){
			printf("[OK] The bootconfig offset address is 0x%X.\n\r", vFlashConfigOffset);
		}
		else{
			printf("[WARNING]  The bootconfig offset address is 0x%X. Not align to 4 bytes!!!\n\r", vFlashConfigOffset);
		}
	}
	else{
		printf("[WARNING] The bootconfig offset address is 0x00.\n\r");
	}

	/* ��ȡApplication��Flash�е�ƫ�Ƶ�ַ�����ж��Ƿ�Ϸ� */
	if (pAppAddressOffset == NULL){
		printf("[ERROR] Get the application offset address failed.\n\r");
		return 0;
	}
	sscanf_s(pAppAddressOffset, "%X", &vFlashAppOffset);
	if (vFlashAppOffset > 0){
		if ((vFlashAppOffset % 4) == 0){
			printf("[OK] The application offset address is 0x%X.\n\r", vFlashAppOffset);
		}
		else{
			printf("[WARNING]  The application offset address is 0x%X. Not align to 4 bytes!!!\n\r", vFlashAppOffset);
		}
	}
	else{
		printf("[WARNING] The application offset address is 0x00.\n\r");
	}

	/* ��ֻ����ʽ��Bootloader�ļ� */
	pBootloaderFile = fopen(pBootloaderPath, "rb");
	if (pBootloaderFile == NULL){
		printf("[ERROR] Open the bootloader file failed,please check the path and file name.\n\r");
		return 0;
	}

	/* ��ֻ����ʽ��Application�ļ� */
	pAppFile = fopen(pAppPath, "rb");
	if (pAppFile == NULL){
		printf("[ERROR] Open the application file failed,please check the path and file name.\n\r");
		fclose(pBootloaderFile);
		return 0;
	}

	/* ����Ŀ���ļ� */
	sprintf(ImageName, "./%s_V%sV%s_%04d%02d%02d.bin", pTargetPath, pBootSwVersion, pAppSwVersion,
		curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday);
	pTargetPath = ImageName;
	pTargetFile = fopen(pTargetPath, "wb");
	if (pTargetFile == NULL){
		printf("[ERROR] Create the target file failed,please check the path and file name.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		return 0;
	}

	/* ��ȡBootloader�ļ���С */
	fseek(pBootloaderFile, 0, SEEK_END);
	vBootloaderSize = ftell(pBootloaderFile);
	g_bootcfg.boot_size = vBootloaderSize;
	if (vBootloaderSize < 0){
		printf("[ERROR] Bootloader get size failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}
	if (vBootloaderSize > vFlashAppOffset){
		printf("[ERROR] The size of bootloader file is too large.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}
	printf("[OK] Bootloader file size: %d bytes.\n\r", vBootloaderSize);

	/* ��ȡApplication�ļ���С */
	fseek(pAppFile, 0, SEEK_END);
	vAppSize = ftell(pAppFile);
	g_bootcfg.app_size = vAppSize;
	if (vAppSize < 0){
		printf("[ERROR] Application get size failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}
	printf("[OK] Application file size: %d bytes.\n\r", vAppSize);

	/* ����Application CRC32 */
	fseek(pAppFile, 0, SEEK_SET);
	g_app_crc32 = 0x00000000; //����ֵ
	for (int i = 0; i<vAppSize; i += tmp){
		tmp = fread(buf, 1, sizeof(buf), pAppFile);
		if (tmp > 0){
			g_app_crc32 = CRC_CalcBlack(buf, tmp, g_app_crc32);
		}
	}
	g_bootcfg.app_crc32 = g_app_crc32;

	/* ��Bootloader�ļ�������Target�ļ� */
	fseek(pBootloaderFile, 0, SEEK_SET);
	fseek(pTargetFile, 0, SEEK_SET);
	vTargetSize = 0; //��ʱĿ���ļ���СΪ0
	while (vTargetSize < vBootloaderSize){
		vReadLen = fread(buf, 1, sizeof(buf), pBootloaderFile);
		if (vReadLen > 0){
			vWriteLen = fwrite(buf, 1, vReadLen, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ���Bootloader�ļ��Ƿ��Ѿ���ȫ������Target�ļ��� */
	if (vTargetSize != vBootloaderSize){
		printf("[ERROR] Copy Bootloader file to Target file failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}

	/* Bootloader�����沿��ȫ��д0xFF */
	if (vTargetSize < vFlashConfigOffset){
		vBlocks = (vFlashConfigOffset - vTargetSize) / sizeof(buf);
		vBytes = (vFlashConfigOffset - vTargetSize) % sizeof(buf);
		memset(buf, 0xFF, sizeof(buf));
		while (vBlocks--){
			vWriteLen = fwrite(buf, 1, sizeof(buf), pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
		if (vBytes){
			vWriteLen = fwrite(buf, 1, vBytes, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* д��Bootconfig������ */
	vWriteLen = fwrite((void *)&g_bootcfg, 1, sizeof(boot_config_t), pTargetFile);
	vTargetSize += vWriteLen;
	if (vWriteLen == -1){
		printf("[ERROR] Write data to file failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}

	/* Bootconfig�����沿��ȫ��д0xFF */
	if (vTargetSize < vFlashAppOffset){
		vBlocks = (vFlashAppOffset - vTargetSize) / sizeof(buf);
		vBytes = (vFlashAppOffset - vTargetSize) % sizeof(buf);
		memset(buf, 0xFF, sizeof(buf));
		while (vBlocks--){
			vWriteLen = fwrite(buf, 1, sizeof(buf), pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
		if (vBytes){
			vWriteLen = fwrite(buf, 1, vBytes, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ��Application�ļ�������Target�ļ� */
	fseek(pAppFile, 0, SEEK_SET);
	while (vTargetSize < (vFlashAppOffset + vAppSize)){
		vReadLen = fread(buf, 1, sizeof(buf), pAppFile);
		if (vReadLen > 0){
			vWriteLen = fwrite(buf, 1, vReadLen, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ���Application�ļ��Ƿ��Ѿ���ȫ������Target�ļ��� */
	if (vTargetSize != (vFlashAppOffset + vAppSize)){
		printf("[ERROR] Copy application file to Target file failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}

	/* �ɹ�����Target�ļ� */
	printf("[OK] Target file size: %d bytes.\n\r", vTargetSize);
	printf("[OK] Create target file: %s\n\r\n\r", pTargetPath);
	printf("\n\rCreate IMAGE file successfully!!!\n\r\n\r");

	if (pBootloaderFile != NULL){
		fclose(pBootloaderFile);
	}
	if (pAppFile != NULL){
		fclose(pAppFile);
	}
	if(pTargetFile != NULL){
		fclose(pTargetFile);
	}

	return 0;
}

/******************************************************************************************
*										����RC LOADER�����ļ�								      *
*******************************************************************************************/
static int make_image_RC_Loader_file(int argc, char *argv[])
{
	int vFlashAppOffset = 0, vFlashConfigOffset;
	char *pBootloaderPath, *pAppPath, *pTargetPath, *pAppAddressOffset, *pCfgAddressOffset;
	char *pDeviceID, *pBootHwVersion, *pBootSwVersion, *pAppHwVersion, *pAppSwVersion, *pRlsHwVersion, *pRlsSwVersion;
	char *pManufacture, *pSerialNumber;
	FILE *pBootloaderFile, *pAppFile, *pTargetFile;
	int vBootloaderSize, vAppSize, vTargetSize, vReadLen, vWriteLen, vBlocks, vBytes;
	int version[4], tmp;
	unsigned char buf[DATA_BUF_MAX];

	char ImageName[60];
	time_t now;
	struct tm curTime;


	/*��ȡϵͳʱ����Ϣ*/
	time(&now);
	localtime_s(&curTime, &now);

	/* ��ȡӦ�ó�������� */
	pBootloaderPath		= argv[1];
	pTargetPath			= argv[2];
	pDeviceID			= argv[3];
	pCfgAddressOffset	= argv[4];
	pAppAddressOffset	= argv[5];
	pBootHwVersion		= argv[6];
	pBootSwVersion		= argv[7];
	pAppHwVersion		= argv[8];
	pAppSwVersion		= argv[9];
	pRlsHwVersion		= argv[10];
	pRlsSwVersion		= argv[11];
	pManufacture		= argv[12];
	pSerialNumber		= argv[13];

	/* ���汾��Ϣ���ַ���ת�������� */
	sscanf_s(pDeviceID, "%d", &version[0]);
	g_bootcfg.device_id = (uint32_t)version[0];

	sscanf_s(pBootHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pBootSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.boot_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pAppHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pAppSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.app_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pRlsHwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.min_hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);

	sscanf_s(pRlsSwVersion, "%d.%d.%d.%d", &version[0], &version[1], &version[2], &version[3]);
	g_bootcfg.max_hw_version = (uint32_t)((version[0] << 24) | (version[1] << 16) | (version[2] << 8) | version[3]);


	tmp = MIN(strlen(pManufacture), sizeof(g_bootcfg.manufacture));
	if (pManufacture != NULL){
		strncpy((char *)g_bootcfg.manufacture, (const char *)pManufacture, tmp);
	}

	tmp = MIN(strlen(pSerialNumber), sizeof(g_bootcfg.serial_number));
	if (pSerialNumber != NULL){
		strncpy((char *)g_bootcfg.serial_number, (const char *)pSerialNumber, tmp);
	}

	/* ��ȡBootconfig��Flash�е�ƫ�Ƶ�ַ�����ж��Ƿ�Ϸ� */
	if (pCfgAddressOffset == NULL){
		printf("[ERROR] Get the bootconfig offset address failed.\n\r");
		return 0;
	}
	sscanf_s(pCfgAddressOffset, "%X", &vFlashConfigOffset);
	if (vFlashConfigOffset > 0){
		if ((vFlashConfigOffset % 4) == 0){
			printf("[OK] The bootconfig offset address is 0x%X.\n\r", vFlashConfigOffset);
		}
		else{
			printf("[WARNING]  The bootconfig offset address is 0x%X. Not align to 4 bytes!!!\n\r", vFlashConfigOffset);
		}
	}
	else{
		printf("[WARNING] The bootconfig offset address is 0x00.\n\r");
	}

	/* ��ȡApplication��Flash�е�ƫ�Ƶ�ַ�����ж��Ƿ�Ϸ� */
	if (pAppAddressOffset == NULL){
		printf("[ERROR] Get the application offset address failed.\n\r");
		return 0;
	}
	sscanf_s(pAppAddressOffset, "%X", &vFlashAppOffset);
	if (vFlashAppOffset > 0){
		if ((vFlashAppOffset % 4) == 0){
			printf("[OK] The application offset address is 0x%X.\n\r", vFlashAppOffset);
		}
		else{
			printf("[WARNING]  The application offset address is 0x%X. Not align to 4 bytes!!!\n\r", vFlashAppOffset);
		}
	}
	else{
		printf("[WARNING] The application offset address is 0x00.\n\r");
	}

	/* ��ֻ����ʽ��Bootloader�ļ� */
	pBootloaderFile = fopen(pBootloaderPath, "rb");
	if (pBootloaderFile == NULL){
		printf("[ERROR] Open the bootloader file failed,please check the path and file name.\n\r");
		return 0;
	}

#if 0
	/* ��ֻ����ʽ��Application�ļ� */
	pAppFile = fopen(pAppPath, "rb");
	if (pAppFile == NULL){
		printf("[ERROR] Open the application file failed,please check the path and file name.\n\r");
		fclose(pBootloaderFile);
		return 0;
	}
#endif

	/* ����Ŀ���ļ� */
	sprintf(ImageName, "./%s_V%s_%04d%02d%02d.bin", pTargetPath, pBootSwVersion,
		curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday);
	pTargetPath = ImageName;
	pTargetFile = fopen(pTargetPath, "wb");
	if (pTargetFile == NULL){
		printf("[ERROR] Create the target file failed,please check the path and file name.\n\r");
		fclose(pBootloaderFile);
		return 0;
	}

	/* ��ȡBootloader�ļ���С */
	fseek(pBootloaderFile, 0, SEEK_END);
	vBootloaderSize = ftell(pBootloaderFile);
	g_bootcfg.boot_size = vBootloaderSize;
	if (vBootloaderSize < 0){
		printf("[ERROR] Bootloader get size failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pTargetFile);
		return 0;
	}
	if (vBootloaderSize > vFlashAppOffset){
		printf("[ERROR] The size of bootloader file is too large.\n\r");
		fclose(pBootloaderFile);
		fclose(pTargetFile);
		return 0;
	}
	printf("[OK] Bootloader file size: %d bytes.\n\r", vBootloaderSize);
#if 0
	/* ��ȡApplication�ļ���С */
	fseek(pAppFile, 0, SEEK_END);
	vAppSize = ftell(pAppFile);
	g_bootcfg.app_size = vAppSize;
	if (vAppSize < 0){
		printf("[ERROR] Application get size failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}
	printf("[OK] Application file size: %d bytes.\n\r", vAppSize);


	/* ����Application CRC32 */
	fseek(pAppFile, 0, SEEK_SET);
	g_app_crc32 = 0x00000000; //����ֵ
	for (int i = 0; i<vAppSize; i += tmp){
		tmp = fread(buf, 1, sizeof(buf), pAppFile);
		if (tmp > 0){
			g_app_crc32 = CRC_CalcBlack(buf, tmp, g_app_crc32);
		}
	}
	g_bootcfg.app_crc32 = g_app_crc32;
#endif
	g_bootcfg.app_crc32 = 0xAA55AA55;


	/* ��Bootloader�ļ�������Target�ļ� */
	fseek(pBootloaderFile, 0, SEEK_SET);
	fseek(pTargetFile, 0, SEEK_SET);
	vTargetSize = 0; //��ʱĿ���ļ���СΪ0
	while (vTargetSize < vBootloaderSize){
		vReadLen = fread(buf, 1, sizeof(buf), pBootloaderFile);
		if (vReadLen > 0){
			vWriteLen = fwrite(buf, 1, vReadLen, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ���Bootloader�ļ��Ƿ��Ѿ���ȫ������Target�ļ��� */
	if (vTargetSize != vBootloaderSize){
		printf("[ERROR] Copy Bootloader file to Target file failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pTargetFile);
		return 0;
	}

	/* Bootloader�����沿��ȫ��д0xFF */
	if (vTargetSize < vFlashConfigOffset){
		vBlocks = (vFlashConfigOffset - vTargetSize) / sizeof(buf);
		vBytes = (vFlashConfigOffset - vTargetSize) % sizeof(buf);
		memset(buf, 0xFF, sizeof(buf));
		while (vBlocks--){
			vWriteLen = fwrite(buf, 1, sizeof(buf), pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pTargetFile);
				return 0;
			}
		}
		if (vBytes){
			vWriteLen = fwrite(buf, 1, vBytes, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* д��Bootconfig������ */
	vWriteLen = fwrite((void *)&g_bootcfg, 1, sizeof(boot_config_t), pTargetFile);
	vTargetSize += vWriteLen;
	if (vWriteLen == -1){
		printf("[ERROR] Write data to file failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pTargetFile);
		return 0;
	}

	/* Bootconfig�����沿��ȫ��д0xFF */
	if (vTargetSize < vFlashAppOffset){
		vBlocks = (vFlashAppOffset - vTargetSize) / sizeof(buf);
		vBytes = (vFlashAppOffset - vTargetSize) % sizeof(buf);
		memset(buf, 0xFF, sizeof(buf));
		while (vBlocks--){
			vWriteLen = fwrite(buf, 1, sizeof(buf), pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pTargetFile);
				return 0;
			}
		}
		if (vBytes){
			vWriteLen = fwrite(buf, 1, vBytes, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}
#if 0
	/* ��Application�ļ�������Target�ļ� */
	fseek(pAppFile, 0, SEEK_SET);
	while (vTargetSize < (vFlashAppOffset + vAppSize)){
		vReadLen = fread(buf, 1, sizeof(buf), pAppFile);
		if (vReadLen > 0){
			vWriteLen = fwrite(buf, 1, vReadLen, pTargetFile);
			vTargetSize += vWriteLen;
			if (vWriteLen == -1){
				printf("[ERROR] Write data to file failed.\n\r");
				fclose(pBootloaderFile);
				fclose(pAppFile);
				fclose(pTargetFile);
				return 0;
			}
		}
	}

	/* ���Application�ļ��Ƿ��Ѿ���ȫ������Target�ļ��� */
	if (vTargetSize != (vFlashAppOffset + vAppSize)){
		printf("[ERROR] Copy application file to Target file failed.\n\r");
		fclose(pBootloaderFile);
		fclose(pAppFile);
		fclose(pTargetFile);
		return 0;
	}
#endif

	/* �ɹ�����Target�ļ� */
	printf("[OK] Target file size: %d bytes.\n\r", vTargetSize);
	printf("[OK] Create target file: %s\n\r\n\r", pTargetPath);
	printf("\n\rCreate IMAGE file successfully!!!\n\r\n\r");

	if (pBootloaderFile != NULL){
		fclose(pBootloaderFile);
	}
	if (pTargetFile != NULL){
		fclose(pTargetFile);
	}

	return 0;
}


#if 0
int _tmain(int argc, char *argv[])
{
	time_t now;
	struct tm curTime;

	/*��ȡϵͳʱ����Ϣ*/
	time(&now);
	localtime_s(&curTime, &now);

	char *path = argv[1];

	printf("%d\n", argc);
	printf("%s\n", argv[0]);
	printf("%s\n", path);
	FILE *pF;

		pF = fopen("./test/test.bin", "rb");

		if (pF == NULL)
		{
			printf("File is NULL!\n");
		}

	return 0;
}
#endif


