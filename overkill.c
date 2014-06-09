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
    #define MAX_CORNERS 40
    #define MIN_CORNERS 20
    int corner_count = MAX_CORNERS;
    CvPoint2D32f corners[MAX_CORNERS];
    int last_corner_count = MAX_CORNERS;
    CvPoint2D32f last_corners[MAX_CORNERS];
    char corner_status[MAX_CORNERS];
    IplImage *tracking_mask = NULL;

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

          /* Remove points that left the tracking mask */
          for(int i=0;i<last_corner_count;i++) {
            /* Not found by flow tracking -> skip */
            if(corner_status[i] == 0) {
              continue;
            }

            /* Point left the tracking mask */
            CvPoint pnt = cvPointFrom32f(corners[i]);
            int col = tracking_mask->imageData[1280*pnt.y+pnt.x];
            if(col == 0) {
              corner_status[i] = 0;
              printf("mark %d out\n", i);
            }
          }

          /* Remove all points marked for deletion */
          int pos;
          CvPoint2D32f new_corners[MAX_CORNERS];

          pos=0;
          for(int i=0;i<corner_count;i++) {
            if(corner_status[i] == 1) {
              new_corners[pos] = corners[i];
              pos++;
            }
          }
          corner_count = pos;
          memcpy(corners, new_corners, sizeof(corners[0])*corner_count);

          pos=0;
          for(int i=0;i<last_corner_count;i++) {
            if(corner_status[i] == 1) {
              new_corners[pos] = last_corners[i];
              pos++;
            }
          }
          last_corner_count = pos;
          memcpy(last_corners, new_corners, sizeof(corners[0])*corner_count);

          /* Calculate angle */
          // FIXME: Use only found
          CvMat last_cc    = cvMat(last_corner_count, 1, CV_32FC2, last_corners);
          CvMat current_cc = cvMat(last_corner_count, 1, CV_32FC2, corners);
          CvMat *transformation = cvCreateMat(2, 3, CV_32FC1);
          cvEstimateRigidTransform(&last_cc, &current_cc, transformation, 0);
          double da = atan2(cvmGet(transformation,1,0), cvmGet(transformation,0,0));
          rotation_angle += da;
          printf("angle: %f\n", rotation_angle);
        }

        /* Create mask for dynamic feature detection, display mask in target */
        IplImage *target;
        if(tracking_mask) {
          cvReleaseImage(&tracking_mask);
        }
        tracking_mask = mask(deshaked_frame, &target);

        /* Highlight vectors */
        for(int i=0;i<last_corner_count;i++) {
          cvLine(target, cvPointFrom32f(corners[i]), cvPointFrom32f(last_corners[i]), CV_RGB(127,0,127), 1, 4, 0);
        }

        /* Detect dynamic features in mask */
        if(current_frame == 0 || corner_count < MIN_CORNERS) {
          corner_count = MAX_CORNERS;
          findTrackingPoints(deshaked_frame, tracking_mask, &corner_count, corners);
        }

        /* We need lots of corners to track for the rigid transform to converge */
        //assert(corner_count > MIN_CORNERS/2);

        /* create last frame for optical flow detection */
        last_deshaked_frame = cvCloneImage(deshaked_frame);
        last_corner_count = corner_count;
        memcpy(last_corners, corners, corner_count*sizeof(corners[0]));


        /* Highlight detected features */
        for(int i=0;i<corner_count;i++) {
          cvCircle(target, cvPointFrom32f(corners[i]), 2, CV_RGB(255,255,255), 1, CV_AA, 0);
        }

        /* Show current rotation angle */
        cvLine(target, cvPoint(640,358), cvPoint(640+200*cos(rotation_angle), 358+200*sin(rotation_angle)), CV_RGB(0,255,0), 2, CV_AA, 0);

        /* Free mem */
        //cvReleaseImage(&tracking_mask);
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
