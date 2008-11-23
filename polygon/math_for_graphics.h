// math stuff for graphics, from FreePCB

#ifndef abs
#define abs(x) (((x) >=0) ? (x) : (-(x)))
#endif


typedef struct PointTag
{
	double X,Y;
} PointT;

typedef struct EllipseTag
{
	PointT Center;			/* ellipse center	 */
//	double MaxRad,MinRad;	 /* major and minor axis */
//	double Phi;				/* major axis rotation  */
	double xrad, yrad;		// radii on x and y
	double theta1, theta2;	// start and end angle for arc
} EllipseKH;


// math stuff for graphics
bool Quadratic( double a, double b, double c, double *x1, double *x2 );
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist );
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style,
				double * x1, double * y1, double * x2, double * y2, double * dist=NULL );
int FindSegmentIntersections( int xi, int yi, int xf, int yf, int style,
								 int xi2, int yi2, int xf2, int yf2, int style2,
								 double x[]=NULL, double y[]=NULL );
bool FindLineEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 );
bool FindVerticalLineEllipseIntersections( double a, double b, double x, double *y1, double *y2 );
bool TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f,
									   int x2i, int y2i, int x2f, int y2f,
									   int * x=NULL, int * y=NULL, double * dist=NULL );
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int style1, int w1,
								   int x2i, int y2i, int x2f, int y2f, int style2, int w2,
								   int max_cl, int * x, int * y );

/** Function GetPointToLineSegmentDistance
 * Get distance between line segment and point
 * @param x,y = point
 * @param	xi,yi and xf,yf = the end-points of the line segment
 * @return the distance
 */
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf );

double GetPointToLineDistance( double a, double b, int x, int y, double * xp=NULL, double * yp=NULL );
bool InRange( double x, double xi, double xf );
double Distance( int x1, int y1, int x2, int y2 );
int GetArcIntersections( EllipseKH * el1, EllipseKH * el2,
						double * x1=NULL, double * y1=NULL,
						double * x2=NULL, double * y2=NULL );

