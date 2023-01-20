#include "robot.h"
#include "usbd_cdc_if.h"

uint8_t* Robot::GetPingPongBufferPtr() {
    return &(usbBuffer.rxData[usbBuffer.pingPongIndex][usbBuffer.rxDataOffset]);
}

void Robot::SendUsbPacket(uint8_t* _data, uint32_t _len) {
    uint8_t ret;
    do{
        ret = CDC_Transmit_HS(_data, _len);
    } while (ret != USBD_OK);
}

void Robot::SwitchPingPongBuffer() {
    usbBuffer.pingPongIndex = (usbBuffer.pingPongIndex == 0 ? 1 : 0);
    usbBuffer.rxDataOffset = 0;
}

uint8_t* Robot::GetLcdBufferPtr() {
    return usbBuffer.rxData[usbBuffer.pingPongIndex == 0 ? 1 : 0];
}

void Robot::ReceiveUsbPacketUntilSizeIs(uint32_t _count) {
    while (usbBuffer.receivedPacketLen != _count);
    usbBuffer.receivedPacketLen = 0;
}

uint8_t* Robot::GetExtraDataRxPtr() {
    return GetLcdBufferPtr() + 60 * 240 * 3;
}
