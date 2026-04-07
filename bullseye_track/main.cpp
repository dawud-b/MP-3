
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <fstream>
#include <vector>


uint16_t framebuffer[1920*1080];

uint8_t downscaled_gray_buffer[480*270];
uint8_t blurred_buffer[480*270];

void update_framebuffer() {
    std::ifstream file("../images/BIMG_001.BIN", std::ios::binary);
    if (!file) return;

    file.read(reinterpret_cast<char*>(framebuffer), sizeof(framebuffer));
}

void downscale_and_gray() {
    for (int r = 0; r < 270; r++) {
        const uint16_t* row = framebuffer + r * 4 * 1920;
        for (int c = 0; c < 480; c++) {
            downscaled_gray_buffer[r * 480 + c] = static_cast<uint8_t>(row[c * 4]);
        }
    }
}

cv::Point contour_center_if_circular(const std::vector<cv::Point>& cnt, float circular_threshold = 0.7) {
    double area = cv::contourArea(cnt);
    if (area < 100)
        return cv::Point (-1, -1);
    double perimeter = cv::arcLength(cnt, true);
    if (perimeter == 0)
        return cv::Point (-1, -1);
    
    double circularity = (4 * 3.14159265358979323846 * area) / (perimeter * perimeter);
    if (circularity <= circular_threshold)
        return cv::Point (-1, -1);
    
    cv::Moments M = cv::moments(cnt);
    return cv::Point (M.m10 / M.m00, M.m01 / M.m00);
}

int main() {

    std::cout << "Starting" << std::endl;

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    while (1) {
        update_framebuffer();
        downscale_and_gray();

        cv::Mat img(270, 480, CV_8UC1, downscaled_gray_buffer);
        cv::Mat blurred_img(270, 480, CV_8UC1, blurred_buffer);

        cv::GaussianBlur(img, blurred_img, cv::Size(3, 3), 1);

        // reuse old image; don't use another buffer uneccessary
        cv::threshold(blurred_img, img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        contours.clear();
        hierarchy.clear();

        cv::findContours(img, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Point> props;
        for (const auto& cnt : contours) {
            cv::Point center = contour_center_if_circular(cnt);
            props.push_back(center);
            std::cout << center << std::endl;
        }



        cv::imwrite("test.png", img);
        return 1;
    }

}