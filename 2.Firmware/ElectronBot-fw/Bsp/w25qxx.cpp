#include <stdio.h>
#include "w25qxx.h"

#define USE_OS_TOOLS
#define W25QXX_DUMMY_BYTE 0xA5

#ifdef USE_OS_TOOLS
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#ifdef USE_OS_TOOLS

static SemaphoreHandle_t sema_flash_spi;
static int32_t sema_flash_spi_init()
{
    sema_flash_spi = xSemaphoreCreateBinary();
    if(sema_flash_spi == NULL)
    {
        return -1;
    }
    return 0;
}
static void sema_flash_spi_wait()
{
    if (sema_flash_spi != NULL)
    {
        xSemaphoreTake(sema_flash_spi, portMAX_DELAY);
    }
}
static void sema_flash_spi_give()
{
    if (sema_flash_spi != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(sema_flash_spi, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
extern "C" void flash_spi_tx_done()
{
    sema_flash_spi_give();
}
#endif

uint8_t Flash::W25qxx_Spi(uint8_t Data) {
    uint8_t ret;
    HAL_SPI_TransmitReceive(spi, &Data, &ret, 1, 100);
    return ret;
}

uint32_t Flash::W25qxx_ReadID(void) {
    uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    W25qxx_Spi(0x9F);
    Temp0 = W25qxx_Spi(W25QXX_DUMMY_BYTE);
    Temp1 = W25qxx_Spi(W25QXX_DUMMY_BYTE);
    Temp2 = W25qxx_Spi(W25QXX_DUMMY_BYTE);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
    return Temp;
}

void Flash::W25qxx_ReadUniqID(void) {
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    W25qxx_Spi(0x4B);
    for (uint8_t i = 0; i < 4; i++)
        W25qxx_Spi(W25QXX_DUMMY_BYTE);
    for (uint8_t i = 0; i < 8; i++)
        w25qxx.UniqID[i] = W25qxx_Spi(W25QXX_DUMMY_BYTE);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
}

void Flash::W25qxx_WriteEnable(void) {
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    W25qxx_Spi(0x06);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    HAL_Delay(1);
}

void Flash::W25qxx_WriteDisable(void) {
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    W25qxx_Spi(0x04);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    HAL_Delay(1);
}

uint8_t Flash::W25qxx_ReadStatusRegister(uint8_t SelectStatusRegister_1_2_3) {
    uint8_t status = 0;
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (SelectStatusRegister_1_2_3 == 1) {
        W25qxx_Spi(0x05);
        status = W25qxx_Spi(W25QXX_DUMMY_BYTE);
        w25qxx.StatusRegister1 = status;
    } else if (SelectStatusRegister_1_2_3 == 2) {
        W25qxx_Spi(0x35);
        status = W25qxx_Spi(W25QXX_DUMMY_BYTE);
        w25qxx.StatusRegister2 = status;
    } else {
        W25qxx_Spi(0x15);
        status = W25qxx_Spi(W25QXX_DUMMY_BYTE);
        w25qxx.StatusRegister3 = status;
    }
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    return status;
}

void Flash::W25qxx_WriteStatusRegister(uint8_t SelectStatusRegister_1_2_3, uint8_t Data) {
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (SelectStatusRegister_1_2_3 == 1) {
        W25qxx_Spi(0x01);
        w25qxx.StatusRegister1 = Data;
    } else if (SelectStatusRegister_1_2_3 == 2) {
        W25qxx_Spi(0x31);
        w25qxx.StatusRegister2 = Data;
    } else {
        W25qxx_Spi(0x11);
        w25qxx.StatusRegister3 = Data;
    }
    W25qxx_Spi(Data);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
}

void Flash::W25qxx_WaitForWriteEnd(void) {
    HAL_Delay(1);
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    W25qxx_Spi(0x05);
    do
    {
        w25qxx.StatusRegister1 = W25qxx_Spi(W25QXX_DUMMY_BYTE);
        HAL_Delay(1);
    } while ((w25qxx.StatusRegister1 & 0x01) == 0x01);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
}

bool Flash::Init() {

#ifdef USE_OS_TOOLS
    sema_flash_spi_init();
#endif

    w25qxx.Lock = 1;
    while (HAL_GetTick() < 100) {
        HAL_Delay(1);
    }
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    HAL_Delay(1);
    uint32_t id = W25qxx_ReadID();
    switch (id & 0x000000FF)
    {
        case 0x20: // 	w25q512
            w25qxx.ID = W25Q512;
            w25qxx.BlockCount = 1024;
            printf("w25qxx Chip: w25q512\r\n");
            break;
        case 0x19: // 	w25q256
            w25qxx.ID = W25Q256;
            w25qxx.BlockCount = 512;
            printf("w25qxx Chip: w25q256\r\n");
            break;
        case 0x18: // 	w25q128
            w25qxx.ID = W25Q128;
            w25qxx.BlockCount = 256;
            printf("w25qxx Chip: w25q128\r\n");
            break;
        case 0x17: //	w25q64
            w25qxx.ID = W25Q64;
            w25qxx.BlockCount = 128;
            printf("w25qxx Chip: w25q64\r\n");
            break;
        case 0x16: //	w25q32
            w25qxx.ID = W25Q32;
            w25qxx.BlockCount = 64;
            printf("w25qxx Chip: w25q32\r\n");
            break;
        case 0x15: //	w25q16
            w25qxx.ID = W25Q16;
            w25qxx.BlockCount = 32;
            printf("w25qxx Chip: w25q16\r\n");
            break;
        case 0x14: //	w25q80
            w25qxx.ID = W25Q80;
            w25qxx.BlockCount = 16;
            printf("w25qxx Chip: w25q80\r\n");
            break;
        case 0x13: //	w25q40
            w25qxx.ID = W25Q40;
            w25qxx.BlockCount = 8;
            printf("w25qxx Chip: w25q40\r\n");
            break;
        case 0x12: //	w25q20
            w25qxx.ID = W25Q20;
            w25qxx.BlockCount = 4;
            printf("w25qxx Chip: w25q20\r\n");
            break;
        case 0x11: //	w25q10
            w25qxx.ID = W25Q10;
            w25qxx.BlockCount = 2;
            printf("w25qxx Chip: w25q10\r\n");
            break;
        default:
            w25qxx.Lock = 0;
            printf("w25qxx Unknown ID\r\n");
            return false;
    }
    w25qxx.PageSize = 256;
    w25qxx.SectorSize = 0x1000;
    w25qxx.SectorCount = w25qxx.BlockCount * 16;
    w25qxx.PageCount = (w25qxx.SectorCount * w25qxx.SectorSize) / w25qxx.PageSize;
    w25qxx.BlockSize = w25qxx.SectorSize * 16;
    w25qxx.CapacityInKiloByte = (w25qxx.SectorCount * w25qxx.SectorSize) / 1024;
    W25qxx_ReadUniqID();
    W25qxx_ReadStatusRegister(1);
    W25qxx_ReadStatusRegister(2);
    W25qxx_ReadStatusRegister(3);

    printf("w25qxx Page Size: %d Bytes\r\n", w25qxx.PageSize);
    printf("w25qxx Page Count: %d\r\n", w25qxx.PageCount);
    printf("w25qxx Sector Size: %d Bytes\r\n", w25qxx.SectorSize);
    printf("w25qxx Sector Count: %d\r\n", w25qxx.SectorCount);
    printf("w25qxx Block Size: %d Bytes\r\n", w25qxx.BlockSize);
    printf("w25qxx Block Count: %d\r\n", w25qxx.BlockCount);
    printf("w25qxx Capacity: %d KiloBytes\r\n", w25qxx.CapacityInKiloByte);
    printf("w25qxx Init Done\r\n");

    w25qxx.Lock = 0;
    return true;
}

void Flash::W25qxx_EraseChip(void) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    W25qxx_WriteEnable();
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    W25qxx_Spi(0xC7);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    W25qxx_WaitForWriteEnd();
    HAL_Delay(10);
    w25qxx.Lock = 0;
}

void Flash::W25qxx_EraseSector(uint32_t SectorAddr) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    W25qxx_WaitForWriteEnd();
    SectorAddr = SectorAddr * w25qxx.SectorSize;
    W25qxx_WriteEnable();
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0x21);
        W25qxx_Spi((SectorAddr & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0x20);
    }
    W25qxx_Spi((SectorAddr & 0xFF0000) >> 16);
    W25qxx_Spi((SectorAddr & 0xFF00) >> 8);
    W25qxx_Spi(SectorAddr & 0xFF);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    W25qxx_WaitForWriteEnd();
    HAL_Delay(1);
    w25qxx.Lock = 0;
}

void Flash::W25qxx_EraseBlock(uint32_t BlockAddr) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    W25qxx_WaitForWriteEnd();
    BlockAddr = BlockAddr * w25qxx.SectorSize * 16;
    W25qxx_WriteEnable();
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0xDC);
        W25qxx_Spi((BlockAddr & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0xD8);
    }
    W25qxx_Spi((BlockAddr & 0xFF0000) >> 16);
    W25qxx_Spi((BlockAddr & 0xFF00) >> 8);
    W25qxx_Spi(BlockAddr & 0xFF);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    W25qxx_WaitForWriteEnd();
    HAL_Delay(1);
    w25qxx.Lock = 0;
}

uint32_t Flash::W25qxx_PageToSector(uint32_t PageAddress) {
    return ((PageAddress * w25qxx.PageSize) / w25qxx.SectorSize);
}

uint32_t Flash::W25qxx_PageToBlock(uint32_t PageAddress) {
    return ((PageAddress * w25qxx.PageSize) / w25qxx.BlockSize);
}

uint32_t Flash::W25qxx_SectorToBlock(uint32_t SectorAddress) {
    return ((SectorAddress * w25qxx.SectorSize) / w25qxx.BlockSize);
}

uint32_t Flash::W25qxx_SectorToPage(uint32_t SectorAddress) {
    return (SectorAddress * w25qxx.SectorSize) / w25qxx.PageSize;
}

uint32_t Flash::W25qxx_BlockToPage(uint32_t BlockAddress) {
    return (BlockAddress * w25qxx.BlockSize) / w25qxx.PageSize;
}

bool Flash::W25qxx_IsEmptyPage(uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_PageSize) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    if (((NumByteToCheck_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) || (NumByteToCheck_up_to_PageSize == 0)) {
        NumByteToCheck_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
    }

    uint8_t pBuffer[32];
    uint32_t WorkAddress;
    uint32_t i;
    for (i = OffsetInByte; i < w25qxx.PageSize; i += sizeof(pBuffer)) {
        SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
        WorkAddress = (i + Page_Address * w25qxx.PageSize);
        if (w25qxx.ID >= W25Q256) {
            W25qxx_Spi(0x0C);
            W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
        } else {
            W25qxx_Spi(0x0B);
        }
        W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
        W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
        W25qxx_Spi(WorkAddress & 0xFF);
        W25qxx_Spi(0);
        HAL_SPI_Receive(spi, pBuffer, sizeof(pBuffer), 100);
        SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
        for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
            if (pBuffer[x] != 0xFF) {
                goto NOT_EMPTY;
            }
        }
    }
    if ((w25qxx.PageSize + OffsetInByte) % sizeof(pBuffer) != 0) {
        i -= sizeof(pBuffer);
        for (; i < w25qxx.PageSize; i++) {
            SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
            WorkAddress = (i + Page_Address * w25qxx.PageSize);
            W25qxx_Spi(0x0B);
            if (w25qxx.ID >= W25Q256) {
                W25qxx_Spi(0x0C);
                W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
            } else {
                W25qxx_Spi(0x0B);
            }
            W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
            W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
            W25qxx_Spi(WorkAddress & 0xFF);
            W25qxx_Spi(0);
            HAL_SPI_Receive(spi, pBuffer, 1, 100);
            SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
            if (pBuffer[0] != 0xFF) {
                goto NOT_EMPTY;
            }
        }
    }
    w25qxx.Lock = 0;
    return true;
    NOT_EMPTY:
        w25qxx.Lock = 0;
        return false;
}

bool Flash::W25qxx_IsEmptySector(uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_SectorSize) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    if ((NumByteToCheck_up_to_SectorSize > w25qxx.SectorSize) || (NumByteToCheck_up_to_SectorSize == 0)) {
        NumByteToCheck_up_to_SectorSize = w25qxx.SectorSize;
    }

    uint8_t pBuffer[32];
    uint32_t WorkAddress;
    uint32_t i;
    for (i = OffsetInByte; i < w25qxx.SectorSize; i += sizeof(pBuffer)) {
        SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
        WorkAddress = (i + Sector_Address * w25qxx.SectorSize);
        if (w25qxx.ID >= W25Q256) {
            W25qxx_Spi(0x0C);
            W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
        } else {
            W25qxx_Spi(0x0B);
        }
        W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
        W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
        W25qxx_Spi(WorkAddress & 0xFF);
        W25qxx_Spi(0);
        HAL_SPI_Receive(spi, pBuffer, sizeof(pBuffer), 100);
        SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
        for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
            if (pBuffer[x] != 0xFF) {
                goto NOT_EMPTY;
            }
        }
    }
    if ((w25qxx.SectorSize + OffsetInByte) % sizeof(pBuffer) != 0) {
        i -= sizeof(pBuffer);
        for (; i < w25qxx.SectorSize; i++) {
            SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
            WorkAddress = (i + Sector_Address * w25qxx.SectorSize);
            if (w25qxx.ID >= W25Q256) {
                W25qxx_Spi(0x0C);
                W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
            } else {
                W25qxx_Spi(0x0B);
            }
            W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
            W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
            W25qxx_Spi(WorkAddress & 0xFF);
            W25qxx_Spi(0);
            HAL_SPI_Receive(spi, pBuffer, 1, 100);
            SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
            if (pBuffer[0] != 0xFF) {
                goto NOT_EMPTY;
            }
        }
    }
    w25qxx.Lock = 0;
    return true;

    NOT_EMPTY:
    w25qxx.Lock = 0;
    return false;
}

bool Flash::W25qxx_IsEmptyBlock(uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_BlockSize) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    if ((NumByteToCheck_up_to_BlockSize > w25qxx.BlockSize) || (NumByteToCheck_up_to_BlockSize == 0)) {
        NumByteToCheck_up_to_BlockSize = w25qxx.BlockSize;
    }
    uint8_t pBuffer[32];
    uint32_t WorkAddress;
    uint32_t i;
    for (i = OffsetInByte; i < w25qxx.BlockSize; i += sizeof(pBuffer)) {
        SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
        WorkAddress = (i + Block_Address * w25qxx.BlockSize);
        if (w25qxx.ID >= W25Q256) {
            W25qxx_Spi(0x0C);
            W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
        } else {
            W25qxx_Spi(0x0B);
        }
        W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
        W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
        W25qxx_Spi(WorkAddress & 0xFF);
        W25qxx_Spi(0);
        HAL_SPI_Receive(spi, pBuffer, sizeof(pBuffer), 100);
        SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
        for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
            if (pBuffer[x] != 0xFF) {
                goto NOT_EMPTY;
            }
        }
    }
    if ((w25qxx.BlockSize + OffsetInByte) % sizeof(pBuffer) != 0) {
        i -= sizeof(pBuffer);
        for (; i < w25qxx.BlockSize; i++) {
            SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
            WorkAddress = (i + Block_Address * w25qxx.BlockSize);
            if (w25qxx.ID >= W25Q256) {
                W25qxx_Spi(0x0C);
                W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
            } else {
                W25qxx_Spi(0x0B);
            }
            W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
            W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
            W25qxx_Spi(WorkAddress & 0xFF);
            W25qxx_Spi(0);
            HAL_SPI_Receive(spi, pBuffer, 1, 100);
            SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
            if (pBuffer[0] != 0xFF) {
                goto NOT_EMPTY;
            }
        }
    }
    w25qxx.Lock = 0;
    return true;
    NOT_EMPTY:
    w25qxx.Lock = 0;
    return false;
}

void Flash::W25qxx_WriteByte(uint8_t pBuffer, uint32_t WriteAddr_inBytes) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    W25qxx_WaitForWriteEnd();
    W25qxx_WriteEnable();
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0x12);
        W25qxx_Spi((WriteAddr_inBytes & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0x02);
    }
    W25qxx_Spi((WriteAddr_inBytes & 0xFF0000) >> 16);
    W25qxx_Spi((WriteAddr_inBytes & 0xFF00) >> 8);
    W25qxx_Spi(WriteAddr_inBytes & 0xFF);
    W25qxx_Spi(pBuffer);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    W25qxx_WaitForWriteEnd();
    w25qxx.Lock = 0;
}

void Flash::W25qxx_WritePage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_PageSize) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    if (((NumByteToWrite_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) || (NumByteToWrite_up_to_PageSize == 0)) {
        NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
    }
    W25qxx_WaitForWriteEnd();
    W25qxx_WriteEnable();
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    Page_Address = (Page_Address * w25qxx.PageSize) + OffsetInByte;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0x12);
        W25qxx_Spi((Page_Address & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0x02);
    }
    W25qxx_Spi((Page_Address & 0xFF0000) >> 16);
    W25qxx_Spi((Page_Address & 0xFF00) >> 8);
    W25qxx_Spi(Page_Address & 0xFF);
    HAL_SPI_Transmit(spi, pBuffer, NumByteToWrite_up_to_PageSize, 100);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    W25qxx_WaitForWriteEnd();
    HAL_Delay(1);
    w25qxx.Lock = 0;
}

void Flash::W25qxx_WriteSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_SectorSize) {
    if ((NumByteToWrite_up_to_SectorSize > w25qxx.SectorSize) || (NumByteToWrite_up_to_SectorSize == 0)) {
        NumByteToWrite_up_to_SectorSize = w25qxx.SectorSize;
    }
    if (OffsetInByte >= w25qxx.SectorSize) {
        return;
    }
    uint32_t StartPage;
    int32_t BytesToWrite;
    uint32_t LocalOffset;
    if ((OffsetInByte + NumByteToWrite_up_to_SectorSize) > w25qxx.SectorSize) {
        BytesToWrite = w25qxx.SectorSize - OffsetInByte;
    } else {
        BytesToWrite = NumByteToWrite_up_to_SectorSize;
    }
    StartPage = W25qxx_SectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_WritePage(pBuffer, StartPage, LocalOffset, BytesToWrite);
        StartPage++;
        BytesToWrite -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize - LocalOffset;
        LocalOffset = 0;
    } while (BytesToWrite > 0);
}

void Flash::W25qxx_WriteBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_BlockSize) {
    if ((NumByteToWrite_up_to_BlockSize > w25qxx.BlockSize) || (NumByteToWrite_up_to_BlockSize == 0)) {
        NumByteToWrite_up_to_BlockSize = w25qxx.BlockSize;
    }
    if (OffsetInByte >= w25qxx.BlockSize) {
        return;
    }
    uint32_t StartPage;
    int32_t BytesToWrite;
    uint32_t LocalOffset;
    if ((OffsetInByte + NumByteToWrite_up_to_BlockSize) > w25qxx.BlockSize) {
        BytesToWrite = w25qxx.BlockSize - OffsetInByte;
    } else {
        BytesToWrite = NumByteToWrite_up_to_BlockSize;
    }
    StartPage = W25qxx_BlockToPage(Block_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_WritePage(pBuffer, StartPage, LocalOffset, BytesToWrite);
        StartPage++;
        BytesToWrite -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize - LocalOffset;
        LocalOffset = 0;
    } while (BytesToWrite > 0);
}

void Flash::W25qxx_ReadByte(uint8_t *pBuffer, uint32_t Bytes_Address) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0x0C);
        W25qxx_Spi((Bytes_Address & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0x0B);
    }
    W25qxx_Spi((Bytes_Address & 0xFF0000) >> 16);
    W25qxx_Spi((Bytes_Address & 0xFF00) >> 8);
    W25qxx_Spi(Bytes_Address & 0xFF);
    W25qxx_Spi(0);
    *pBuffer = W25qxx_Spi(W25QXX_DUMMY_BYTE);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    w25qxx.Lock = 0;
}

void Flash::W25qxx_ReadBytes(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0x0C);
        W25qxx_Spi((ReadAddr & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0x0B);
    }
    W25qxx_Spi((ReadAddr & 0xFF0000) >> 16);
    W25qxx_Spi((ReadAddr & 0xFF00) >> 8);
    W25qxx_Spi(ReadAddr & 0xFF);
    W25qxx_Spi(0);
    HAL_SPI_Receive(spi, pBuffer, NumByteToRead, 2000);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    HAL_Delay(1);
    w25qxx.Lock = 0;
}

void Flash::W25qxx_ReadPage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_PageSize) {
    while (w25qxx.Lock == 1) {
        HAL_Delay(1);
    }
    w25qxx.Lock = 1;
    if ((NumByteToRead_up_to_PageSize > w25qxx.PageSize) || (NumByteToRead_up_to_PageSize == 0)) {
        NumByteToRead_up_to_PageSize = w25qxx.PageSize;
    }
    if ((OffsetInByte + NumByteToRead_up_to_PageSize) > w25qxx.PageSize) {
        NumByteToRead_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
    }
    Page_Address = Page_Address * w25qxx.PageSize + OffsetInByte;
    SPI1_CS_GPIO_Port->BSRR = (uint32_t) SPI1_CS_Pin << 16U;
    if (w25qxx.ID >= W25Q256) {
        W25qxx_Spi(0x0C);
        W25qxx_Spi((Page_Address & 0xFF000000) >> 24);
    } else {
        W25qxx_Spi(0x0B);
    }
    W25qxx_Spi((Page_Address & 0xFF0000) >> 16);
    W25qxx_Spi((Page_Address & 0xFF00) >> 8);
    W25qxx_Spi(Page_Address & 0xFF);
    W25qxx_Spi(0);
    HAL_SPI_Receive(spi, pBuffer, NumByteToRead_up_to_PageSize, 100);
    SPI1_CS_GPIO_Port->BSRR = SPI1_CS_Pin;
    HAL_Delay(1);
    w25qxx.Lock = 0;
}

void Flash::W25qxx_ReadSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_SectorSize) {
    if ((NumByteToRead_up_to_SectorSize > w25qxx.SectorSize) || (NumByteToRead_up_to_SectorSize == 0)) {
        NumByteToRead_up_to_SectorSize = w25qxx.SectorSize;
    }
    if (OffsetInByte >= w25qxx.SectorSize) {
        return;
    }
    uint32_t StartPage;
    int32_t BytesToRead;
    uint32_t LocalOffset;
    if ((OffsetInByte + NumByteToRead_up_to_SectorSize) > w25qxx.SectorSize) {
        BytesToRead = w25qxx.SectorSize - OffsetInByte;
    } else {
        BytesToRead = NumByteToRead_up_to_SectorSize;
    }
    StartPage = W25qxx_SectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_ReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
        StartPage++;
        BytesToRead -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize - LocalOffset;
        LocalOffset = 0;
    } while (BytesToRead > 0);
}

void Flash::W25qxx_ReadBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_BlockSize) {
    if ((NumByteToRead_up_to_BlockSize > w25qxx.BlockSize) || (NumByteToRead_up_to_BlockSize == 0)) {
        NumByteToRead_up_to_BlockSize = w25qxx.BlockSize;
    }
    if (OffsetInByte >= w25qxx.BlockSize) {
        return;
    }
    uint32_t StartPage;
    int32_t BytesToRead;
    uint32_t LocalOffset;
    if ((OffsetInByte + NumByteToRead_up_to_BlockSize) > w25qxx.BlockSize) {
        BytesToRead = w25qxx.BlockSize - OffsetInByte;
    } else {
        BytesToRead = NumByteToRead_up_to_BlockSize;
    }
    StartPage = W25qxx_BlockToPage(Block_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_ReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
        StartPage++;
        BytesToRead -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize - LocalOffset;
        LocalOffset = 0;
    } while (BytesToRead > 0);
}


