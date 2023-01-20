#ifndef ELECTRONBOT_FW_ROBOT_H
#define ELECTRONBOT_FW_ROBOT_H

#include "stm32f4xx.h"
#include "screen.h"
#include "servo.h"
#include "audio.h"


class Robot {
    public: explicit Robot(SPI_HandleTypeDef* _spi, I2S_HandleTypeDef* _i2s, I2C_HandleTypeDef* _i2c) :
    spi(_spi), i2s(_i2s), i2c(_i2c)
    {
        lcd = new Screen(spi);
        servo = new Servo(i2c);
        audio = new Audio(i2s, i2c);
	}

    struct UsbBuffer_t
    {
        uint8_t extraDataTx[32];
        uint8_t rxData[2][60 * 240 * 3 + 32]; // 43232bytes, 43200 of which are lcd buffer
        volatile uint16_t receivedPacketLen = 0;
        volatile uint8_t pingPongIndex = 0;
        volatile uint32_t rxDataOffset = 0;
    };
    UsbBuffer_t usbBuffer;


    uint8_t* GetPingPongBufferPtr();
    uint8_t* GetLcdBufferPtr();
    uint8_t* GetExtraDataRxPtr();
    void SwitchPingPongBuffer();
    void SendUsbPacket(uint8_t* _data, uint32_t _len);
    void ReceiveUsbPacketUntilSizeIs(uint32_t _count);

    Screen* lcd;
    Servo* servo;
    Audio* audio;

private:

    SPI_HandleTypeDef* spi;
	I2S_HandleTypeDef* i2s;
	I2C_HandleTypeDef* i2c;
};

#endif
