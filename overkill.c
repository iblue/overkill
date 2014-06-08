#include <highgui.h>
#include <stdio.h>
#include "features.h"

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file>\n", argv[0]);
        exit(1);
    }

    initFeatures();

    /* Create a window */
    cvNamedWindow("Overkill", CV_WINDOW_AUTOSIZE);

    /* capture frame from video file */
    CvCapture* capture = cvCreateFileCapture(argv[1]);

    /* Create IplImage to point to each frame */
    IplImage* frame;

    /* Loop until frame ended or ESC is pressed */
    while(1) {
        /* grab frame image, and retrieve */
        frame = cvQueryFrame(capture);

        /* Track features */
        for(int i=0;i<FEATURE_COUNT;i++) {
          matchFeature(frame, i);
        }

        /* exit loop if fram is null / movie end */
        if(!frame) break;

        /* display frame into window */
        cvShowImage("Overkill", frame);

        /* if ESC is pressed then exit loop */
        char c = cvWaitKey(33);
        if(c==27 || c == 'q') break;
    }

    /* destroy pointer to video */
    cvReleaseCapture(&capture);

    /* delete window */
    cvDestroyWindow("Overkill");

    return 0;
}
