#include <highgui.h>
#include <stdio.h>

#define FEATURE_COUNT 4
IplImage* features[FEATURE_COUNT];


void matchFeature(IplImage *frame, IplImage *template) {
  /* Initialize result space */
  /* TODO: Optimization: Only once per template. Maybe on template load? */
  //static CvMat *result = NULL;
  //if(result == NULL) {
  CvMat *result = NULL;
  int result_cols = frame->width - template->width + 1;
  int result_rows = frame->height - template->height + 1;
  result = cvCreateMat(result_rows, result_cols, CV_32FC1);
  //}

  cvMatchTemplate(frame, template, result, 1);

  double minVal; double maxVal; CvPoint minLoc; CvPoint maxLoc;
  CvPoint matchLoc;

  cvMinMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, NULL);
  matchLoc = minLoc;

  /* Marker is at top left, move to center */
  matchLoc.x += template->width/2;
  matchLoc.y += template->height/2;

  /* Draw circle, FIXME: Remove */
  cvCircle(frame, matchLoc, 2, CV_RGB(0,255,0), 1, CV_AA, 0);

  return;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file>\n", argv[0]);
        exit(1);
    }

    /* FIXME: Dynamic from config file. Also change FEATURE_COUNT */
    features[0] = cvLoadImage("/home/iblue/motiontrack/axis.png", 1);
    features[1] = cvLoadImage("/home/iblue/motiontrack/top-center.png", 1);
    features[2] = cvLoadImage("/home/iblue/motiontrack/edge-left.png", 1);
    features[3] = cvLoadImage("/home/iblue/motiontrack/edge-right.png", 1);

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
          matchFeature(frame, features[i]);
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
