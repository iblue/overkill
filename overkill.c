#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include "features.h"
#include "deshaker.h"
#include "mask.h"
#include "optiflow.h"
#include "inline.h"

float rotation_angle=0.0;

void opticalFlow(IplImage* prev, IplImage* curr, IplImage *mask, CvPoint2D32f*
    prev_pts, CvPoint2D32f *curr_pts, int* pts_cnt);
void highlightData(IplImage *target, CvPoint2D32f* curr_pts, CvPoint2D32f* prev_pts, int size);
int filterPts(CvPoint2D32f *pts, char* status, int size);
double angle(CvPoint2D32f *pts1, CvPoint2D32f *pts2, int size);

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
    IplImage* current_deshaked_frame = NULL;
    IplImage* last_deshaked_frame = NULL;

    /* For dynamic feature tracking */
    #define MAX_CORNERS 40
    #define MIN_CORNERS 10
    int corner_count = MAX_CORNERS;
    CvPoint2D32f corners[MAX_CORNERS];
    CvPoint2D32f last_corners[MAX_CORNERS];
    IplImage *tracking_mask = NULL;

    /* Holds the corners detected by findTrackingPoints */
    int tracking_point_count = MAX_CORNERS;
    CvPoint2D32f tracking_points[MAX_CORNERS];

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
        current_deshaked_frame = cvCreateImage(cvSize(frame->width,frame->height),
          frame->depth, frame->nChannels);
        deshake(frame, current_deshaked_frame);

        /* Create mask for dynamic feature detection, display mask in target */
        IplImage *target;

        // FIXME: Split masking and highlighting!
        tracking_mask = mask(current_deshaked_frame, &target);

        /* Detect dynamic features in mask, save to tracking_points */
        if(current_frame == 0 || corner_count < MIN_CORNERS) {
          tracking_point_count = MAX_CORNERS;
          findTrackingPoints(current_deshaked_frame, tracking_mask,
              &tracking_point_count, tracking_points);
          printf("Found %d tracking points\n", tracking_point_count);
          memcpy(corners, tracking_points, tracking_point_count*sizeof(corners[0]));
          memcpy(last_corners, tracking_points, tracking_point_count*sizeof(corners[0]));
          corner_count = tracking_point_count;
        }

        /* Determine movement of features across frames */
        opticalFlow(last_deshaked_frame, current_deshaked_frame, tracking_mask,
            last_corners, corners, &corner_count);
        assert(corner_count > 0);

        /* Calculate angle between features */
        rotation_angle += angle(last_corners, corners, corner_count);

        /* Rendering */
        highlightData(target, corners, last_corners, corner_count);

        /* create last frame for optical flow detection */
        if(last_deshaked_frame) {
          cvReleaseImage(&last_deshaked_frame);
        }
        last_deshaked_frame = cvCloneImage(current_deshaked_frame);
        memcpy(last_corners, corners, corner_count*sizeof(corners[0]));

        if(tracking_mask) {
          cvReleaseImage(&tracking_mask);
        }

        /* Free mem */
        cvReleaseImage(&tracking_mask);
        cvReleaseImage(&current_deshaked_frame);

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

/* Highlights some global data in the target frame */
void highlightData(IplImage *target, CvPoint2D32f* curr_pts, CvPoint2D32f* prev_pts, int size) {

  /* Highlight current feature position */
  for(int i=0;i<size;i++) {
    cvCircle(target, cvPointFrom32f(curr_pts[i]), 2, CV_RGB(255,255,255), 1, CV_AA, 0);
  }

  /* Highlight vectors */
  for(int i=0;i<size;i++) {
    cvLine(target, cvPointFrom32f(prev_pts[i]), cvPointFrom32f(curr_pts[i]), CV_RGB(127,0,127), 1, 4, 0);
  }

  /* Show current rotation angle */
  cvLine(target, cvPoint(640,358), cvPoint(640+200*cos(rotation_angle), 358+200*sin(rotation_angle)), CV_RGB(0,255,0), 2, CV_AA, 0);
}

void opticalFlow(IplImage* prev, IplImage* curr, IplImage *mask, CvPoint2D32f*
    prev_pts, CvPoint2D32f *curr_pts, int* pts_cnt) {

  static char corner_status[MAX_CORNERS];

  if(!prev) {
    return;
  }

  /* Termination criteria: FIXME static */
  int type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
  double eps = 0.01;
  int iter = 10;

  CvTermCriteria crit = cvTermCriteria(type,iter,eps);
  cvCalcOpticalFlowPyrLK(prev, curr,
      NULL, NULL, prev_pts, curr_pts, *pts_cnt,
      cvSize(10,10), 3, corner_status, NULL, crit, 0);

  /* Remove points that left the tracking mask */
  for(int i=0;i<*pts_cnt;i++) {
    /* Not found by flow tracking -> skip */
    if(corner_status[i] == 0) {
      continue;
    }

    /* Point left the tracking mask */
    CvPoint pnt = cvPointFrom32f(curr_pts[i]);
    int col = mask->imageData[1280*pnt.y+pnt.x];
    if(col == 0) {
      corner_status[i] = 0;
      printf("Feature %d left bounds\n", i);
    }
  }

  /* Remove all points marked for deletion */
  filterPts(curr_pts, corner_status, *pts_cnt);
  *pts_cnt = filterPts(prev_pts, corner_status, *pts_cnt);
}

int filterPts(CvPoint2D32f *pts, char* status, int size) {
  CvPoint2D32f new[MAX_CORNERS];

  int pos=0;
  for(int i=0;i<size;i++) {
    if(status[i] != 0) {
      new[pos++] = pts[i];
    }
  }
  memcpy(pts, new, pos*sizeof(pts[0]));
  return pos;
}

double angle(CvPoint2D32f *pts1, CvPoint2D32f *pts2, int size) {
  CvMat last_cc    = cvMat(size, 1, CV_32FC2, pts1);
  CvMat current_cc = cvMat(size, 1, CV_32FC2, pts2);
  CvMat *transformation = cvCreateMat(2, 3, CV_32FC1);
  cvEstimateRigidTransform(&last_cc, &current_cc, transformation, 0);
  return atan2(cvmGet(transformation,1,0), cvmGet(transformation,0,0));
}
