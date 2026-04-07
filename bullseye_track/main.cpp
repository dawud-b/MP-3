
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <fstream>
#include <vector>


uint16_t framebuffer[1920*1080];

uint8_t downscaled_gray_img[480*270];

void update_framebuffer() {
    std::ifstream file("../images/BIMG_000.BIN", std::ios::binary);
    if (!file) return;

    file.read(reinterpret_cast<char*>(framebuffer), sizeof(framebuffer));
}

void downscale_and_gray() {
    for (int r = 0; r < 270; r++) {
        const uint16_t* row = framebuffer + r * 4 * 1920;
        for (int c = 0; c < 480; c++) {
            downscaled_gray_img[r * 480 + c] = static_cast<uint8_t>(row[c * 4]);
        }
    }
}

int main() {

    while (1) {

        update_framebuffer();
        downscale_and_gray();

        cv::Mat img(270, 480, CV_8UC1, downscaled_gray_img);

        cv::imwrite("test.png", img);
        cv::imshow("asdf", img);
        cv::waitKey(0);
        return 1;



    }


}