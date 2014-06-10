#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <math.h>
#include "overkill.h"
#include "features.h"
#include "deshaker.h"
#include "mask.h"
#include "optiflow.h"
#include "inline.h"

float feature_angle=0.0;
float rotation_angle = 0.0; /* normed -pi..pi */
int revolutions=0;          /* real revolutions */
float current_error=0.0;
float current_time=0.0;
int current_frame=0;

void highlightData(IplImage *target, CvPoint2D32f* curr_pts, CvPoint2D32f* prev_pts, int size);
double angle(CvPoint2D32f *pts1, CvPoint2D32f *pts2, int size);

#ifdef RENDER_INFO_TEXT
void renderText(IplImage *target);
#endif

int main(int argc, char **argv) {
    if(argc != 5) {
        printf("Overkill: Motion Tracking Prototype\n\n"
               "Usage: %s <input video file> <output video file> <feature file> <csv out>\n", argv[0]);
        exit(1);
    }

    FILE* csv_fh = fopen(argv[4],"w");

    /* Initialize static feature detection */
    initFeatures(argv[3]);

#ifdef RENDER_SCREEN
    /* Create a window */
    cvNamedWindow("Overkill", CV_WINDOW_AUTOSIZE);
#endif

    /* capture frame from video file */
    CvCapture* capture = cvCreateFileCapture(argv[1]);

    /* Output file */
    CvVideoWriter* output = cvCreateVideoWriter(argv[2], CV_FOURCC('P','I','M','1'), 30, cvSize(1280,720), 1);

    /* Create IplImage to point to each frame */
    IplImage* frame;
    IplImage* current_deshaked_frame = NULL;
    IplImage* last_deshaked_frame = NULL;
    IplImage* feature_frame = NULL;

    /* For dynamic feature tracking */
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
        current_frame = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);
        int total_frames = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
        current_time = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_MSEC)/1000.0;
        frame = cvQueryFrame(capture);

        printf("Processing frame %d/%d...\n", current_frame, total_frames);

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
          /* Detect flow to last dynamic frame for correction */
          if(feature_frame) {
            /* Calculate angle between features */
            // FIXME: Does not work, because some corners left.
            // Also does not work because rotation is ambigious
            //feature_angle += angle(tracking_points, last_corners, corner_count);
            //rotation_angle = feature_angle;

            cvReleaseImage(&feature_frame);
          }

          tracking_point_count = MAX_CORNERS;
          findTrackingPoints(current_deshaked_frame, tracking_mask,
              &tracking_point_count, tracking_points);
          //printf("Found %d tracking points\n", tracking_point_count);
          memcpy(corners, tracking_points, tracking_point_count*sizeof(corners[0]));
          memcpy(last_corners, tracking_points, tracking_point_count*sizeof(corners[0]));
          corner_count = tracking_point_count;

          /* Same frame for later detection */
          feature_frame = cvCloneImage(current_deshaked_frame);
        }

        /* Determine movement of features across frames */
        opticalFlow(last_deshaked_frame, current_deshaked_frame, tracking_mask,
            last_corners, corners, &corner_count);

        /* Calculate angle between features */
        if(corner_count > 0) {
          double da = angle(last_corners, corners, corner_count);
          rotation_angle += da;
          if(rotation_angle > M_PI) {
            rotation_angle -= 2*M_PI;
          }
          if(rotation_angle < -M_PI) {
            rotation_angle += 2*M_PI;
          }
          if(da == 0.0) {
            current_error = M_PI;
            printf("Tracking lost.\n");
          } else {
            double add_err = fabs(pow(da*8.0,2)*0.05); /* heuristic error values */

            // heuristic value correction
            current_error += add_err;
          }
        } else {
          current_error = M_PI;
          printf("Tracking lost.\n");
        }

        if(current_error > M_PI) {
          current_error = M_PI;
        }

        /* Try resync */
        int sync_zone = resyncByStatic(current_frame, target);

        if(sync_zone == FEATURE_ZERO) {
          /* Very good sync: Max 1.5 degrees error */
          current_error = (1.5/360.0)*(2*M_PI);
        } else if(sync_zone == FEATURE_90 || sync_zone == FEATURE_270) {
          /* Other zones not so exact. About 2-3 degrees error */
          current_error = (2.5/360.0)*(2*M_PI);
        }

        /* Revolution counter: Only if in sync and angle goes over zero mark */
        double last_angle;
        if(current_frame == 0) {
          last_angle = rotation_angle;
        }
        if(current_frame != 0){
          if(sync_zone == FEATURE_ZERO) {
            //printf("in zerosync. last: %f, current: %f \n", last_angle, rotation_angle);
            if(fabs(last_angle/(2*M_PI)*360.0) < 30.0) {
              if(last_angle < 0 && rotation_angle > 0) { // prevent false positives
                revolutions++;
              } else if(last_angle > 0 && rotation_angle < 0) {
                revolutions--;
              }
            }
          }
        }
        last_angle = rotation_angle;



        // Display angle
        {
          double degrees   = rotation_angle/(2*M_PI)*360;
          if(degrees < 0) {
            degrees += 360.0;
          }
          double error = current_error/(2*M_PI)*360;
          printf("%f: revolutions: %d, degrees %2.1f +- %2.1f\n", current_time,
              revolutions, degrees, error);
          // CSV output
          fprintf(csv_fh, "%f,0.0,%f,%f\n", current_time, ((double)revolutions*2.0*M_PI)+rotation_angle, current_error);
        }

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

        /* Invert deshake */
        {
          IplImage *new = cvCreateImage(cvSize(target->width,target->height),
              target->depth, target->nChannels);
          cvWarpAffine(target, new, ok_transformation,
              CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS+CV_WARP_INVERSE_MAP, CV_RGB(0,0,0));
          cvReleaseImage(&target);
          target = new;
        }
#ifdef RENDER_INFO_TEXT
        renderText(target);
#endif

        /* display frame into window and write to outfile */
#ifdef RENDER_SCREEN
        cvShowImage("Overkill", target);
#endif
        cvWriteFrame(output, target);

        /* Free target mem */
        cvReleaseImage(&target);
        /* Frame is released automatically */

        /* if ESC or q is pressed then exit loop */
#ifdef RENDER_SCREEN
        char c = cvWaitKey(33);
        if(c==27 || c == 'q') break;
#endif
    }

    /* destroy pointer to capturing */
    cvReleaseCapture(&capture);

    /* delete window */
#ifdef RENDER_SCREEN
    cvDestroyWindow("Overkill");
#endif
    return 0;
}

#ifdef RENDER_INFO_TEXT
void renderText(IplImage *target) {
  CvFont font;
  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1, 1, 0, 1, CV_AA);

  char value[40];
  char lost[] = "Tracking lost.";

  double degrees   = rotation_angle/(2*M_PI)*360;
  if(degrees < 0) {
    degrees += 360.0;
  }
  double error = current_error/(2*M_PI)*360;

  if(error > 90.0) {
    memcpy(value, lost, sizeof(lost));
   } else {
    snprintf((char *)&value, sizeof(value)-1, "%f: %d + %2.1f+-%2.1f",
        current_time, revolutions, degrees, error);
   }
  cvPutText(target, (const char*)value, cvPoint(50,680), &font, CV_RGB(0,0,0));
}
#endif

/* Highlights some global data in the target frame */
void highlightData(IplImage *target, CvPoint2D32f* curr_pts, CvPoint2D32f* prev_pts, int size) {

#ifdef RENDER_DEBUG
  /* Highlight current feature position */
  for(int i=0;i<size;i++) {
    cvCircle(target, cvPointFrom32f(curr_pts[i]), 2, CV_RGB(255,255,255), 1, CV_AA, 0);
  }

  /* Highlight vectors */
  for(int i=0;i<size;i++) {
    cvLine(target, cvPointFrom32f(prev_pts[i]), cvPointFrom32f(curr_pts[i]), CV_RGB(127,0,127), 1, 4, 0);
  }
#endif

  /* Show current rotation angle */
  double real_ang = rotation_angle;
  double err_p = rotation_angle + current_error;
  double err_n = rotation_angle - current_error;
  cvLine(target, cvPoint(640,358), cvPoint(640+200*cos(real_ang), 358+200*sin(real_ang)), CV_RGB(0,255,0), 1, CV_AA, 0);
  cvLine(target, cvPoint(640,358), cvPoint(640+200*cos(err_p), 358+200*sin(err_p)), CV_RGB(255,0,0), 1, CV_AA, 0);
  cvLine(target, cvPoint(640,358), cvPoint(640+200*cos(err_n), 358+200*sin(err_n)), CV_RGB(255,0,0), 1, CV_AA, 0);

}



double angle(CvPoint2D32f *pts1, CvPoint2D32f *pts2, int size) {
  CvMat last_cc    = cvMat(size, 1, CV_32FC2, pts1);
  CvMat current_cc = cvMat(size, 1, CV_32FC2, pts2);
  CvMat *transformation = cvCreateMat(2, 3, CV_32FC1);
  cvEstimateRigidTransform(&last_cc, &current_cc, transformation, 0);
  return atan2(cvmGet(transformation,1,0), cvmGet(transformation,0,0));
}
