#include "electron_low_level.h"

using namespace cv;


int main()
{
    ElectronLowLevel robot;

    if (robot.Connect())
        printf("Robot connected!\n");
    else
    {
        printf("Connect failed!\n");
        getchar();
        return 0;
    }

    VideoCapture video("happy.mp4");
    Mat frame;


    while (true)
    {
        video >> frame;
        if (frame.empty())
            break;
        Mat src = imread("C:\\Users\\admin\\Desktop\\1.jpg"); /*图片 */
        robot.SetImageSrc(src);
        robot.Sync();
    }

    robot.Disconnect();
    printf("File play finished, robot Disconnected!\n");
    printf("Press [Enter] to exit.\n");

    getchar();
    return 0;
}

