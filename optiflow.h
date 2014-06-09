void findTrackingPoints(IplImage *deshaked_frame, IplImage *mask,
    int* max_corners, CvPoint2D32f* corners);
void opticalFlow(IplImage* prev, IplImage* curr, IplImage *mask, CvPoint2D32f*
    prev_pts, CvPoint2D32f *curr_pts, int* pts_cnt);
int filterPts(CvPoint2D32f *pts, char* status, int size);
