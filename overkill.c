#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include "features.h"

/* Why is there no such function in OpenCV? */
static inline double okDistance(CvPoint *pt1, CvPoint *pt2) {
  return sqrt(pow(pt1->x - pt2->x,2) + pow(pt1->y - pt2->y,2));
}

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

        /* exit loop if fram is null / movie end */
        if(!frame) break;

        int frame_read = 0;
        if(fread(&frame_read, sizeof(int), 1, fh) == 1) {
          /* we have data for this frame */
        } else {
          fwrite(&current_frame, sizeof(int), 1, fh);
        }

        /* Track features */
        CvPoint last_location[FEATURE_COUNT];

        for(int i=0;i<FEATURE_COUNT;i++) {
          /* Calculate feature position or load from feature cache */
          CvPoint location;
          if(fread(&location, sizeof(CvPoint), 1, fh) == 1) {
          } else {
            location = matchFeature(frame, i);
            fwrite(&location, sizeof(CvPoint), 1, fh);
          }

          /* Init */
          if(current_frame == 0) {
            last_location[i] = location;
          }

          /* Calculate distance to last location. If distance too big -> skip marker */
          double distance = okDistance(&last_location[i], &location);
          char marker_fail = 0;

          if(distance > 15.0) {
            marker_fail = 1;
          } else {
            last_location[i] = location;
          }

          /* Draw marker */
          CvScalar color = (current_frame % 2 == 0) ? CV_RGB(255,0,0):  CV_RGB(0,255,0);
          if(marker_fail) {
            color = CV_RGB(255,255,0);
          }

          cvCircle(frame, location, 3, color, 1, CV_AA, 0);

          printf("Frame %d, Feature %d at %d, %d. FAIL: %d\n", current_frame, i, location.x, location.y, marker_fail);
        }

        /* Signed ints will be transformed to floats anyway, so we can do this
         * directly without performance loss */

        /* Calculate transformation to target */
        CvPoint2D32f targets[] = {
          cvPoint2D32f(640.0, 360.0),   /* Axis */
          cvPoint2D32f(433.0, 360.0),   /* Top Center */
          cvPoint2D32f(454.0, 467.0),   /* Edge Left */
          cvPoint2D32f(447.0, 253.0),   /* Edge Right */
          cvPoint2D32f(815.0, 446.0),   /* Bottom Left */
          cvPoint2D32f(810.0, 253.0),   /* Bottom Right */
        };
        CvMat to = cvMat(6, 1, CV_32FC2, targets);

        // Matrix contains 2D vectors as channels (who designed this shit?)
        CvPoint2D32f points[6];
        for(int i=0;i<6;i++) {
          points[i] = cvPoint2D32f((double) last_location[i].x,
                                   (double) last_location[i].y);
        }
        CvMat from = cvMat(6, 1, CV_32FC2, points);

        CvMat *transformation = cvCreateMat(2,3, CV_32FC1);

        cvEstimateRigidTransform(&from, &to, transformation, 0);

        /* FIXME: Overkill - we just need the same type */
        IplImage *dst = cvCloneImage(frame);
        cvWarpAffine(frame, dst, transformation, CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, CV_RGB(0,0,0));


        /*CvPoint axis = matchFeature(frame, FEATURE_AXIS);
        axis.x -= 10;
        CvPoint topCenter = matchFeature(frame, FEATURE_TOP_CENTER);
        double radius = sqrt(pow(axis.x - topCenter.x,2) + pow(axis.y - topCenter.y,2));
        radius *= 1.27;
        cvCircle(frame, axis, radius, CV_RGB(0,255,255), 1, CV_AA, 0);*/

        /* display frame into window */
        cvShowImage("Overkill", dst);

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
