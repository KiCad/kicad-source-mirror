/**
 * @file idf_common.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014  Cirilo Bernardo
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

#ifndef IDF_COMMON_H
#define IDF_COMMON_H

#include <list>
#include <fstream>
#include <exception>
#include <string>
#include <cmath>

// differences in angle smaller than MIN_ANG are considered equal
#define MIN_ANG     (0.01)

class IDF_POINT;
class IDF_SEGMENT;
class IDF_DRILL_DATA;
class IDF_OUTLINE;
class IDF_LIB;


struct IDF_ERROR : std::exception
{
    std::string message;

    IDF_ERROR( const char* aSourceFile,
               const char* aSourceMethod,
               int         aSourceLine,
               const std::string& aMessage ) throw();

    virtual ~IDF_ERROR() throw();

    virtual const char* what() const throw();
};


namespace IDF3 {

    /**
     * ENUM FILE_STATE
     * represents state values for the IDF parser's input
     */
    enum FILE_STATE
    {
        FILE_START = 0, // no data has been read; expecting .HEADER
        FILE_HEADER,    // header has been read; expecting  .BOARD_OUTLINE
        FILE_OUTLINE,   // board outline has been read; most sections can be accepted
        FILE_PLACEMENT, // placement has been read; no further sections can be accepted
        FILE_INVALID,   // file is invalid
        FILE_ERROR      // other errors while processing the file
    };

    /**
     * ENUM IDF_VERSION
     * represents the supported IDF versions (3.0 and 2.0  ONLY)
     */
    enum IDF_VERSION
    {
        IDF_V2 = 0,     // version 2 has read support only; files written as IDFv3
        IDF_V3          // version 3 has full read/write support
    };

    /**
     * ENUM KEY_OWNER
     * represents the type of CAD which has ownership an object
     */
    enum KEY_OWNER
    {
        UNOWNED = 0,        //< either MCAD or ECAD may modify a feature
        MCAD,               //< only MCAD may modify a feature
        ECAD                //< only ECAD may modify a feature
    };

    /**
     * ENUM KEY_HOLETYPE
     * represents the purpose of an IDF hole
     */
    enum KEY_HOLETYPE
    {
        PIN = 0,            //< drill hole is for a pin
        VIA,                //< drill hole is for a via
        MTG,                //< drill hole is for mounting
        TOOL,               //< drill hole is for tooling
        OTHER               //< user has specified a custom type
    };

    /**
     * ENUM KEY_PLATING
     * represents the plating condition of a hole
     */
    enum KEY_PLATING
    {
        PTH = 0,            //< Plate-Through Hole
        NPTH                //< Non-Plate-Through Hole
    };

    /**
     * ENUM KEY_REFDES
     * represents a component's Reference Designator
     */
    enum KEY_REFDES
    {
        BOARD = 0,          //< feature is associated with the board
        NOREFDES,           //< feature is associated with a component with no RefDes
        PANEL,              //< feature is associated with an IDF panel
        REFDES              //< reference designator as assigned by the CAD software
    };

    /**
     * ENUM CAD_TYPE
     * represents the class of CAD program which is opening or modifying a file
     */
    enum CAD_TYPE
    {
        CAD_ELEC = 0,       //< An Electrical CAD is opening/modifying the file
        CAD_MECH,           //< A Mechanical CAD is opening/modifying the file
        CAD_INVALID
    };

    /**
     * ENUM IDF_LAYER
     * represents the various IDF layer classes and groupings
     */
    enum IDF_LAYER
    {
        LYR_TOP = 0,
        LYR_BOTTOM,
        LYR_BOTH,
        LYR_INNER,
        LYR_ALL,
        LYR_INVALID
    };

    /**
     * ENUM OUTLINE_TYPE
     * identifies the class of outline
     */
    enum OUTLINE_TYPE
    {
        OTLN_BOARD = 0,
        OTLN_OTHER,
        OTLN_PLACE,
        OTLN_ROUTE,
        OTLN_PLACE_KEEPOUT,
        OTLN_ROUTE_KEEPOUT,
        OTLN_VIA_KEEPOUT,
        OTLN_GROUP_PLACE,
        OTLN_COMPONENT,
        OTLN_INVALID
    };

    /**
     * ENUM COMP_TYPE
     * identifies whether a component is a mechanical or electrical part
     */
    enum COMP_TYPE
    {
        COMP_ELEC = 0,      //< Component library object is an electrical part
        COMP_MECH,          //< Component library object is a mechanical part
        COMP_INVALID
    };

    /**
     * ENUM IDF_UNIT
     * represents the native unit of the board and of component outlines
     */
    enum IDF_UNIT
    {
        UNIT_MM = 0,        //< Units in the file are in millimeters
        UNIT_THOU,          //< Units in the file are in mils (aka thou)
        UNIT_TNM,           //< Deprecated Ten Nanometer Units from IDFv2
        UNIT_INVALID
    };

    /**
     * ENUM IDF_PLACEMENT
     * represents the placement status of a component
     */
    enum IDF_PLACEMENT
    {
        PS_UNPLACED = 0,    //< component location on the board has not been specified
        PS_PLACED,          //< component location has been specified and may be modified by ECAD or MCAD
        PS_MCAD,            //< component location has been specified and may only be modified by MCAD
        PS_ECAD,            //< component location has been specified and may only be modified by ECAD
        PS_INVALID
    };

    /**
     * Function CalcAngleRad
     * calculates the angle (radians) between the horizon and the segment aStartPoint to aEndPoint
     *
     * @param aStartPoint is the start point of a line segment
     * @param aEndPoint is the end point of a line segment
     *
     * @return double: the angle in radians
     */
    double  CalcAngleRad( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );


    /**
     * Function CalcAngleDeg
     * calculates the angle (degrees) between the horizon and the segment aStartPoint to aEndPoint
     *
     * @param aStartPoint is the start point of a line segment
     * @param aEndPoint is the end point of a line segment
     *
     * @return double: the angle in degrees
     */
    double  CalcAngleDeg( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

    /**
     * Function GetOutline
     * takes contiguous elements from 'aLines' and stuffs them into 'aOutline'; elements put
     * into the outline are deleted from aLines. This function is useful for sorting the jumbled
     * mess of line segments and arcs which represent a board outline and cutouts in KiCad.
     * The function will determine which segment element within aLines contains the leftmost
     * point and retrieve the outline of which that segment is part.
     *
     * @param aLines (input/output) is a list of IDF segments which comprise an outline and
     *        cutouts.
     * @param aOutline (output) is the ordered set of segments
     */
    void GetOutline( std::list<IDF_SEGMENT*>& aLines,
                     IDF_OUTLINE& aOutline );

#ifdef DEBUG_IDF
    // prints out segment information for debug purposes
    void PrintSeg( IDF_SEGMENT* aSegment );
#endif
}


/**
 * Class IDF_NOTE
 * represents an entry in the NOTE section of an IDF file
 */
class IDF_NOTE
{
friend class IDF3_BOARD;
private:
    std::string text;   // note text as per IDFv3
    double xpos;        // text X position as per IDFv3
    double ypos;        // text Y position as per IDFv3
    double height;      // text height as per IDFv3
    double length;      // text length as per IDFv3

    /**
     * Function readNote
     * reads a note entry from an IDFv3 file
     *
     * @param aBoardFile is an open BOARD file; the file position must be set to the start of a NOTE entry
     * @param aBoardState is the parser's current state value
     * @param aBoardUnit is the BOARD file's native units (MM or THOU)
     *
     * @return bool: true if a note item was read, false otherwise. In case of unrecoverable errors
     * an exception is thrown
     */
    bool readNote( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState, IDF3::IDF_UNIT aBoardUnit );

    /**
     * Function writeNote
     * writes a note entry to an IDFv3 file
     *
     * @param aBoardFile is an open BOARD file; the file position must be within a NOTE section
     * @param aBoardUnit is the BOARD file's native units (MM or THOU)
     *
     * @return bool: true if the item was successfully written, false otherwise. In case of
     * unrecoverable errors an exception is thrown
     */
    bool writeNote( std::ofstream& aBoardFile, IDF3::IDF_UNIT aBoardUnit );

public:
    IDF_NOTE();

    /**
     * Function SetText
     * sets the text to be stored as a NOTE entry
     */
    void SetText( const std::string& aText );

    /**
     * Function SetPosition
     * sets the position (mm) of the NOTE entry
     */
    void SetPosition( double aXpos, double aYpos );

    /**
     * Function SetSize
     * sets the height and length (mm) of the NOTE entry
     */
    void SetSize( double aHeight, double aLength );

    /**
     * Function GetText
     * returns the string stored in the note entry
     */
    const std::string& GetText( void );

    /**
     * Function GetText
     * returns the position (mm) of the note entry
     */
    void GetPosition( double& aXpos, double& aYpos );

    /**
     * Function GetText
     * returns the height and length (mm) of the note entry
     */
    void GetSize( double& aHeight, double& aLength );
};


/**
 * @Class IDF_DRILL_DATA
 * contains information describing a drilled hole and is responsible for
 * writing this information to a file in compliance with the IDFv3 specification.
 */
class IDF_DRILL_DATA
{
friend class IDF3_BOARD;
friend class IDF3_COMPONENT;
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

    /**
     * Function read
     * read a drill entry from an IDFv3 file
     *
     * @param aBoardFile is an open IDFv3 file; the file position must be within the DRILLED_HOLES section
     * @param aBoardUnit is the board file's native unit (MM or THOU)
     * @param aBoardState is the state value of the parser
     *
     * @return bool: true if data was successfully read, otherwise false. In case of an
     * unrecoverable error an exception is thrown
     */
    bool read( std::ifstream& aBoardFile, IDF3::IDF_UNIT aBoardUnit, IDF3::FILE_STATE aBoardState,
               IDF3::IDF_VERSION aIdfVersion );

    /**
     * Function write
     * writes a single line representing a hole within a .DRILLED_HOLES section
     * In case of an unrecoverable error an exception is thrown.
     *
     * @param aBoardFile is an open BOARD file
     * @param aBoardUnit is the native unit of the output file
     */
    void write( std::ofstream& aBoardFile, IDF3::IDF_UNIT aBoardUnit );

public:
    /**
     * Constructor IDF_DRILL_DATA
     * creates an empty drill entry which can be populated by the
     * read() function
     */
    IDF_DRILL_DATA();

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
     * Function Matches
     * returns true if the given drill diameter and location
     * matches the diameter and location of this IDF_DRILL_DATA object
     *
     * @param aDrillDia is the drill diameter (mm)
     * @param aPosX is the X position (mm) of the drilled hole
     * @param aPosY is the Y position (mm) of the drilled hole
     *
     * @return bool: true if the diameter and position match this object
     */
    bool Matches( double aDrillDia, double aPosX, double aPosY );

    /**
     * Function GettDrillDia
     * returns the drill diameter in mm
     */
    double GetDrillDia();

    /**
     * Function GettDrillXPos
     * returns the drill's X position in mm
     */
    double GetDrillXPos();

    /**
     * Function GettDrillYPos
     * returns the drill's Y position in mm
     */
    double GetDrillYPos();

    /**
     * Function GetDrillPlating
     * returns the plating value (PTH, NPTH)
     */
    IDF3::KEY_PLATING GetDrillPlating();

    /**
     * Function GetDrillRefDes
     * returns the reference designator of the hole; this
     * may be a component reference designator, BOARD, or
     * NOREFDES as per IDFv3.
     */
    const std::string& GetDrillRefDes();

    /**
     * Function GetDrillHoleType
     * returns the classification of the hole; this may be one of
     * PIN, VIA, MTG, TOOL, or a user-specified string
     */
    const std::string& GetDrillHoleType();

    IDF3::KEY_OWNER GetDrillOwner( void )
    {
        return owner;
    }
};


/**
 * @Class IDF_POINT
 * represents a point as used by the various IDF related classes
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
     *
     * @param aPoint : coordinates of the point being compared
     * @param aRadius : radius (mm) within which the points are considered the same
     *
     * @return bool: true if this point matches the given point
     */
    bool    Matches( const IDF_POINT& aPoint, double aRadius = 1e-5 );

    /**
     * Function CalcDistance()
     * returns the Euclidean distance between this point and the given point
     *
     * @param aPoint : coordinates of the point whose distance is to be determined
     *
     * @return double: distance between this point and aPoint
     */
    double  CalcDistance( const IDF_POINT& aPoint ) const;
};


/**
 * @Class IDF_SEGMENT
 * represents a geometry segment as used in IDFv3 outlines; it may be any of
 * an arc, line segment, or circle
 */
class IDF_SEGMENT
{
private:
    /**
     * Function CalcCenterAndRadius()
     * Calculates the center, radius, and angle between center and start point given the
     * IDF compliant points and included angle.
     *
     * @var startPoint, @var endPoint, and @var angle must be set prior as per IDFv3
     */
    void CalcCenterAndRadius( void );

public:
    IDF_POINT startPoint;   ///< starting point coordinates in mm
    IDF_POINT endPoint;     ///< end point coordinates in mm
    IDF_POINT   center;     ///< center of an arc or circle; internally calculated and not to be set by the user
    double  angle;          ///< included angle (degrees) according to IDFv3 specification
    double  offsetAngle;    ///< angle between center and start of arc; internally calculated
    double  radius;         ///< radius of the arc or circle; internally calculated

    /**
     * Constructor IDF_SEGMENT
     * initializes the internal variables
     */
    IDF_SEGMENT();

    /**
     * Function IDF_SEGMENT
     * creates a straight segment
     */
    IDF_SEGMENT( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

    /**
     * Constructor IDF_SEGMENT
     * creates a straight segment, arc, or circle depending on the angle
     *
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
     * Function MatchesStart
     * returns true if the given coordinate is within a radius 'rad'
     * of the start point.
     *
     * @param aPoint : coordinates of the point (mm) being compared
     * @param aRadius : radius (mm) within which the points are considered the same
     *
     * @return bool: true if the given point matches the start point of this segment
     */
    bool MatchesStart( const IDF_POINT& aPoint, double aRadius = 1e-3 );

    /**
     * Function MatchesEnd
     * returns true if the given coordinate is within a radius 'rad'
     * of the end point.
     *
     * @param aPoint : coordinates (mm) of the point being compared
     * @param aRadius : radius (mm) within which the points are considered the same
     *
     * @return bool: true if the given point matches the end point of this segment
     */
    bool MatchesEnd( const IDF_POINT& aPoint, double aRadius = 1e-3 );

    /**
     * Function IsCircle
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
    double dir;                         // accumulator to help determine winding direction
    std::list<IDF_SEGMENT*> outline;    // sequential segments comprising an outline

public:
    IDF_OUTLINE() { dir = 0.0; }
    ~IDF_OUTLINE() { Clear(); }

    /**
     * Function IsCCW
     * returns true if the current list of points represents a counterclockwise winding
     */
    bool IsCCW( void );

    /**
     * Function IsCircle
     * returns true if this outline is a circle
     */
    bool IsCircle( void );

    /**
     * Function Clear
     * clears the internal list of outline segments
     */
    void Clear( void )
    {
        dir = 0.0;

        while( !outline.empty() )
        {
            delete outline.front();
            outline.pop_front();
        }
    }

    /**
     * Function size
     * returns the size of the internal segment list
     */
    size_t size( void )
    {
        return outline.size();
    }

    /**
     * Function empty
     * returns true if the internal segment list is empty
     */
    bool empty( void )
    {
        return outline.empty();
    }

    /**
     * Function front
     * returns the front() iterator of the internal segment list
     */
    IDF_SEGMENT*& front( void )
    {
        return outline.front();
    }

    /**
     * Function back
     * returns the back() iterator of the internal segment list
     */
    IDF_SEGMENT*& back( void )
    {
        return outline.back();
    }

    /**
     * Function begin
     * returns the begin() iterator of the internal segment list
     */
    std::list<IDF_SEGMENT*>::iterator begin( void )
    {
        return outline.begin();
    }

    /**
     * Function end
     * returns the end() iterator of the internal segment list
     */
    std::list<IDF_SEGMENT*>::iterator end( void )
    {
        return outline.end();
    }

    /**
     * Function push
     * adds a segment to the internal segment list; segments must be added
     * in order so that startPoint[N] == endPoint[N - 1]
     *
     * @param item is a pointer to the segment to add to the outline
     *
     * @return bool: true if the segment was added, otherwise false
     * (outline restrictions have been violated)
     */
    bool push( IDF_SEGMENT* item );
};

#endif  // IDF_COMMON_H
