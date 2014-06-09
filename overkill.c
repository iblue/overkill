#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include "features.h"
#include "deshaker.h"
#include "mask.h"
#include "optiflow.h"
#include "inline.h"

int main(int argc, char **argv) {
    if(argc != 4) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file> <output video file> <feature file>\n", argv[0]);
        exit(1);
    }

    /* Initialize static feature detection */
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

        /* track static features into global variable */
        trackFeatures(frame, current_frame);

        /* deshake using static features */
        IplImage *deshaked_frame = cvCreateImage(cvSize(frame->width,frame->height),
          frame->depth, frame->nChannels);
        deshake(frame, deshaked_frame);

        /* Create mask for dynamic feature detection, display mask in target */
        IplImage *target;
        IplImage *tracking_mask = mask(deshaked_frame, &target);

        /* Detect dynamic features in mask */
        int corner_count = 30;
        CvPoint2D32f corners[corner_count];
        findTrackingPoints(deshaked_frame, tracking_mask, &corner_count, corners);

        /* Highlight detected features */
        for(int i=0;i<corner_count;i++) {
          cvCircle(target, cvPointFrom32f(corners[i]), 2, CV_RGB(255,255,255), 1, CV_AA, 0);
        }

        /* Free mem */
        cvReleaseImage(&tracking_mask);
        cvReleaseImage(&deshaked_frame);

        /* display frame into window and write to outfile */
        cvShowImage("Overkill", target);
        cvWriteFrame(output, target);

        /* Free target mem */
        cvReleaseImage(&target);
        /* Frame is released automatically */

        /* if ESC or q is pressed then exit loop */
        char c = cvWaitKey(33);
        if(c==27 || c == 'q') break;
    }

    /* destroy pointer to capturing */
    cvReleaseCapture(&capture);

    /* delete window */
    cvDestroyWindow("Overkill");

    return 0;
}
