/**
 * @file idf_common.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017  Cirilo Bernardo
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

    IDF_ERROR( const char* aSourceFile, const char* aSourceMethod, int aSourceLine,
               const std::string& aMessage ) noexcept;

    virtual ~IDF_ERROR() noexcept;

    virtual const char* what() const noexcept override;
};


namespace IDF3 {

/**
 * State values for the IDF parser's input.
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
 * The supported IDF versions (3.0 and 2.0  ONLY).
 */
enum IDF_VERSION
{
    IDF_V2 = 0, // version 2 has read support only; files written as IDFv3
    IDF_V3      // version 3 has full read/write support
};

/**
 * The type of CAD which has ownership an object.
 */
enum KEY_OWNER
{
    UNOWNED = 0, //< either MCAD or ECAD may modify a feature
    MCAD,        //< only MCAD may modify a feature
    ECAD         //< only ECAD may modify a feature
};

/**
 * The purpose of an IDF hole.
 */
enum KEY_HOLETYPE
{
    PIN = 0, //< drill hole is for a pin
    VIA,     //< drill hole is for a via
    MTG,     //< drill hole is for mounting
    TOOL,    //< drill hole is for tooling
    OTHER    //< user has specified a custom type
};

/**
 * The plating condition of a hole.
 */
enum KEY_PLATING
{
    PTH = 0, //< Plate-Through Hole
    NPTH     //< Non-Plate-Through Hole
};

/**
 * A component's Reference Designator.
 */
enum KEY_REFDES
{
    BOARD = 0, //< feature is associated with the board
    NOREFDES,  //< feature is associated with a component with no RefDes
    PANEL,     //< feature is associated with an IDF panel
    REFDES     //< reference designator as assigned by the CAD software
};

/**
 * The class of CAD program which is opening or modifying a file.
 */
enum CAD_TYPE
{
    CAD_ELEC = 0, //< An Electrical CAD is opening/modifying the file
    CAD_MECH,     //< A Mechanical CAD is opening/modifying the file
    CAD_INVALID
};

/**
 * The various IDF layer classes and groupings.
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
 * The class of outline.
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
 * Whether a component is a mechanical or electrical part.
 */
enum COMP_TYPE
{
    COMP_ELEC = 0, //< Component library object is an electrical part
    COMP_MECH,     //< Component library object is a mechanical part
    COMP_INVALID
};

/**
 * The native unit of the board and of component outlines.
 */
enum IDF_UNIT
{
    UNIT_MM = 0, //< Units in the file are in millimeters
    UNIT_THOU,   //< Units in the file are in mils (aka thou)
    UNIT_TNM,    //< Deprecated Ten Nanometer Units from IDFv2
    UNIT_INVALID
};

/**
 * The placement status of a component.
 */
enum IDF_PLACEMENT
{
    PS_UNPLACED = 0, //< component location on the board has not been specified
    PS_PLACED,       //< component location has been specified and may be modified by ECAD or MCAD
    PS_MCAD,         //< component location has been specified and may only be modified by MCAD
    PS_ECAD,         //< component location has been specified and may only be modified by ECAD
    PS_INVALID
};

/**
 * Calculate the angle (radians) between the horizon and the segment aStartPoint to aEndPoint.
 *
 * @param aStartPoint is the start point of a line segment.
 * @param aEndPoint is the end point of a line segment.
 * @return the angle in radians.
 */
double CalcAngleRad( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

/**
 * Calculate the angle (degrees) between the horizon and the segment aStartPoint to aEndPoint.
 *
 * @param aStartPoint is the start point of a line segment.
 * @param aEndPoint is the end point of a line segment.
 * @return the angle in degrees.
 */
double CalcAngleDeg( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

/**
 * Take contiguous elements from 'aLines' and stuffs them into 'aOutline'; elements put
 * into the outline are deleted from aLines.
 *
 * This function is useful for sorting the jumbled mess of line segments and arcs which represent
 * a board outline and cutouts in KiCad.  The function will determine which segment element within
 * aLines contains the leftmost point and retrieve the outline of which that segment is part.
 *
 * @param aLines (input/output) is a list of IDF segments which comprise an outline and cutouts.
 * @param aOutline (output) is the ordered set of segments/
 */
void GetOutline( std::list<IDF_SEGMENT*>& aLines, IDF_OUTLINE& aOutline );

#ifdef DEBUG_IDF
    // prints out segment information for debug purposes
    void PrintSeg( IDF_SEGMENT* aSegment );
#endif
}


/**
 * An entry in the NOTE section of an IDF file.
 */
class IDF_NOTE
{
public:
    IDF_NOTE();

    /**
     * Set the text to be stored as a NOTE entry.
     */
    void SetText( const std::string& aText );

    /**
     * Set the position (mm) of the NOTE entry.
     */
    void SetPosition( double aXpos, double aYpos );

    /**
     * Set the height and length (mm) of the NOTE entry.
     */
    void SetSize( double aHeight, double aLength );

    /**
     * @return the string stored in the note entry.
     */
    const std::string& GetText();

    /**
     * @return the position (mm) of the note entry.
     */
    void GetPosition( double& aXpos, double& aYpos );

    /**
     * @return the height and length (mm) of the note entry.
     */
    void GetSize( double& aHeight, double& aLength );

private:
    friend class IDF3_BOARD;

    /**
     * Read a note entry from an IDFv3 file.
     *
     * @param aBoardFile is an open BOARD file; the file position must be set to the start of
     *                   a NOTE entry.
     * @param aBoardState is the parser's current state value.
     * @param aBoardUnit is the BOARD file's native units (MM or THOU).
     * @return true if a note item was read, false otherwise.
     * @throw In case of unrecoverable errors.
     */
    bool readNote( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState,
                   IDF3::IDF_UNIT aBoardUnit );

    /**
     * Write a note entry to an IDFv3 file.
     *
     * @param aBoardFile is an open BOARD file; the file position must be within a NOTE section.
     * @param aBoardUnit is the BOARD file's native units (MM or THOU).
     * @return true if the item was successfully written, false otherwise.
     * @throw In case of unrecoverable error.
     */
    bool writeNote( std::ostream& aBoardFile, IDF3::IDF_UNIT aBoardUnit );

    std::string text;   // note text as per IDFv3
    double xpos;        // text X position as per IDFv3
    double ypos;        // text Y position as per IDFv3
    double height;      // text height as per IDFv3
    double length;      // text length as per IDFv3
};


/**
 * A drilled hole.
 *
 * Responsible for writing this information to a file in compliance with the IDFv3 specification.
 */
class IDF_DRILL_DATA
{
public:
    /**
     * Create an empty drill entry which can be populated by the read() function.
     */
    IDF_DRILL_DATA();

    /**
     * Create a drill entry with information compliant with the IDFv3 specifications.
     *
     * @param aDrillDia is the drill diameter.
     * @param aPosX is the X coordinate of the drill center.
     * @param aPosY is the Y coordinate of the drill center.
     * @param aPlating is a plating flag, PTH or NPTH.
     * @param aRefDes is the component Reference Designator.
     * @param aHoleType is the type of hole.
     * @param aOwner is one of MCAD, ECAD, UNOWNED.
     */
    IDF_DRILL_DATA( double aDrillDia, double aPosX, double aPosY,
                    IDF3::KEY_PLATING aPlating,
                    const std::string& aRefDes,
                    const std::string& aHoleType,
                    IDF3::KEY_OWNER aOwner );

    /**
     * Return true if the given drill diameter and location matches the diameter and location
     * of this IDF_DRILL_DATA object.
     *
     * @param aDrillDia is the drill diameter (mm).
     * @param aPosX is the X position (mm) of the drilled hole.
     * @param aPosY is the Y position (mm) of the drilled hole.
     * @return true if the diameter and position match this object.
     */
    bool Matches( double aDrillDia, double aPosX, double aPosY ) const;

    /**
     * @return the drill diameter in mm.
     */
    double GetDrillDia() const;

    /**
     * @return the drill's X position in mm.
     */
    double GetDrillXPos() const;

    /**
     * @return the drill's Y position in mm.
     */
    double GetDrillYPos() const;

    /**
     * @return the plating value (PTH, NPTH).
     */
    IDF3::KEY_PLATING GetDrillPlating();

    /**
     * @return the reference designator of the hole; this may be a component reference designator,
     *         BOARD, or NOREFDES as per IDFv3.
     */
    const std::string& GetDrillRefDes();

    /**
     * @return the classification of the hole; this may be one of PIN, VIA, MTG, TOOL, or a
     *         user-specified string.
     */
    const std::string& GetDrillHoleType();

    IDF3::KEY_OWNER GetDrillOwner() const
    {
        return owner;
    }

private:
    friend class IDF3_BOARD;
    friend class IDF3_COMPONENT;

    /**
     * Read a drill entry from an IDFv3 file
     *
     * @param aBoardFile is an open IDFv3 file; the file position must be within the DRILLED_HOLES
     *                   section.
     * @param aBoardUnit is the board file's native unit (MM or THOU).
     * @param aBoardState is the state value of the parser.
     * @return true if data was successfully read, otherwise false.
     * @throw in case of an unrecoverable error.
     */
    bool read( std::istream& aBoardFile, IDF3::IDF_UNIT aBoardUnit, IDF3::FILE_STATE aBoardState,
               IDF3::IDF_VERSION aIdfVersion );

    /**
     * Write a single line representing a hole within a .DRILLED_HOLES section.
     *
     * @param aBoardFile is an open BOARD file
     * @param aBoardUnit is the native unit of the output file
     * @throw in case of an unrecoverable error.
     */
    void write( std::ostream& aBoardFile, IDF3::IDF_UNIT aBoardUnit );

    double dia;
    double x;
    double y;
    IDF3::KEY_PLATING plating;
    IDF3::KEY_REFDES kref;
    IDF3::KEY_HOLETYPE khole;
    std::string refdes;
    std::string holetype;
    IDF3::KEY_OWNER owner;
};


/**
 * A point as used by the various IDF related classes.
 */
class IDF_POINT
{
public:
    IDF_POINT()
    {
        x = 0.0;
        y = 0.0;
    }

    IDF_POINT( double aX, double aY )
    {
        x = aX;
        y = aY;
    }

    /**
     * Return true if the given coordinate point is within the given radius of the point.
     *
     * @param aPoint is the coordinates of the point being compared.
     * @param aRadius is the radius (mm) within which the points are considered the same.
     * @return true if this point matches the given point.
     */
    bool Matches( const IDF_POINT& aPoint, double aRadius = 1e-5 ) const;

    /**
     * Return the Euclidean distance between this point and the given point.
     *
     * @param aPoint is the coordinates of the point whose distance is to be determined.
     * @return double is the distance between this point and aPoint.
     */
    double  CalcDistance( const IDF_POINT& aPoint ) const;

    double x;   // < X coordinate
    double y;   // < Y coordinate
};


/**
 * A segment as used in IDFv3 outlines.
 *
 * It may be any of an arc, line segment, or circle
 */
class IDF_SEGMENT
{
public:
    /**
     * Initialize the internal variables.
     */
    IDF_SEGMENT();

    /**
     * Create a straight segment.
     */
    IDF_SEGMENT( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint );

    /**
     * Create a straight segment, arc, or circle depending on the angle.
     *
     * @param aStartPoint is the start point (center if using KiCad convention, otherwise IDF
     *                    convention)
     * @param aEndPoint is the end point (start of arc if using KiCad convention, otherwise IDF
     *                  convention)
     * @param aAngle is the included angle; the KiCad convention is equivalent to the IDF convention
     * @param fromKicad set to true if we need to convert from KiCad to IDF convention.
     */
    IDF_SEGMENT( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint, double aAngle,
                 bool aFromKicad );

    /**
     * Return true if the given coordinate is within a radius 'rad' of the start point.
     *
     * @param aPoint are the coordinates of the point (mm) being compared.
     * @param aRadius is the radius (mm) within which the points are considered the same.
     * @return true if the given point matches the start point of this segment.
     */
    bool MatchesStart( const IDF_POINT& aPoint, double aRadius = 1e-3 );

    /**
     * Return true if the given coordinate is within a radius 'rad' of the end point.
     *
     * @param aPoint are the coordinates (mm) of the point being compared.
     * @param aRadius is the radius (mm) within which the points are considered the same.
     * @return true if the given point matches the end point of this segment.
     */
    bool MatchesEnd( const IDF_POINT& aPoint, double aRadius = 1e-3 );

    /**
     * @return true if this segment is a circle.
     */
    bool IsCircle();

    /**
     * @return the minimum X coordinate of this segment.
     */
    double GetMinX();

    /**
     * Swap the start and end points and alters internal variables as necessary for arcs.
     */
    void SwapEnds();

private:
    /**
     * Calculate the center, radius, and angle between center and start point given the
     * IDF compliant points and included angle.
     *
     * @var startPoint, @var endPoint, and @var angle must be set prior as per IDFv3.
     */
    void CalcCenterAndRadius();

public:
    IDF_POINT startPoint;   ///< starting point coordinates in mm
    IDF_POINT endPoint;     ///< end point coordinates in mm

    ///< center of an arc or circle; internally calculated and not to be set by the user.
    IDF_POINT center;
    double    angle;        ///< included angle (degrees) according to IDFv3 specification
    double    offsetAngle;  ///< angle between center and start of arc; internally calculated
    double    radius;       ///< radius of the arc or circle; internally calculated
};


/**
 * A segment and winding information for an IDF outline.
 */
class IDF_OUTLINE
{
public:
    IDF_OUTLINE() { dir = 0.0; }
    ~IDF_OUTLINE() { Clear(); }

    /**
     * @return true if the current list of points represents a counterclockwise winding.
     */
    bool IsCCW();

    /**
     * @returns true if this outline is a circle.
     */
    bool IsCircle();

    /**
     * Clear the internal list of outline segments.
     */
    void Clear()
    {
        dir = 0.0;

        while( !outline.empty() )
        {
            delete outline.front();
            outline.pop_front();
        }
    }

    /**
     * @return the size of the internal segment list.
     */
    size_t size()
    {
        return outline.size();
    }

    /**
     * @return true if the internal segment list is empty.
     */
    bool empty()
    {
        return outline.empty();
    }

    /**
     * @return the front() iterator of the internal segment list.
     */
    IDF_SEGMENT*& front()
    {
        return outline.front();
    }

    /**
     * @return the back() iterator of the internal segment list.
     */
    IDF_SEGMENT*& back()
    {
        return outline.back();
    }

    /**
     * @return the begin() iterator of the internal segment list.
     */
    std::list<IDF_SEGMENT*>::iterator begin()
    {
        return outline.begin();
    }

    /**
     * @return the end() iterator of the internal segment list.
     */
    std::list<IDF_SEGMENT*>::iterator end()
    {
        return outline.end();
    }

    /**
     * Add a segment to the internal segment list.
     *
     * Segments must be added in order so that startPoint[N] == endPoint[N - 1].
     *
     * @param item is a pointer to the segment to add to the outline.
     * @return true if the segment was added, otherwise false (outline restrictions have been
     *              violated).
     */
    bool push( IDF_SEGMENT* item );

private:
    double dir;                         // accumulator to help determine winding direction
    std::list<IDF_SEGMENT*> outline;    // sequential segments comprising an outline
};

#endif  // IDF_COMMON_H
