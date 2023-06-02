#ifndef ELECTRONBOT_FW_W25QXX_H
#define ELECTRONBOT_FW_W25QXX_H


//————————————————————————————————————————————————————————————————————————————————————————
// W25Qxx系列中的xx代表容量，单位为Mb。
//W25Qxx系列的Flash内部是按照Page(页)、Sector（扇区）、Block（块）的结构来划分的。
//每页（Page）为256个字节，
//每个扇区（Sector）有16页（Page）也就是4KB，
//每个块（Block）有16个扇区（Sector）也就是64KB,  64 * 1024 * 1024
//W25Qxx最小擦除单位为一个扇区（Sector），也就是每次至少擦除4KB。
//最大写入单位是一页（Page），也就是一次最多写入256个字节。
//————————————————————————————————————————————————————————————————————————————————————————


#include "stm32f4xx.h"
#include "spi.h"

class Flash {

public:
    explicit Flash(SPI_HandleTypeDef* _spi) : spi(_spi){

}
    typedef enum W25QXX_ID_t
    {
        W25Q10 = 1,
        W25Q20,
        W25Q40,
        W25Q80,
        W25Q16,
        W25Q32,
        W25Q64,
        W25Q128,
        W25Q256,
        W25Q512,
    } W25QXX_ID_t;
    typedef struct w25qxx_t
    {
        W25QXX_ID_t ID;
        uint8_t UniqID[8];
        uint16_t PageSize;
        uint32_t PageCount;
        uint32_t SectorSize;
        uint32_t SectorCount;
        uint32_t BlockSize;
        uint32_t BlockCount;
        uint32_t CapacityInKiloByte;
        uint8_t StatusRegister1;
        uint8_t StatusRegister2;
        uint8_t StatusRegister3;
        uint8_t Lock;
    } w25qxx_t;


    w25qxx_t w25qxx;

    bool Init(void);
    void W25qxx_EraseChip(void);
    void W25qxx_EraseSector(uint32_t SectorAddr);
    void W25qxx_EraseBlock(uint32_t BlockAddr);
    uint32_t W25qxx_PageToSector(uint32_t PageAddress);
    uint32_t W25qxx_PageToBlock(uint32_t PageAddress);
    uint32_t W25qxx_SectorToBlock(uint32_t SectorAddress);
    uint32_t W25qxx_SectorToPage(uint32_t SectorAddress);
    uint32_t W25qxx_BlockToPage(uint32_t BlockAddress);

    bool W25qxx_IsEmptyPage(uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_PageSize);
    bool W25qxx_IsEmptySector(uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_SectorSize);
    bool W25qxx_IsEmptyBlock(uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_BlockSize);

    void W25qxx_WriteByte(uint8_t pBuffer, uint32_t WriteAddr_inBytes);
    void W25qxx_WritePage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_PageSize);
    void W25qxx_WriteSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_SectorSize);
    void W25qxx_WriteBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_BlockSize);
    void W25qxx_ReadByte(uint8_t *pBuffer, uint32_t Bytes_Address);
    void W25qxx_ReadBytes(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
    void W25qxx_ReadPage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_PageSize);
    void W25qxx_ReadSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_SectorSize);
    void W25qxx_ReadBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_BlockSize);

private:
    uint8_t W25qxx_Spi(uint8_t Data);
    uint32_t W25qxx_ReadID(void);
    void W25qxx_ReadUniqID(void);
    void W25qxx_WriteEnable(void);
    void W25qxx_WriteDisable(void);
    uint8_t W25qxx_ReadStatusRegister(uint8_t SelectStatusRegister_1_2_3);
    void W25qxx_WriteStatusRegister(uint8_t SelectStatusRegister_1_2_3, uint8_t Data);
    void W25qxx_WaitForWriteEnd(void);
    SPI_HandleTypeDef* spi;

};

#endif //ELECTRONBOT_FW_W25QXX_H
