#include <cmath>
#include "common_inc.h"
#include "screen.h"
#include "robot.h"


Robot robot(&hspi1, &hi2s3, &hi2c1, &hi2c3);
float jointSetPoints[6];
bool isEnabled = false;
/* Thread Definitions -----------------------------------------------------*/


/* Timer Callbacks -------------------------------------------------------*/

/* Default Entry -------------------------------------------------------*/
void Main(void)
{
//    while (true) {
//        printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
//               jointSetPoints[0], jointSetPoints[1], jointSetPoints[2],
//               jointSetPoints[3], jointSetPoints[4], jointSetPoints[5]);
//    }
    HAL_Delay(2000);
    robot.lcd->Init(Screen::DEGREE_0);
    robot.lcd->SetWindow(0, 239, 0, 239);

    robot.servo->UpdateJointAngle(robot.servo->joint[1], 0);
    robot.servo->UpdateJointAngle(robot.servo->joint[2], 0);
    robot.servo->UpdateJointAngle(robot.servo->joint[3], 0);
    robot.servo->UpdateJointAngle(robot.servo->joint[4], 0);
    robot.servo->UpdateJointAngle(robot.servo->joint[5], 0);
    robot.servo->UpdateJointAngle(robot.servo->joint[6], 0);


    float t = 0;
    while (true) {
#if 1
        for (int p = 0; p < 4; p++) {
            // send joint angles
            for (int j = 0; j < 6; j++) {
                for (int i = 0; i < 4; i++) {
                    auto *b = (unsigned char *) &(robot.servo->joint[j + 1].angle);
                    robot.usbBuffer.extraDataTx[j * 4 + i + 1] = *(b + i);
                }
            }
            robot.SendUsbPacket(robot.usbBuffer.extraDataTx, 32);
            robot.ReceiveUsbPacketUntilSizeIs(224); // last packet is 224bytes
            // get joint angles
            uint8_t *ptr = robot.GetExtraDataRxPtr();
            if (!isEnabled && ptr[0] == 1) {
                isEnabled = true;
                robot.servo->SetJointEnable(robot.servo->joint[1], true);
                robot.servo->SetJointEnable(robot.servo->joint[2], true);
                robot.servo->SetJointEnable(robot.servo->joint[3], true);
                robot.servo->SetJointEnable(robot.servo->joint[4], true);
                robot.servo->SetJointEnable(robot.servo->joint[5], true);
                robot.servo->SetJointEnable(robot.servo->joint[6], true);
            }

            for (int j = 0; j < 6; j++) {
                jointSetPoints[j] = *((float *) (ptr + 4 * j + 1));
            }

            while (robot.lcd->isBusy);
            if (p == 0) {
                robot.lcd->WriteFrameBuffer(robot.GetLcdBufferPtr(), 60 * 240 * 3);
            } else {
                robot.lcd->WriteFrameBuffer(robot.GetLcdBufferPtr(), 60 * 240 * 3, true);
            }
        }
        HAL_Delay(1);
#endif


        t += 0.01f;

        robot.servo->UpdateJointAngle(robot.servo->joint[1], jointSetPoints[0]);
        robot.servo->UpdateJointAngle(robot.servo->joint[2], jointSetPoints[1]);
        robot.servo->UpdateJointAngle(robot.servo->joint[3], jointSetPoints[2]);
        robot.servo->UpdateJointAngle(robot.servo->joint[4], jointSetPoints[3]);
        robot.servo->UpdateJointAngle(robot.servo->joint[5], jointSetPoints[4]);
        robot.servo->UpdateJointAngle(robot.servo->joint[6], jointSetPoints[5]);

        HAL_Delay(1);

#if 1
        printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
               jointSetPoints[0], jointSetPoints[1], jointSetPoints[2],
               jointSetPoints[3], jointSetPoints[4], jointSetPoints[5]);
#endif
    }
}

extern "C"
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
    robot.lcd->isBusy = false;
}

