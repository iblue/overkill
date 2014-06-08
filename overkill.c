#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include "features.h"
#include "deshaker.h"
#include "mask.h"
#include "inline.h"

int main(int argc, char **argv) {
    if(argc != 4) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file> <output video file> <feature file>\n", argv[0]);
        exit(1);
    }

    initFeatures(argv[3]);

    /* Create a window */
    cvNamedWindow("Overkill", CV_WINDOW_AUTOSIZE);

    /* capture frame from video file */
    CvCapture* capture = cvCreateFileCapture(argv[1]);

    /* Output file */
    CvVideoWriter* output = cvCreateVideoWriter(argv[2], CV_FOURCC('P','I','M','1'), 30, cvSize(1280,720), 1);

    /* Create IplImage to point to each frame */
    IplImage* frame;

    /* Loop until frame ended or ESC is pressed */
    while(1) {
        /* grab frame image, and retrieve */
        int current_frame = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);
        frame = cvQueryFrame(capture);

        /* exit loop if fram is null / movie end */
        if(!frame) break;

        trackFeatures(frame, current_frame);

        IplImage *temp = cvCreateImage(cvSize(frame->width,frame->height),
          frame->depth, frame->nChannels);

        /*IplImage *target = cvCreateImage(cvSize(frame->width,frame->height),
          frame->depth, frame->nChannels);*/
        IplImage *target;

        deshake(frame, temp);
        target = mask(temp);
        cvReleaseImage(&temp);

        /* display frame into window and write to outfile */
        cvShowImage("Overkill", target);
        cvWriteFrame(output, target);

        cvReleaseImage(&target);

        /* if ESC or q is pressed then exit loop */
        char c = cvWaitKey(33);
        if(c==27 || c == 'q') break;
    }

    /* destroy pointer to video */
    cvReleaseCapture(&capture);

    /* delete window */
    cvDestroyWindow("Overkill");

    return 0;
}
