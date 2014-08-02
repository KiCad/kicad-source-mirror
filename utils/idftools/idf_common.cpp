/**
 * file: idf_common.cpp
 *
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


#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cstdio>
#include <cmath>
#include <idf_common.h>
#include <idf_helpers.h>

using namespace IDF3;
using namespace std;


std::string source;
std::string message;

IDF_ERROR::IDF_ERROR( const char* aSourceFile,
           const char* aSourceMethod,
           int         aSourceLine,
           const std::string& aMessage ) throw()
{
    ostringstream ostr;

    if( aSourceFile )
        ostr << "* " << aSourceFile << ":";
    else
        ostr << "* [BUG: No Source File]:";

    ostr << aSourceLine << ":";

    if( aSourceMethod )
        ostr << aSourceMethod << "(): ";
    else
        ostr << "[BUG: No Source Method]:\n* ";

    ostr << aMessage;
    message = ostr.str();

    return;
}


IDF_ERROR::~IDF_ERROR() throw()
{
    return;
}


const char* IDF_ERROR::what() const throw()
{
    return message.c_str();
}


IDF_NOTE::IDF_NOTE()
{
    xpos = 0.0;
    ypos = 0.0;
    height = 0.0;
    length = 0.0;
}


bool IDF_NOTE::readNote( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState,
                         IDF3::IDF_UNIT aBoardUnit )
{
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;

    // RECORD 2: X, Y, text Height, text Length, "TEXT"
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading board notes" ) );
    }

    if( isComment )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: comment within a section (NOTES)" ) );
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: X position in NOTES section must not be in quotes" ) );
    }

    if( CompareToken( ".END_NOTES", token ) )
        return false;

    istringstream istr;
    istr.str( token );

    istr >> xpos;
    if( istr.fail() )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: X position in NOTES section is not numeric" ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: Y position in NOTES section is missing" ) );
    }

    if( quoted )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: Y position in NOTES section must not be in quotes" ) );
    }

    istr.clear();
    istr.str( token );

    istr >> ypos;
    if( istr.fail() )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: Y position in NOTES section is not numeric" ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text height in NOTES section is missing" ) );
    }

    if( quoted )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text height in NOTES section must not be in quotes" ) );
    }

    istr.clear();
    istr.str( token );

    istr >> height;
    if( istr.fail() )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text height in NOTES section is not numeric" ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text length in NOTES section is missing" ) );
    }

    if( quoted )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text length in NOTES section must not be in quotes" ) );
    }

    istr.clear();
    istr.str( token );

    istr >> length;
    if( istr.fail() )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text length in NOTES section is not numeric" ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        aBoardState = IDF3::FILE_INVALID;
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDFv3 file\n"
                          "* Violation of specification: text value in NOTES section is missing" ) );
    }

    text = token;

    if( aBoardUnit == UNIT_THOU )
    {
        xpos *= IDF_THOU_TO_MM;
        ypos *= IDF_THOU_TO_MM;
        height *= IDF_THOU_TO_MM;
        length *= IDF_THOU_TO_MM;
    }

    return true;
}


bool IDF_NOTE::writeNote( std::ofstream& aBoardFile, IDF3::IDF_UNIT aBoardUnit )
{
    if( aBoardUnit == UNIT_THOU )
    {
        aBoardFile << setiosflags(ios::fixed) << setprecision(1)
                   << (xpos / IDF_THOU_TO_MM) << " "
                   << (ypos / IDF_THOU_TO_MM) << " "
                   << (height / IDF_THOU_TO_MM) << " "
                   << (length / IDF_THOU_TO_MM) << " ";
    }
    else
    {
        aBoardFile << setiosflags(ios::fixed) << setprecision(5)
                   << xpos << " " << ypos << " " << height << " " << length << " ";
    }

    aBoardFile << "\"" << text << "\"\n";

    return !aBoardFile.bad();
}


void IDF_NOTE::SetText( const std::string& aText )
{
    text = aText;
    return;
}

void IDF_NOTE::SetPosition( double aXpos, double aYpos )
{
    xpos = aXpos;
    ypos = aYpos;
    return;
}

void IDF_NOTE::SetSize( double aHeight, double aLength )
{
    height = aHeight;
    length = aLength;
    return;
}

const std::string& IDF_NOTE::GetText( void )
{
    return text;
}

void IDF_NOTE::GetPosition( double& aXpos, double& aYpos )
{
    aXpos = xpos;
    aYpos = ypos;
    return;
}

void IDF_NOTE::GetSize( double& aHeight, double& aLength )
{
    aHeight = height;
    aLength = length;
    return;
}


/*
 * CLASS: IDF_DRILL_DATA
 */
IDF_DRILL_DATA::IDF_DRILL_DATA()
{
    dia = 0.0;
    x = 0.0;
    y = 0.0;
    plating = NPTH;
    kref = NOREFDES;
    khole = MTG;
    owner = UNOWNED;

    return;
}


IDF_DRILL_DATA::IDF_DRILL_DATA( double aDrillDia, double aPosX, double aPosY,
                                IDF3::KEY_PLATING aPlating,
                                const std::string aRefDes,
                                const std::string aHoleType,
                                IDF3::KEY_OWNER aOwner )
{
    if( aDrillDia < 0.3 )
        dia = 0.3;
    else
        dia = aDrillDia;

    x = aPosX;
    y = aPosY;
    plating = aPlating;

    if( !aRefDes.compare( "BOARD" ) )
    {
        kref = BOARD;
    }
    else if( aRefDes.empty() || !aRefDes.compare( "NOREFDES" ) )
    {
        kref = NOREFDES;
    }
    else if( !aRefDes.compare( "PANEL" ) )
    {
        kref = PANEL;
    }
    else
    {
        kref = REFDES;
        refdes = aRefDes;
    }

    if( !aHoleType.compare( "PIN" ) )
    {
        khole = PIN;
    }
    else if( !aHoleType.compare( "VIA" ) )
    {
        khole = VIA;
    }
    else if( aHoleType.empty() || !aHoleType.compare( "MTG" ) )
    {
        khole = MTG;
    }
    else if( !aHoleType.compare( "TOOL" ) )
    {
        khole = TOOL;
    }
    else
    {
        khole = OTHER;
        holetype = aHoleType;
    }

    owner = aOwner;
}    // IDF_DRILL_DATA::IDF_DRILL_DATA( ... )

bool IDF_DRILL_DATA::Matches( double aDrillDia, double aPosX, double aPosY )
{
    double ddia = aDrillDia - dia;
    IDF_POINT p1, p2;

    p1.x = x;
    p1.y = y;
    p2.x = aPosX;
    p2.y = aPosY;

    if( ddia > -0.00001 && ddia < 0.00001 && p1.Matches( p2, 0.00001 ) )
        return true;

    return false;
}

bool IDF_DRILL_DATA::read( std::ifstream& aBoardFile, IDF3::IDF_UNIT aBoardUnit,
                           IDF3::FILE_STATE aBoardState, IDF3::IDF_VERSION aIdfVersion )
{
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;

    // RECORD 2: DIA, X, Y, Plating Style, REFDES, HOLE TYPE, HOLE OWNER
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "problems reading board drilled holes" ) );

    if( isComment )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: comment within a section (DRILLED HOLES)" ) );

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: drill diameter must not be in quotes" ) );

    if( CompareToken( ".END_DRILLED_HOLES", token ) )
        return false;

    istringstream istr;
    istr.str( token );

    istr >> dia;
    if( istr.fail() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: drill diameter is not numeric" ) );

    if( ( aBoardUnit == UNIT_MM && dia < IDF_MIN_DIA_MM )
        || ( aBoardUnit == UNIT_THOU && dia < IDF_MIN_DIA_THOU )
        || ( aBoardUnit == UNIT_TNM && dia < IDF_MIN_DIA_TNM ) )
    {
        ostringstream ostr;
        ostr << "invalid IDF file\n";
        ostr << "* Invalid drill diameter (too small): '" << token << "'";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: missing X position for drilled hole" ) );

    if( quoted )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: X position in DRILLED HOLES section must not be in quotes" ) );

    istr.clear();
    istr.str( token );

    istr >> x;
    if( istr.fail() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: X position in DRILLED HOLES section is not numeric" ) );

    if( !GetIDFString( iline, token, quoted, idx ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: missing Y position for drilled hole" ) );

    if( quoted )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: Y position in DRILLED HOLES section must not be in quotes" ) );

    istr.clear();
    istr.str( token );

    istr >> y;
    if( istr.fail() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: Y position in DRILLED HOLES section is not numeric" ) );

    if( aIdfVersion > IDF_V2 )
    {
        if( !GetIDFString( iline, token, quoted, idx ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDFv3 file\n"
                              "* Violation of specification: missing PLATING for drilled hole" ) );

            if( CompareToken( "PTH", token ) )
            {
                plating = IDF3::PTH;
            }
            else if( CompareToken( "NPTH", token ) )
            {
                plating = IDF3::NPTH;
            }
            else
            {
                ostringstream ostr;
                ostr << "invalid IDFv3 file\n";
                ostr << "* Violation of specification: invalid PLATING type ('" << token << "')";

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }
    }
    else
    {
        plating = IDF3::PTH;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
        {
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDFv3 file\n"
                              "* Violation of specification: missing REFDES for drilled hole" ) );
        }
        else
        {
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDFv2 file\n"
                              "* Violation of specification: missing HOLE TYPE for drilled hole" ) );
        }
    }

    std::string tok1 = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
        {
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDFv3 file\n"
                              "* Violation of specification: missing HOLE TYPE for drilled hole" ) );
        }
        else
        {
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDFv2 file\n"
                              "* Violation of specification: missing REFDES for drilled hole" ) );
        }
    }

    std::string tok2 = token;

    if( aIdfVersion > IDF_V2 )
        token = tok1;

    if( CompareToken( "BOARD", token ) )
    {
        kref = IDF3::BOARD;
    }
    else if( CompareToken( "NOREFDES", token ) )
    {
        kref = IDF3::NOREFDES;
    }
    else if( CompareToken( "PANEL", token ) )
    {
        kref = IDF3::PANEL;
    }
    else
    {
        kref = IDF3::REFDES;
        refdes = token;
    }

    if( aIdfVersion > IDF_V2 )
        token = tok2;
    else
        token = tok1;

    if( CompareToken( "PIN", token ) )
    {
        khole = IDF3::PIN;
    }
    else if( CompareToken( "VIA", token ) )
    {
        khole = IDF3::VIA;
    }
    else if( CompareToken( "MTG", token ) )
    {
        khole = IDF3::MTG;
    }
    else if( CompareToken( "TOOL", token ) )
    {
        khole = IDF3::TOOL;
    }
    else
    {
        khole = IDF3::OTHER;
        holetype = token;
    }

    if( aIdfVersion > IDF_V2 )
    {
        if( !GetIDFString( iline, token, quoted, idx ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDFv3 file\n"
                              "* Violation of specification: missing OWNER for drilled hole" ) );

            if( !ParseOwner( token, owner ) )
            {
                ostringstream ostr;
                ostr << "invalid IDFv3 file\n";
                ostr << "* Violation of specification: invalid OWNER for drilled hole ('" << token << "')";

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }
    }
    else
    {
        owner = IDF3::UNOWNED;
    }

    if( aBoardUnit == UNIT_THOU )
    {
        dia *= IDF_THOU_TO_MM;
        x *= IDF_THOU_TO_MM;
        y *= IDF_THOU_TO_MM;
    }
    else if( ( aIdfVersion == IDF_V2 ) && ( aBoardUnit == UNIT_TNM ) )
    {
        dia *= IDF_TNM_TO_MM;
        x *= IDF_TNM_TO_MM;
        y *= IDF_TNM_TO_MM;
    }
    else if( aBoardUnit != UNIT_MM )
    {
        ostringstream ostr;
        ostr << "\n* BUG: invalid UNIT type: " << aBoardUnit;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    return true;
}

void IDF_DRILL_DATA::write( std::ofstream& aBoardFile, IDF3::IDF_UNIT aBoardUnit )
{
    std::string holestr;
    std::string refstr;
    std::string ownstr;
    std::string pltstr;

    switch( khole )
    {
        case PIN:
            holestr = "PIN";
            break;

        case VIA:
            holestr = "VIA";
            break;

        case TOOL:
            holestr = "TOOL";
            break;

        case OTHER:
            holestr = "\"" + holetype + "\"";
            break;

        default:
            holestr = "MTG";
            break;
    }

    switch( kref )
    {
        case BOARD:
            refstr = "BOARD";
            break;

        case PANEL:
            refstr = "PANEL";
            break;

        case REFDES:
            refstr = "\"" + refdes + "\"";
            break;

        default:
            refstr = "NOREFDES";
            break;
    }

    if( plating == PTH )
        pltstr = "PTH";
    else
        pltstr = "NPTH";

    switch( owner )
    {
        case MCAD:
            ownstr = "MCAD";
            break;

        case ECAD:
            ownstr = "ECAD";
            break;

        default:
            ownstr = "UNOWNED";
            break;
    }

    if( aBoardUnit == UNIT_MM )
    {
        aBoardFile << std::setiosflags( std::ios::fixed ) << std::setprecision( 3 ) << dia << " "
            << std::setprecision( 5 ) << x << " " << y << " "
            << pltstr.c_str() << " " << refstr.c_str() << " "
            << holestr.c_str() << " " << ownstr.c_str() << "\n";
    }
    else
    {
        aBoardFile << std::setiosflags( std::ios::fixed ) << std::setprecision( 1 ) << (dia / IDF_THOU_TO_MM) << " "
        << std::setprecision( 1 ) << (x / IDF_THOU_TO_MM) << " " << (y / IDF_THOU_TO_MM) << " "
            << pltstr.c_str() << " " << refstr.c_str() << " "
            << holestr.c_str() << " " << ownstr.c_str() << "\n";
    }

    return;
}    // IDF_DRILL_DATA::Write( aBoardFile, unitMM )


double IDF_DRILL_DATA::GetDrillDia()
{
    return dia;
}

double IDF_DRILL_DATA::GetDrillXPos()
{
    return x;
}

double IDF_DRILL_DATA::GetDrillYPos()
{
    return y;
}

IDF3::KEY_PLATING IDF_DRILL_DATA::GetDrillPlating()
{
    return plating;
}

const std::string& IDF_DRILL_DATA::GetDrillRefDes()
{
    switch( kref )
    {
        case BOARD:
            refdes = "BOARD";
            break;

        case PANEL:
            refdes = "PANEL";
            break;

        case REFDES:
            break;

        default:
            refdes = "NOREFDES";
            break;
    }

    return refdes;
}

const std::string& IDF_DRILL_DATA::GetDrillHoleType()
{
    switch( khole )
    {
        case PIN:
            holetype = "PIN";
            break;

        case VIA:
            holetype = "VIA";
            break;

        case TOOL:
            holetype = "TOOL";
            break;

        case OTHER:
            break;

        default:
            holetype = "MTG";
            break;
    }

    return holetype;
}


#ifdef DEBUG_IDF
void IDF3::PrintSeg( IDF_SEGMENT* aSegment )
{
    if( aSegment->IsCircle() )
    {
        fprintf(stdout, "printSeg(): CIRCLE: C(%.3f, %.3f) P(%.3f, %.3f) rad. %.3f\n",
                aSegment->startPoint.x, aSegment->startPoint.y,
                aSegment->endPoint.x, aSegment->endPoint.y,
                aSegment->radius );
        return;
    }

    if( aSegment->angle < -MIN_ANG || aSegment->angle > MIN_ANG )
    {
        fprintf(stdout, "printSeg(): ARC: p1(%.3f, %.3f) p2(%.3f, %.3f) ang. %.3f\n",
                aSegment->startPoint.x, aSegment->startPoint.y,
                aSegment->endPoint.x, aSegment->endPoint.y,
                aSegment->angle );
        return;
    }

    fprintf(stdout, "printSeg(): LINE: p1(%.3f, %.3f) p2(%.3f, %.3f)\n",
            aSegment->startPoint.x, aSegment->startPoint.y,
            aSegment->endPoint.x, aSegment->endPoint.y );

    return;
}
#endif


bool IDF_POINT::Matches( const IDF_POINT& aPoint, double aRadius )
{
    double dx = x - aPoint.x;
    double dy = y - aPoint.y;

    double d2 = dx * dx + dy * dy;

    if( d2 <= aRadius * aRadius )
        return true;

    return false;
}


double IDF_POINT::CalcDistance( const IDF_POINT& aPoint ) const
{
    double dx   = aPoint.x - x;
    double dy   = aPoint.y - y;
    double dist = sqrt( dx * dx + dy * dy );

    return dist;
}


double IDF3::CalcAngleRad( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint )
{
    return atan2( aEndPoint.y - aStartPoint.y, aEndPoint.x - aStartPoint.x );
}


double IDF3::CalcAngleDeg( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint )
{
    double ang = CalcAngleRad( aStartPoint, aEndPoint );

    // round to thousandths of a degree
    int iang = int (ang / M_PI * 1800000.0);

    ang = iang / 10000.0;

    return ang;
}


void IDF3::GetOutline( std::list<IDF_SEGMENT*>& aLines,
                       IDF_OUTLINE& aOutline )
{
    aOutline.Clear();

    // NOTE: To tell if the point order is CCW or CW,
    // sum all:  (endPoint.X[n] - startPoint.X[n])*(endPoint[n] + startPoint.Y[n])
    // If the result is >0, the direction is CW, otherwise
    // it is CCW. Note that the result cannot be 0 unless
    // we have a bounded area of 0.

    // First we find the segment with the leftmost point
    std::list<IDF_SEGMENT*>::iterator bl    = aLines.begin();
    std::list<IDF_SEGMENT*>::iterator el    = aLines.end();
    std::list<IDF_SEGMENT*>::iterator idx   = bl++;       // iterator for the object with minX

    double minx = (*idx)->GetMinX();
    double curx;

    while( bl != el )
    {
        curx = (*bl)->GetMinX();

        if( curx < minx )
        {
            minx = curx;
            idx = bl;
        }

        ++bl;
    }

    aOutline.push( *idx );
#ifdef DEBUG_IDF
    PrintSeg( *idx );
#endif
    aLines.erase( idx );

    // If the item is a circle then we're done
    if( aOutline.front()->IsCircle() )
        return;

    // Assemble the loop
    bool complete = false;  // set if loop is complete
    bool matched;           // set if a segment's end point was matched

    while( !complete )
    {
        matched = false;
        bl  = aLines.begin();
        el  = aLines.end();

        while( bl != el && !matched )
        {
            if( (*bl)->MatchesStart( aOutline.back()->endPoint ) )
            {
                if( (*bl)->IsCircle() )
                {
                    // a circle on the perimeter is pathological but we just ignore it
                    ++bl;
                }
                else
                {
                    matched = true;
#ifdef DEBUG_IDF
                    PrintSeg( *bl );
#endif
                    aOutline.push( *bl );
                    aLines.erase( bl );
                }

                continue;
            }

            ++bl;
        }

        if( !matched )
        {
            // attempt to match the end points
            bl  = aLines.begin();
            el  = aLines.end();

            while( bl != el && !matched )
            {
                if( (*bl)->MatchesEnd( aOutline.back()->endPoint ) )
                {
                    if( (*bl)->IsCircle() )
                    {
                        // a circle on the perimeter is pathological but we just ignore it
                        ++bl;
                    }
                    else
                    {
                        matched = true;
                        (*bl)->SwapEnds();
#ifdef DEBUG_IDF
                        printSeg( *bl );
#endif
                        aOutline.push( *bl );
                        aLines.erase( bl );
                    }

                    continue;
                }

                ++bl;
            }
        }

        if( !matched )
        {
            // still no match - attempt to close the loop
            if( (aOutline.size() > 1) || ( aOutline.front()->angle < -MIN_ANG )
                || ( aOutline.front()->angle > MIN_ANG ) )
            {
                // close the loop
                IDF_SEGMENT* seg = new IDF_SEGMENT( aOutline.back()->endPoint,
                                                    aOutline.front()->startPoint );

                if( seg )
                {
                    complete = true;
#ifdef DEBUG_IDF
                    printSeg( seg );
#endif
                    aOutline.push( seg );
                    break;
                }
            }

            // the outline is bad; drop the segments
            aOutline.Clear();

            return;
        }

        // check if the loop is complete
        if( aOutline.front()->MatchesStart( aOutline.back()->endPoint ) )
        {
            complete = true;
            break;
        }
    }
}


IDF_SEGMENT::IDF_SEGMENT()
{
    angle = 0.0;
    offsetAngle = 0.0;
    radius = 0.0;
}


IDF_SEGMENT::IDF_SEGMENT( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint )
{
    angle = 0.0;
    offsetAngle = 0.0;
    radius = 0.0;
    startPoint = aStartPoint;
    endPoint = aEndPoint;
}


IDF_SEGMENT::IDF_SEGMENT( const IDF_POINT& aStartPoint,
                          const IDF_POINT& aEndPoint,
                          double aAngle,
                          bool aFromKicad )
{
    double diff = abs( aAngle ) - 360.0;

    if( ( diff < MIN_ANG
        && diff > -MIN_ANG ) || ( aAngle < MIN_ANG && aAngle > -MIN_ANG ) || (!aFromKicad) )
    {
        angle = 0.0;
        startPoint = aStartPoint;
        endPoint = aEndPoint;

        if( diff < MIN_ANG && diff > -MIN_ANG )
        {
            angle = 360.0;
            center = aStartPoint;
            offsetAngle = 0.0;
            radius = aStartPoint.CalcDistance( aEndPoint );
        }
        else if( aAngle > MIN_ANG || aAngle < -MIN_ANG )
        {
            angle = aAngle;
            CalcCenterAndRadius();
        }

        return;
    }

    // we need to convert from the KiCad arc convention
    angle = aAngle;

    center = aStartPoint;

    offsetAngle = IDF3::CalcAngleDeg( aStartPoint, aEndPoint );

    radius = aStartPoint.CalcDistance( aEndPoint );

    startPoint = aEndPoint;

    double ang = offsetAngle + aAngle;
    ang = (ang / 180.0) * M_PI;

    endPoint.x  = ( radius * cos( ang ) ) + center.x;
    endPoint.y  = ( radius * sin( ang ) ) + center.y;
}


bool IDF_SEGMENT::MatchesStart( const IDF_POINT& aPoint, double aRadius )
{
    return startPoint.Matches( aPoint, aRadius );
}


bool IDF_SEGMENT::MatchesEnd( const IDF_POINT& aPoint, double aRadius )
{
    return endPoint.Matches( aPoint, aRadius );
}


void IDF_SEGMENT::CalcCenterAndRadius( void )
{
    // NOTE:  this routine does not check if the points are the same
    // or too close to be sensible in a production setting.

    double offAng = IDF3::CalcAngleRad( startPoint, endPoint );
    double d = startPoint.CalcDistance( endPoint ) / 2.0;
    double xm   = ( startPoint.x + endPoint.x ) * 0.5;
    double ym   = ( startPoint.y + endPoint.y ) * 0.5;

    radius = d / sin( angle * M_PI / 360.0 );

    if( radius < 0.0 )
    {
        radius = -radius;
    }

    // calculate the height of the triangle with base d and hypotenuse r
    double dh2 = radius * radius - d * d;

    if( dh2 < 0 )
    {
        // this should only ever happen due to rounding errors when r == d
        dh2 = 0;
    }

    double h = sqrt( dh2 );

    if( angle > 0.0 )
        offAng += M_PI2;
    else
        offAng -= M_PI2;

    if( ( angle > 180.0 ) || ( angle < -180.0 ) )
        offAng += M_PI;

    center.x = h * cos( offAng ) + xm;
    center.y = h * sin( offAng ) + ym;

    offsetAngle = IDF3::CalcAngleDeg( center, startPoint );
}


bool IDF_SEGMENT::IsCircle( void )
{
    double diff = abs( angle ) - 360.0;

    if( ( diff < MIN_ANG ) && ( diff > -MIN_ANG ) )
        return true;

    return false;
}


double IDF_SEGMENT::GetMinX( void )
{
    if( angle == 0.0 )
        return std::min( startPoint.x, endPoint.x );

    // Calculate the leftmost point of the circle or arc

    if( IsCircle() )
    {
        // if only everything were this easy
        return center.x - radius;
    }

    // cases:
    // 1. CCW arc: if offset + included angle >= 180 deg then
    // MinX = center.x - radius, otherwise MinX is the
    // same as for the case of a line.
    // 2. CW arc: if offset + included angle <= -180 deg then
    // MinX = center.x - radius, otherwise MinX is the
    // same as for the case of a line.

    if( angle > 0 )
    {
        // CCW case
        if( ( offsetAngle + angle ) >= 180.0 )
        {
            return center.x - radius;
        }
        else
        {
            return std::min( startPoint.x, endPoint.x );
        }
    }

    // CW case
    if( ( offsetAngle + angle ) <= -180.0 )
    {
        return center.x - radius;
    }

    return std::min( startPoint.x, endPoint.x );
}


void IDF_SEGMENT::SwapEnds( void )
{
    if( IsCircle() )
    {
        // reverse the direction
        angle = -angle;
        return;
    }

    IDF_POINT tmp = startPoint;
    startPoint = endPoint;
    endPoint = tmp;

    if( ( angle < MIN_ANG ) && ( angle > -MIN_ANG ) )
        return;         // nothing more to do

        // change the direction of the arc
        angle = -angle;
    // calculate the new offset angle
    offsetAngle = IDF3::CalcAngleDeg( center, startPoint );
}


bool IDF_OUTLINE::IsCCW( void )
{
    // note: when outlines are not valid, 'false' is returned
    switch( outline.size() )
    {
    case 0:
        // no outline
        return false;
        break;

    case 1:
        // circles are always reported as CCW
        if( outline.front()->IsCircle() )
            return true;
        else
            return false;
        break;

    case 2:
        // we may have a closed outline consisting of:
        // 1. arc and line, winding depends on the arc
        // 2. 2 arcs, winding depends on larger arc
        {
            double a1 = outline.front()->angle;
            double a2 = outline.back()->angle;

            if( ( a1 < -MIN_ANG || a1 > MIN_ANG )
                && ( a2 < -MIN_ANG || a2 > MIN_ANG ) )
            {
                // we have 2 arcs
                if( abs( a1 ) >= abs( a2 ) )
                {
                    // winding depends on a1
                    if( a1 < 0.0 )
                        return false;
                    else
                        return true;
                }
                else
                {
                    if( a2 < 0.0 )
                        return false;
                    else
                        return true;
                }
            }

            // we may have a line + arc (or 2 lines)
            if( a1 < -MIN_ANG )
                return false;

            if( a1 > MIN_ANG )
                return true;

            if( a2 < -MIN_ANG )
                return false;

            if( a2 > MIN_ANG )
                return true;

            // we have 2 lines (invalid outline)
            return false;
        }
        break;

    default:
        break;
    }

    double winding = dir + ( outline.front()->startPoint.x - outline.back()->endPoint.x )
    * ( outline.front()->startPoint.y + outline.back()->endPoint.y );

    if( winding > 0.0 )
        return false;

    return true;
}


// returns true if the outline is a circle
bool IDF_OUTLINE::IsCircle( void )
{
    if( outline.front()->IsCircle() )
        return true;

    return false;
}


bool IDF_OUTLINE::push( IDF_SEGMENT* item )
{
    if( !outline.empty() )
    {
        if( item->IsCircle() )
        {
            // not allowed
            ERROR_IDF << "INVALID GEOMETRY\n";
            cerr << "* a circle is being added to a non-empty outline\n";
            return false;
        }
        else
        {
            if( outline.back()->IsCircle() )
            {
                // we can't add lines to a circle
                ERROR_IDF << "INVALID GEOMETRY\n";
                cerr << "* a line is being added to a circular outline\n";
                return false;
            }
            else if( !item->MatchesStart( outline.back()->endPoint ) )
            {
                // startPoint[N] != endPoint[N -1]
                ERROR_IDF << "INVALID GEOMETRY\n";
                cerr << "* disjoint segments (current start point != last end point)\n";
                cerr << "* start point: " << item->startPoint.x << ", " << item->startPoint.y << "\n";
                cerr << "* end point: " << outline.back()->endPoint.x << ", " << outline.back()->endPoint.y << "\n";
                return false;
            }
        }
    }

    outline.push_back( item );
    dir += ( outline.back()->endPoint.x - outline.back()->startPoint.x )
    * ( outline.back()->endPoint.y + outline.back()->startPoint.y );

    return true;
}
