#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include "features.h"

int main(int argc, char **argv) {
    if(argc != 4) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file> <output video file> <feature file>\n", argv[0]);
        exit(1);
    }

    initFeatures();

    /* Create a window */
    cvNamedWindow("Overkill", CV_WINDOW_AUTOSIZE);

    /* capture frame from video file */
    CvCapture* capture = cvCreateFileCapture(argv[1]);

    /* Output file */
    CvVideoWriter* output = cvCreateVideoWriter(argv[2], CV_FOURCC('P','I','M','1'), 30, cvSize(1280,720), 1);

    /* In/Output feature list */
    FILE* fh = fopen(argv[3], "r+");
    if(fh == NULL) {
      fh = fopen(argv[3], "w");
    }
    assert(fh != NULL);

    /* Create IplImage to point to each frame */
    IplImage* frame;

    /* Loop until frame ended or ESC is pressed */
    while(1) {
        /* grab frame image, and retrieve */
        int current_frame = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);
        frame = cvQueryFrame(capture);
        int frame_read = 0;
        if(fread(&frame_read, sizeof(int), 1, fh) == 1) {
          /* we have data for this frame */
        } else {
          fwrite(&current_frame, sizeof(int), 1, fh);
        }

        /* Track features */
        for(int i=0;i<FEATURE_COUNT;i++) {
          CvPoint location;
          if(fread(&location, sizeof(CvPoint), 1, fh) == 1) {
          } else {
            location = matchFeature(frame, i);
            fwrite(&location, sizeof(CvPoint), 1, fh);
          }
          /* Draw marker */
          cvCircle(frame, location, 3, (current_frame % 2 == 0) ? CV_RGB(255,0,0):  CV_RGB(0,255,0), 1, CV_AA, 0);
          printf("Frame %d, Feature %d at %d, %d\n", current_frame, i, location.x, location.y);
        }

        /*CvPoint axis = matchFeature(frame, FEATURE_AXIS);
        axis.x -= 10;
        CvPoint topCenter = matchFeature(frame, FEATURE_TOP_CENTER);
        double radius = sqrt(pow(axis.x - topCenter.x,2) + pow(axis.y - topCenter.y,2));
        radius *= 1.27;
        cvCircle(frame, axis, radius, CV_RGB(0,255,255), 1, CV_AA, 0);*/

        /* exit loop if fram is null / movie end */
        if(!frame) break;

        /* display frame into window */
        cvShowImage("Overkill", frame);

        /* And output to file */
        cvWriteFrame(output, frame);

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
