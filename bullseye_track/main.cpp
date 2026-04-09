// sentry acceptable_x_closeness acceptable_y_closeness wait_time_between_frame T_for_verbose

#include <opencv2/opencv.hpp>
#include "launcher_commands.h"
#include <fstream>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#define CENTER_X 240
#define CENTER_Y 135

#define LAUNCHER_DEVICE "/dev/launcher0"

uint16_t framebuffer[1920*1080];

uint8_t downscaled_gray_buffer[480*270];
uint8_t blurred_buffer[480*270];

void update_framebuffer() {
    static int i = 0; // just trying to simulate camera quickly
    char buf[32];
    snprintf(buf, sizeof(buf), "./images/BIMG_%03d.BIN", i);
    std::ifstream file(buf, std::ios::binary);
    i++;
    if (i > 10)
        i = 0;
    
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


int chain_depth(const std::vector<cv::Point>& props, const std::vector<cv::Vec4i>& hierarchy, int i) {
    if (i == -1 || props[i] == cv::Point(-1, -1))
        return 0;
    return 1 + chain_depth(props, hierarchy, hierarchy[i][2]);
}

int main(int argc, char** argv) {

    int x_center_threshold = 10;
    int y_center_threshold = 10;
    int wait = 0;

    if (argc >= 3) {
        x_center_threshold = std::atoi(argv[1]);
        y_center_threshold = std::atoi(argv[2]);
    } if (argc >= 4) {
        wait = std::atoi(argv[3]);
    }

    bool verbose = argc >= 5 && (*argv[4] == 't' || *argv[4] == 'T');

    std::cout << "Starting" << std::endl;

    unsigned char command = LAUNCHER_STOP;
    int fd = open(LAUNCHER_DEVICE, O_WRONLY);
    if (fd == -1)
        return -1;

    write(fd, &command, 1);

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;


    while (1) {
        update_framebuffer(); // only relevant to our prototyping
        if (verbose) {
            start = std::chrono::high_resolution_clock::now();
        }
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
        }

        int idx_with_top_depth = -1;
        int top_depth = 0;

        for (int i = 0; i < contours.size(); i++) {
            if (props[i] == cv::Point(-1, -1))
                continue;
            
            int depth = chain_depth(props, hierarchy, i);
            if (depth > top_depth) {
                top_depth = depth;
                idx_with_top_depth = i;
            }
        }

        cv::Point center = cv::Point(-1, -1);
        if (top_depth >= 4)
            center = props[idx_with_top_depth];
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (verbose) {
            std::cout << "depth: " << top_depth << std::endl;
            std::cout << center << std::endl;
            std::cout << duration.count() << std::endl;
        }

        int x = center.x;
        int y = center.y;


        unsigned char previous_command = 0;

        if (std::abs(x - CENTER_X) < 10 && std::abs(y - CENTER_Y) < 10) {
            if (previous_command != LAUNCHER_FIRE) {
                command = LAUNCHER_STOP;
                write(fd, &command, 1);
            }
            command = LAUNCHER_FIRE;
            write(fd, &command, 1);
            usleep(wait);
            continue;
        }


        if (std::abs(x - CENTER_X) >= 10 && std::abs(y - CENTER_Y) >= 10) {
            if (x - CENTER_X > 0)
                command = LAUNCHER_RIGHT;
            else
                command = LAUNCHER_LEFT;
            
            if (y - CENTER_Y > 0)
                command |= LAUNCHER_UP;
            else
                command |= LAUNCHER_DOWN;
        } else if (std::abs(x - CENTER_X) >= 10) {
            if (x - CENTER_X > 0)
                command = LAUNCHER_RIGHT;
            else
                command = LAUNCHER_LEFT;
        } else {
            if (y - CENTER_Y > 0)
                command |= LAUNCHER_UP;
            else
                command |= LAUNCHER_DOWN;
        }

        if (previous_command == command) {
            usleep(wait);
            continue;
        }

        previous_command = command;
        command = LAUNCHER_STOP;
        write(fd, &command, 1);
        command = previous_command;
        write(fd, &command, 1);

        usleep(wait);
    }

}