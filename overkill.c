#include <cv.h>

#include <highgui.h>
#include <stdio.h>
#include "features.h"
#include "deshaker.h"
#include "mask.h"
#include "optiflow.h"
#include "inline.h"

float rotation_angle=0.0;

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
    IplImage* last_deshaked_frame = NULL;

    /* For dynamic feature tracking */
    int corner_count = 30;
    CvPoint2D32f corners[corner_count];
    int last_corner_count = corner_count;
    CvPoint2D32f last_corners[corner_count];
    char corner_status[corner_count];
    CvPoint2D32f lock_on_corners[corner_count];
    int lock_on_corner_count = corner_count;

    double da = 0.0;

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

        /* Detect optical flow to last frame */
        if(last_deshaked_frame) {
          /* Termination criteria */
          int type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
          double eps = 0.01;
          int iter = 10;

          CvTermCriteria crit = cvTermCriteria(type,iter,eps);
          cvCalcOpticalFlowPyrLK(last_deshaked_frame, deshaked_frame,
              NULL, NULL, last_corners, corners, last_corner_count,
              cvSize(10,10), 3, corner_status, NULL, crit, 0);
          cvReleaseImage(&last_deshaked_frame);

          /* Calculate angle */
          // FIXME: Use only found
          CvMat last_cc    = cvMat(last_corner_count, 1, CV_32FC2, lock_on_corners);
          CvMat current_cc = cvMat(last_corner_count, 1, CV_32FC2, corners);
          CvMat *transformation = cvCreateMat(2, 3, CV_32FC1);
          cvEstimateRigidTransform(&last_cc, &current_cc, transformation, 0);
          da = atan2(cvmGet(transformation,1,0), cvmGet(transformation,0,0));
          printf("angle: %f\n", rotation_angle);
        }

        /* Create mask for dynamic feature detection, display mask in target */
        IplImage *target;
        IplImage *tracking_mask = mask(deshaked_frame, &target);

        /* Detect dynamic features in mask */
        if(current_frame%3 == 0) {
          rotation_angle += da;
          findTrackingPoints(deshaked_frame, tracking_mask, &corner_count, corners);
          lock_on_corner_count = corner_count;
          memcpy(lock_on_corners, corners, lock_on_corner_count*sizeof(corners[0]));
        }

        /* create last frame for optical flow detection */
        last_deshaked_frame = cvCloneImage(deshaked_frame);
        last_corner_count = corner_count;
        memcpy(last_corners, corners, corner_count*sizeof(corners[0]));


        /* Highlight detected features */
        for(int i=0;i<corner_count;i++) {
          cvCircle(target, cvPointFrom32f(corners[i]), 2, CV_RGB(255,255,255), 1, CV_AA, 0);
        }

        /* Show current rotation angle */
        cvLine(target, cvPoint(640,355), cvPoint(640+200*cos(rotation_angle), 355+200*sin(rotation_angle)), CV_RGB(255,0,0), 1, CV_AA, 0);

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
