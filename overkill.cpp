#include <opencv2/opencv.hpp>
#include <stdio.h>

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file>\n", argv[0]);
        exit(1);
    }

    cv::VideoCapture cap(argv[1]);
    if(!cap.isOpened()) {
      printf("Could not open video file: %s\n", argv[1]);
    }
    assert(cap.isOpened());
    return 0;
}
