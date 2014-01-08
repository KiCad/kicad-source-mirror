/**
 * @file idf.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013  Cirilo Bernardo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef IDF_H
#define IDF_H

#include <wx/string.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841
#endif

#ifndef M_PI2
#define M_PI2 ( M_PI / 2.0 )
#endif

#ifndef M_PI4
#define M_PI4 ( M_PI / 4.0 )
#endif

class IDF_POINT;
class IDF_SEGMENT;
class IDF_DRILL_DATA;
class IDF_OUTLINE;

namespace IDF3 {
enum KEY_OWNER
{
    UNOWNED = 0,        // < either MCAD or ECAD may modify a feature
    MCAD,               // < only MCAD may modify a feature
    ECAD                // < only ECAD may modify a feature
};

enum KEY_HOLETYPE
{
    PIN = 0,            // < drill hole is for a pin
    VIA,                // < drill hole is for a via
    MTG,                // < drill hole is for mounting
    TOOL,               // < drill hole is for tooling
    OTHER               // < user has specified a custom type
};

enum KEY_PLATING
{
    PTH = 0,            // < Plate-Through Hole
    NPTH                // < Non-Plate-Through Hole
};

enum KEY_REFDES
{
    BOARD = 0,          // < feature is associated with the board
    NOREFDES,           // < feature is associated with a component with no RefDes
    PANEL,              // < feature is associated with an IDF panel
    REFDES              // < reference designator as assigned by the CAD software
};

// calculate the angle between the horizon and the segment aStartPoint to aEndPoint
double  CalcAngleRad( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );
double  CalcAngleDeg( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

// take contiguous elements from 'lines' and stuff them into 'outline'
void GetOutline( std::list<IDF_SEGMENT*>& aLines,
        IDF_OUTLINE& aOutline );
}


/**
 * @Class IDF_POINT
 * represents a point
 */
class IDF_POINT
{
public:
    double x;   // < X coordinate
    double y;   // < Y coordinate

    IDF_POINT()
    {
        x = 0.0;
        y = 0.0;
    }

    /**
     * Function Matches()
     * returns true if the given coordinate point is within the given radius
     * of the point.
     * @param aPoint : coordinates of the point being compared
     * @param aRadius : radius within which the points are considered the same
     */
    bool    Matches( const IDF_POINT& aPoint, double aRadius = 1e-5 );
    double  CalcDistance( const IDF_POINT& aPoint ) const;
};


/**
 * @Class IDF_SEGMENT
 * represents a geometry segment as used in IDFv3 outlines
 */
class IDF_SEGMENT
{
private:
    /**
     * Function CalcCenterAndRadius()
     * Calculates the center, radius, and angle between center and start point given the
     * IDF compliant points and included angle.
     * @var startPoint, @var endPoint, and @var angle must be set prior as per IDFv3
     */
    void CalcCenterAndRadius( void );

public:
    IDF_POINT startPoint;   // starting point in IDF coordinates
    IDF_POINT endPoint;     // end point in IDF coordinates
    IDF_POINT   center;     // center of an arc or circle; used primarily for calculating min X
    double  angle;          // included angle (degrees) according to IDFv3 specification
    double  offsetAngle;    // angle between center and start of arc; used to speed up some calcs.
    double  radius;         // radius of the arc or circle; used to speed up some calcs.

    /**
     * Function IDF_SEGMENT()
     * initializes the internal variables
     */
    IDF_SEGMENT();

    /**
     * Function IDF_SEGMENT( start, end )
     * creates a straight segment
     */
    IDF_SEGMENT( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

    /**
     * Function IDF_SEGMENT( start, end )
     * creates a straight segment, arc, or circle depending on the angle
     * @param aStartPoint : start point (center if using KiCad convention, otherwise IDF convention)
     * @param aEndPoint : end point (start of arc if using KiCad convention, otherwise IDF convention)
     * @param aAngle : included angle; the KiCad convention is equivalent to the IDF convention
     * @param fromKicad : set true if we need to convert from KiCad to IDF convention
     */
    IDF_SEGMENT( const IDF_POINT& aStartPoint,
            const IDF_POINT& aEndPoint,
            double aAngle,
            bool aFromKicad );

    /**
     * Function MatchesStart()
     * returns true if the given coordinate is within a radius 'rad'
     * of the start point.
     * @param aPoint : coordinates of the point being compared
     * @param aRadius : radius within which the points are considered the same
     */
    bool MatchesStart( const IDF_POINT& aPoint, double aRadius = 1e-3 );

    /**
     * Function MatchesEnd()
     * returns true if the given coordinate is within a radius 'rad'
     * of the end point.
     * @param aPoint : coordinates of the point being compared
     * @param aRadius : radius within which the points are considered the same
     */
    bool MatchesEnd( const IDF_POINT& aPoint, double aRadius = 1e-3 );

    /**
     * Function IsCircle()
     * returns true if this segment is a circle
     */
    bool IsCircle( void );

    /**
     * Function GetMinX()
     * returns the minimum X coordinate of this segment
     */
    double GetMinX( void );

    /**
     * Function SwapEnds()
     * Swaps the start and end points and alters internal
     * variables as necessary for arcs
     */
    void SwapEnds( void );
};


/**
 * @Class IDF_OUTLINE
 * contains segment and winding information for an IDF outline
 */
class IDF_OUTLINE
{
private:
    double dir;
    std::list<IDF_SEGMENT*> outline;

public:
    IDF_OUTLINE() { dir = 0.0; }
    ~IDF_OUTLINE() { Clear(); }

    // returns true if the current list of points represents a counterclockwise winding
    bool IsCCW( void )
    {
        if( dir > 0.0 )
            return false;

        return true;
    }

    // clears the internal list of outline segments
    void Clear( void )
    {
        dir = 0.0;

        while( !outline.empty() )
        {
            delete outline.front();
            outline.pop_front();
        }
    }

    // returns the size of the internal segment list
    size_t size( void )
    {
        return outline.size();
    }

    // returns true if the internal segment list is empty
    bool empty( void )
    {
        return outline.empty();
    }

    // return the front() of the internal segment list
    IDF_SEGMENT*& front( void )
    {
        return outline.front();
    }

    // return the back() of the internal segment list
    IDF_SEGMENT*& back( void )
    {
        return outline.back();
    }

    // return the begin() iterator of the internal segment list
    std::list<IDF_SEGMENT*>::iterator begin( void )
    {
        return outline.begin();
    }

    // return the end() iterator of the internal segment list
    std::list<IDF_SEGMENT*>::iterator end( void )
    {
        return outline.end();
    }

    // push a segment onto the internal list
    void push( IDF_SEGMENT* item )
    {
        // XXX - check that startPoint[N] == endPoint[N -1], otherwise THROW
        // XXX - a Circle must stand alone; if we add to a circle or add a
        // circle to an existing list, we should throw an exception.
        outline.push_back( item );
        dir += ( outline.back()->endPoint.x - outline.back()->startPoint.x )
               * ( outline.back()->endPoint.y + outline.back()->startPoint.y );
    }
};


/**
 * @Class IDF_BOARD
 * contains objects necessary for the maintenance of the IDF board and library files.
 */
class IDF_BOARD
{
private:
    std::list<IDF_DRILL_DATA*> drills;      ///< IDF drill data
    int outlineIndex;                       ///< next outline index to use
    bool useThou;                           ///< true if output is THOU
    double scale;                           ///< scale from KiCad IU to IDF output units
    double boardThickness;                  ///< total thickness of the PCB
    bool hasBrdOutlineHdr;                  ///< true when a board outline header has been written

    double offsetX;                         ///< offset to roughly center the board on the world origin
    double offsetY;

    FILE* layoutFile;                       ///< IDF board file (*.emn)
    FILE* libFile;                          ///< IDF library file (*.emp)

    /**
     * Function Write
     * outputs a .DRILLED_HOLES section compliant with the
     * IDFv3 specification.
     * @param aLayoutFile : open file (*.emn) for output
     */
    bool WriteDrills( void );

public:
    IDF_BOARD();

    ~IDF_BOARD();

    // Set up the output files and scale factor;
    // return TRUE if everything is OK
    bool Setup( wxString aBoardName, wxString aFullFileName, bool aUseThou, int aBoardThickness );

    // Finish a board
    // Write out all current data and close files.
    // Return true for success
    bool Finish( void );

    /**
     * Function GetScale
     * returns the output scaling factor
     */
    double GetScale( void );

    /**
     * Function SetOffset
     * sets the global coordinate offsets
     */
    void SetOffset( double x, double y );

    /**
     * Function GetOffset
     * returns the global coordinate offsets
     */
    void GetOffset( double& x, double& y );

    // Add an outline; the very first outline is the board perimeter;
    // all additional outlines are cutouts.
    bool AddOutline( IDF_OUTLINE& aOutline );

    /**
     * Function AddDrill
     * creates a drill entry and adds it to the list of PCB holes
     * @param dia : drill diameter
     * @param x : X coordinate of the drill center
     * @param y : Y coordinate of the drill center
     * @param plating : flag, PTH or NPTH
     * @param refdes : component Reference Designator
     * @param holetype : purpose of hole
     * @param owner : one of MCAD, ECAD, UNOWNED
     */
    bool AddDrill( double dia, double x, double y,
            IDF3::KEY_PLATING plating,
            const std::string refdes,
            const std::string holeType,
            IDF3::KEY_OWNER owner );

    /**
     * Function AddSlot
     * creates a slot cutout within the IDF BOARD section; this is a deficient representation
     * of a KiCad 'oval' drill; IDF is unable to represent a plated slot and unable to
     * represent the Reference Designator association with a slot.
     */
    bool AddSlot( double aWidth, double aLength, double aOrientation, double aX, double aY );
};


/**
 * @Class IDF_DRILL_DATA
 * contains information describing a drilled hole and is responsible for
 * writing this information to a file in compliance with the IDFv3 specification.
 */
class IDF_DRILL_DATA
{
private:
    double dia;
    double x;
    double y;
    IDF3::KEY_PLATING plating;
    IDF3::KEY_REFDES kref;
    IDF3::KEY_HOLETYPE khole;
    std::string refdes;
    std::string holetype;
    IDF3::KEY_OWNER owner;

public:
    /**
     * Constructor IDF_DRILL_DATA
     * creates a drill entry with information compliant with the
     * IDFv3 specifications.
     * @param aDrillDia : drill diameter
     * @param aPosX : X coordinate of the drill center
     * @param aPosY : Y coordinate of the drill center
     * @param aPlating : flag, PTH or NPTH
     * @param aRefDes : component Reference Designator
     * @param aHoleType : purpose of hole
     * @param aOwner : one of MCAD, ECAD, UNOWNED
     */
    IDF_DRILL_DATA( double aDrillDia, double aPosX, double aPosY,
            IDF3::KEY_PLATING aPlating,
            const std::string aRefDes,
            const std::string aHoleType,
            IDF3::KEY_OWNER aOwner );

    /**
     * Function Write
     * writes a single line representing the hole within a .DRILLED_HOLES section
     */
    bool Write( FILE* aLayoutFile );
};


/**
 * @Class IDF_LIB
 * stores information on IDF models ( also has an inbuilt NOMODEL model )
 * and is responsible for writing the ELECTRICAL sections of the library file
 * (*.emp) and the PLACEMENT section of the board file.
 */
class IDF_LIB
{
    // TODO: IMPLEMENT

public:
    /**
     * Function WriteLib
     * writes all current library information to the output file
     */
    bool WriteLib( FILE* aLibFile );

    // write placement information to the board file
    bool WriteBrd( FILE* aLayoutFile );

    // bool Finish( void )
    // {
    // TODO: Write out the library (*.emp) file
    // idf_lib.Write( lib_file );
    // TODO: fclose( lib_file );
    // }
};

#endif  // IDF_H
