#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

/* Why is there no such function in OpenCV? */
static inline double okDistance(CvPoint *pt1, CvPoint *pt2) {
  return sqrt(pow(pt1->x - pt2->x,2) + pow(pt1->y - pt2->y,2));
}
