/*! \file kbool/samples/boolonly/boolonly.cpp
    \brief boolonly demonstrates the use of the boolean algorithm 
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: boolonly.cpp,v 1.9 2005/01/16 18:39:24 kire_putsje Exp $
*/

#ifdef __GNUG__
#pragma implementation
#endif

#include "boolonly.h"

#include <math.h>

// Constructors
KBoolPoint::KBoolPoint()
{
    _x = 0.0;
    _y = 0.0;
}

KBoolPoint::KBoolPoint(double const X, double const Y)
{
    _x = X;
    _y = Y;
}

double KBoolPoint::GetX()
{
    return _x;
}

double KBoolPoint::GetY()
{
    return _y;
}

template class TDLI<KBoolPoint>;


void ArmBoolEng( Bool_Engine* booleng )
{
    // set some global vals to arm the boolean engine
    double DGRID = 1000;  // round coordinate X or Y value in calculations to this
    double MARGE = 0.001;   // snap with in this range points to lines in the intersection routines
                          // should always be > DGRID  a  MARGE >= 10*DGRID is oke
                          // this is also used to remove small segments and to decide when
                          // two segments are in line.
    double CORRECTIONFACTOR = 500.0;  // correct the polygons by this number
    double CORRECTIONABER   = 1.0;    // the accuracy for the rounded shapes used in correction
    double ROUNDFACTOR      = 1.5;    // when will we round the correction shape to a circle
    double SMOOTHABER       = 10.0;   // accuracy when smoothing a polygon
    double MAXLINEMERGE     = 1000.0; // leave as is, segments of this length in smoothen
 

    // DGRID is only meant to make fractional parts of input data which 
    // are doubles, part of the integers used in vertexes within the boolean algorithm.
    // Within the algorithm all input data is multiplied with DGRID
    
    // space for extra intersection inside the boolean algorithms
    // only change this if there are problems
    int GRID =10000;

    booleng->SetMarge( MARGE );
    booleng->SetGrid( GRID );
    booleng->SetDGrid( DGRID );
    booleng->SetCorrectionFactor( CORRECTIONFACTOR );
    booleng->SetCorrectionAber( CORRECTIONABER );
    booleng->SetSmoothAber( SMOOTHABER );
    booleng->SetMaxlinemerge( MAXLINEMERGE );
    booleng->SetRoundfactor( ROUNDFACTOR );

}

void AddPolygonsToBoolEng2( Bool_Engine* booleng )
{
    int x1 = 100;
    int x2 = 200;
    int y1 = 100;
    int y2 = 200;
    int pitch1 = 200;
    int numRowsAndCols = 120;
    int i, j;

    for ( i = 0; i < numRowsAndCols; i++) {
        for ( j = 0; j < numRowsAndCols; j++) {
            // foreach point in a polygon ...
            if (booleng->StartPolygonAdd(GROUP_A))
            {
                // Counter-Clockwise
                booleng->AddPoint(x1,y1);
                booleng->AddPoint(x2,y1);
                booleng->AddPoint(x2,y2);
                booleng->AddPoint(x1,y2);

            }
            booleng->EndPolygonAdd(); 
            x1 += pitch1;
            x2 += pitch1;
        }
        x1 = 100;
        x2 = 200;

        y1 += pitch1;
        y2 += pitch1;

    }

    x1 = 150;
    x2 = 250;
    y1 = 150;
    y2 = 250;

    for ( i = 0; i < numRowsAndCols; i++) {
        for ( int j = 0; j < numRowsAndCols; j++) {
            // foreach point in a polygon ...
            if (booleng->StartPolygonAdd(GROUP_B))
            {
                // Counter Clockwise
                booleng->AddPoint(x1,y1);
                booleng->AddPoint(x2,y1);
                booleng->AddPoint(x2,y2);
                booleng->AddPoint(x1,y2);

            }
            booleng->EndPolygonAdd(); 
            x1 += pitch1;
            x2 += pitch1;
        }
        x1 = 150;
        x2 = 250;

        y1 += pitch1;
        y2 += pitch1;

    }

}

void AddPolygonsToBoolEng( Bool_Engine* booleng )
{
    // foreach point in a polygon ...
    if (booleng->StartPolygonAdd(GROUP_A))
    {
        booleng->AddPoint( 28237.480000, 396.364000 );
        booleng->AddPoint( 28237.980000, 394.121000 );
        booleng->AddPoint( 28242.000000, 395.699000 );
        booleng->AddPoint( 28240.830000, 397.679000 );
    }
    booleng->EndPolygonAdd(); 

    // foreach point in a polygon ...
    if (booleng->StartPolygonAdd(GROUP_B))
    {
        booleng->AddPoint( 28242.100000, 398.491000 );
        booleng->AddPoint( 28240.580000, 397.485000 );
        booleng->AddPoint( 28237.910000, 394.381000 );
    }
    booleng->EndPolygonAdd(); 

    if (booleng->StartPolygonAdd(GROUP_B))
    {
        booleng->AddPoint( 28243.440000, 399.709000 );
        booleng->AddPoint( 28237.910000, 394.381000 );
        booleng->AddPoint( 28239.290000, 394.763000 );
    }
    booleng->EndPolygonAdd(); 
}

void AddPolygonsToBoolEng3( Bool_Engine* booleng )
{
    // foreach point in a polygon ...
    if (booleng->StartPolygonAdd(GROUP_A))
    {
        booleng->AddPoint(100,100);
        booleng->AddPoint(-100,100); 
        booleng->AddPoint(-100,-100); 
        booleng->AddPoint(100,-100); 
    }
    booleng->EndPolygonAdd(); 

    // foreach point in a polygon ...
    if (booleng->StartPolygonAdd(GROUP_B))
    {
        booleng->AddPoint(50,50);
        booleng->AddPoint(-50,50);
        booleng->AddPoint(-50,-50);
        booleng->AddPoint(50,-50);
        booleng->EndPolygonAdd();
    }
    booleng->EndPolygonAdd(); 
}

void AddPolygonsToBoolEng4( Bool_Engine* booleng )
{
    // foreach point in a polygon ...
    if (booleng->StartPolygonAdd(GROUP_A))
    {
        booleng->AddPoint(0,0);
        booleng->AddPoint(0,1000); 
        booleng->AddPoint(1000,1000); 
        booleng->AddPoint(1000,0); 
    }
    booleng->EndPolygonAdd(); 
}

void GetPolygonsFromBoolEng( Bool_Engine* booleng )
{
    // foreach resultant polygon in the booleng ...
    while ( booleng->StartPolygonGet() )
    {
        // foreach point in the polygon
        while ( booleng->PolygonHasMorePoints() )
        {
            fprintf(stderr,"x = %f\t", booleng->GetPolygonXPoint());
            fprintf(stderr,"y = %f\n", booleng->GetPolygonYPoint());
        }
        booleng->EndPolygonGet();
    }
}

void GetPolygonsFromBoolEngKEY( Bool_Engine* booleng )
{
   FILE* file = fopen("keyfile.key", "w");

fprintf(file,"\
   HEADER 5; \
   BGNLIB; \
   LASTMOD {2-11-15  15:39:21}; \
   LASTACC {2-11-15  15:39:21}; \
   LIBNAME trial; \
   UNITS; \
   USERUNITS 0.0001; PHYSUNITS 2.54e-009; \
\
   BGNSTR;  \
   CREATION {2-11-15  15:39:21}; \
   LASTMOD  {2-11-15  15:39:21}; \
   STRNAME top; \
");
    // foreach resultant polygon in the booleng ...
    while ( booleng->StartPolygonGet() )
    {
        fprintf(file,"BOUNDARY; LAYER 2;  DATATYPE 0;\n");
        fprintf(file," XY %d; \n",booleng->GetNumPointsInPolygon()+1 );

        booleng->PolygonHasMorePoints();
        double firstx = booleng->GetPolygonXPoint();
        double firsty = booleng->GetPolygonYPoint();
        fprintf(file,"X %f;\t", firstx);
        fprintf(file,"Y %f; \n", firsty); 

        // foreach point in the polygon
        while ( booleng->PolygonHasMorePoints() )
        {
            fprintf(file,"X %f;\t", booleng->GetPolygonXPoint());
            fprintf(file,"Y %f; \n", booleng->GetPolygonYPoint()); 
        }
        booleng->EndPolygonGet();
        fprintf(file,"X %f;\t", firstx);
        fprintf(file,"Y %f; \n", firsty); 
        fprintf(file,"ENDEL;\n");
    }

fprintf(file,"\
   ENDSTR top; \
   ENDLIB; \
");
fclose (file);
}

int main()
{
    printf( "------------------------------------------------------\n" );
    printf( "|            Unit test of the KBool Engine           |\n" );
    printf( "------------------------------------------------------\n" );

    float correction;
    char a = '1';
    while (a != '0')
    {
         Bool_Engine* booleng = new Bool_Engine();
         ArmBoolEng( booleng );

         AddPolygonsToBoolEng( booleng );

	      printf( "\n***********************************\n" );
	      printf( "*** version: %s \n", booleng->GetVersion() );
	      printf( "***********************************\n" );
	      printf( "1: OR operation\n" );
	      printf( "2: AND operation\n" );
	      printf( "3: EXOR operation\n" );
	      printf( "4: A subtract B\n" );
	      printf( "5: B subtract A\n" );
	      printf( "6: Correct each polygon with a factor\n" );
	      printf( "7: Smoothen each polygon\n" );
	      printf( "8: Make a ring around each polygon\n" );
	      printf( "9: No operation\n" );
	      printf( "0: Quit\n" );
	      printf( "***********************************\n" );

	      printf( "type a number and <return>" );
          scanf( "%c", &a );

	      switch (a)
	      {
	      case ('0'):
	          break;
	      case ('1'):
	          booleng->Do_Operation(BOOL_OR);
	          break;
	      case ('2'):
	          booleng->Do_Operation(BOOL_AND);
	          break;
	      case ('3'):
	          booleng->Do_Operation(BOOL_EXOR);
	          break;
	      case ('4'):
	          booleng->Do_Operation(BOOL_A_SUB_B);
	          break;
	      case ('5'):
	          booleng->Do_Operation(BOOL_B_SUB_A);
	          break;
	      case ('6'):
              printf( "give correction factor (eg. 100.0 or -100.0)<return>:"); 
              scanf("%f", &correction ); // correct the polygons by this number
              booleng->SetCorrectionFactor( correction );
	          booleng->Do_Operation(BOOL_CORRECTION);
	          break;
	      case ('7'):
	          booleng->Do_Operation(BOOL_SMOOTHEN);
	          break;
	      case ('8'):
              printf("give width of ring <return>:");
              scanf("%f", &correction );
	          // create a ring of this size
              booleng->SetCorrectionFactor( fabs( correction / 2.0) );
	          booleng->Do_Operation(BOOL_MAKERING);
	          break;
	      case ('9'):
	          break;	      
          default:
	          break;

      }

	   if (a != '0')
	   {
	       printf("\nresulting polygons\n" );

           GetPolygonsFromBoolEng( booleng );

           //OR USE THIS
           //GetPolygonsFromBoolEngKEY( booleng );

	       printf( "\n\ntype a character and <return>");
           scanf( "%c", &a );
	   }
      delete booleng;
   }

   return 0;
}

