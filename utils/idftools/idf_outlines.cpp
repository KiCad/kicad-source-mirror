/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

#include <idf_helpers.h>
#include <idf_outlines.h>
#include <idf_parser.h>

using namespace IDF3;
using namespace std;


/*
 * CLASS: BOARD OUTLINE
 */
BOARD_OUTLINE::BOARD_OUTLINE()
{
    outlineType = OTLN_BOARD;
    single = false;
    owner = UNOWNED;
    parent = NULL;
    thickness = 0.0;
    unit = UNIT_MM;
    return;
}

BOARD_OUTLINE::~BOARD_OUTLINE()
{
    Clear();
    return;
}

IDF3::OUTLINE_TYPE BOARD_OUTLINE::GetOutlineType( void )
{
    return outlineType;
}

bool BOARD_OUTLINE::readOutlines( std::ifstream& aBoardFile )
{
    // reads the outline data from a file
    double x, y, ang;
    double dLoc  = 1e-5;    // distances are equal when closer than 0.1 micron
    bool comment = false;
    bool quoted  = false;
    bool closed  = false;
    int idx      = 0;
    int loopidx  = -1;
    int tmp      = 0;
    int npts     = 0;
    std::string iline;
    std::string entry;
    std::stringstream tstr;
    IDF_OUTLINE* op = NULL;
    IDF_SEGMENT* sp = NULL;
    IDF_POINT prePt;
    IDF_POINT curPt;
    std::streampos pos;

    // destroy any existing outline data
    ClearOutlines();

    while( aBoardFile.good() )
    {
        if( !FetchIDFLine( aBoardFile, iline, comment, pos ) )
            continue;

        idx = 0;
        GetIDFString( iline, entry, quoted, idx );

        if( quoted )
        {
            ERROR_IDF << "invalid outline; FIELD 1 is quoted\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        // check for the end of the section
        if( entry.size() >= 5 && CompareToken( ".END_", entry.substr( 0, 5 ) ) )
        {
            // rewind to the start of the last line; the routine invoking
            // this is responsible for checking that the current '.END_ ...'
            // matches the section header.
            aBoardFile.seekg( pos );

            if( outlines.size() > 0 )
            {
                if( npts > 0 && !closed )
                {
                    ERROR_IDF << "invalid outline (not closed)\n";
                    return false;
                }

                // verify winding
                if( !single )
                {
                    if( !outlines.front()->IsCCW() )
                    {
                        ERROR_IDF << "invalid IDF3 file (BOARD_OUTLINE)\n";
                        cerr << "* first outline is not in CCW order\n";
//#warning TO BE IMPLEMENTED
                        // outlines.front()->EnsureWinding( false );
                        return true;
                    }

                    if( outlines.size() > 1 && outlines.back()->IsCCW() && !outlines.back()->IsCircle() )
                    {
                        ERROR_IDF << "invalid IDF3 file (BOARD_OUTLINE)\n";
                        cerr << "* cutout points are not in CW order\n";
//#warning TO BE IMPLEMENTED
                        // outlines.front()->EnsureWinding( true );
                        return true;
                    }
                }
            }

            return true;
        }

        tstr.clear();
        tstr << entry;

        tstr >> tmp;
        if( tstr.fail() )
        {
            if( outlineType == OTLN_COMPONENT && CompareToken( "PROP", entry ) )
            {
                aBoardFile.seekg( pos );
                return true;
            }

            ERROR_IDF << "invalid outline; FIELD 1 is not numeric\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( tmp != loopidx )
        {
            // index change

            if( tmp < 0 )
            {
                ERROR_IDF << "invalid outline; FIELD 1 is invalid\n";
                std::cerr << "    LINE: " << iline << "\n";
                return false;
            }

            if( loopidx == -1 )
            {
                // first outline
                if( single )
                {
                    // outline may have a Loop Index of 0 or 1
                    if( tmp == 0 || tmp == 1 )
                    {
                        op = new IDF_OUTLINE;
                        if( op == NULL )
                        {
                            ERROR_IDF << "memory allocation failed\n";
                            return false;
                        }
                        outlines.push_back( op );
                        loopidx = tmp;
                    }
                    else
                    {
                        ERROR_IDF << "invalid outline; FIELD 1 is invalid (must be 0 or 1)\n";
                        std::cerr << "    LINE: " << iline << "\n";
                        return false;
                    }
                }
                else
                {
                    // outline *MUST* have a Loop Index of 0
                    if( tmp != 0 )
                    {
                        ERROR_IDF << "invalid outline; first outline of a BOARD or PANEL must have a Loop Index of 0\n";
                        std::cerr << "    LINE: " << iline << "\n";
                        return false;
                    }

                    op = new IDF_OUTLINE;

                    if( op == NULL )
                    {
                        ERROR_IDF << "memory allocation failed\n";
                        return false;
                    }

                    outlines.push_back( op );
                    loopidx = tmp;
                }
                // end of block for first outline
            }
            else
            {
                // outline for cutout
                if( single )
                {
                    ERROR_IDF << "invalid outline; a simple outline type may only have one outline\n";
                    std::cerr << "    LINE: " << iline << "\n";
                    return false;
                }

                if( tmp - loopidx != 1 )
                {
                    ERROR_IDF << "invalid outline; cutouts must be numbered in order from 1 onwards\n";
                    std::cerr << "    LINE: " << iline << "\n";
                    return false;
                }

                // verify winding of previous outline
                if( ( loopidx = 0 && !op->IsCCW() )
                    || ( loopidx > 0 && op->IsCCW() ) )
                {
                    ERROR_IDF << "invalid outline (violation of loop point order rules by Loop Index "
                    << loopidx << ")\n";
                    return false;
                }

                op = new IDF_OUTLINE;

                if( op == NULL )
                {
                    ERROR_IDF << "memory allocation failed\n";
                    return false;
                }

                outlines.push_back( op );
                loopidx = tmp;
            }
            // end of index change code
            npts = 0;
            closed = false;
        }

        if( op == NULL )
        {
            ERROR_IDF << "invalid outline; FIELD 1 is invalid\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( !GetIDFString( iline, entry, quoted, idx ) )
        {
            ERROR_IDF << "invalid RECORD 3, FIELD 2 does not exist\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( quoted )
        {
            ERROR_IDF << "invalid RECORD 3, FIELD 2 is quoted\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        tstr.clear();
        tstr << entry;

        tstr >> x;
        if( tstr.fail() )
        {
            ERROR_IDF << "invalid RECORD 3, invalid X value in FIELD 2\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( !GetIDFString( iline, entry, quoted, idx ) )
        {
            ERROR_IDF << "invalid RECORD 3, FIELD 3 does not exist\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( quoted )
        {
            ERROR_IDF << "invalid RECORD 3, FIELD 3 is quoted\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        tstr.clear();
        tstr << entry;

        tstr >> y;
        if( tstr.fail() )
        {
            ERROR_IDF << "invalid RECORD 3, invalid Y value in FIELD 3\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( !GetIDFString( iline, entry, quoted, idx ) )
        {
            ERROR_IDF << "invalid RECORD 3, FIELD 4 does not exist\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        if( quoted )
        {
            ERROR_IDF << "invalid RECORD 3, FIELD 4 is quoted\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        tstr.clear();
        tstr << entry;

        tstr >> ang;
        if( tstr.fail() )
        {
            ERROR_IDF << "invalid ANGLE value in FIELD 3\n";
            std::cerr << "    LINE: " << iline << "\n";
            return false;
        }

        // the line was successfully read; convert to mm if necessary
        if( unit == UNIT_THOU )
        {
            x *= IDF_MM_TO_THOU;
            y *= IDF_MM_TO_THOU;
        }

        if( npts++ == 0 )
        {
            // first point
            prePt.x = x;
            prePt.y = y;

            // ensure that the first point is not an arc specification
            if( ang < -MIN_ANG || ang > MIN_ANG )
            {
                ERROR_IDF << "invalid RECORD 3, first point has non-zero angle\n";
                std::cerr << "    LINE: " << iline << "\n";
                return false;
            }
        }
        else
        {
            // Nth point
            if( closed )
            {
                ERROR_IDF << "invalid RECORD 3; adding a segment to a closed outline\n";
                std::cerr << "    LINE: " << iline << "\n";
                return false;
            }

            curPt.x = x;
            curPt.y = y;

            if( ang > -MIN_ANG && ang < MIN_ANG )
            {
                sp = new IDF_SEGMENT( prePt, curPt );
            }
            else
            {
                sp = new IDF_SEGMENT( prePt, curPt, ang, false );
            }

            if( sp == NULL )
            {
                ERROR_IDF << "memory allocation failure\n";
                return false;
            }

            if( sp->IsCircle() )
            {
                // this is  a circle; the loop is closed
                if( op->size() != 0 )
                {
                    ERROR_IDF << "invalid RECORD 3; adding a circle to a non-empty outline\n";
                    std::cerr << "    LINE: " << iline << "\n";
                    delete sp;
                    return false;
                }

                closed = true;
            }
            else if( op->size() != 0 )
            {
                if( curPt.Matches( op->front()->startPoint, dLoc ) )
                    closed = true;
            }

            op->push( sp );
            prePt.x = x;
            prePt.y = y;
        }
    }   //  while( aBoardFile.good() )

    // NOTE:
    // 1. ideally we would ensure that there are no arcs with a radius of 0; this entails
    //    actively calculating the last point as the previous entry could have been an instruction

    return false;
}

bool BOARD_OUTLINE::writeComments( std::ofstream& aBoardFile )
{
    if( comments.empty() )
        return true;

    list< string >::const_iterator itS = comments.begin();
    list< string >::const_iterator itE = comments.end();

    while( itS != itE )
    {
        aBoardFile << "# " << *itS << "\n";
        ++itS;
    }

    return !aBoardFile.fail();
}

bool BOARD_OUTLINE::writeOwner( std::ofstream& aBoardFile )
{
    switch( owner )
    {
        case ECAD:
            aBoardFile << "ECAD\n";
            break;

        case MCAD:
            aBoardFile << "MCAD\n";
            break;

        default:
            aBoardFile << "UNOWNED\n";
            break;
    }

    return !aBoardFile.fail();
}

bool BOARD_OUTLINE::writeOutline( std::ofstream& aBoardFile, IDF_OUTLINE* aOutline, size_t aIndex )
{
    // TODO: check the stream integrity

    std::list<IDF_SEGMENT*>::iterator bo;
    std::list<IDF_SEGMENT*>::iterator eo;

    if( aOutline->size() == 1 )
    {
        if( !aOutline->front()->IsCircle() )
        {
            // this is a bad outline
            ERROR_IDF << "bad outline (single segment item, not circle)\n";
            return false;
        }

        if( single )
            aIndex = 0;

        // NOTE: a circle always has an angle of 360, never -360,
        // otherwise SolidWorks chokes on the file.
        if( unit == UNIT_MM )
        {
            aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
            << aOutline->front()->startPoint.x << " "
            << aOutline->front()->startPoint.y << " 0\n";

            aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
            << aOutline->front()->endPoint.x << " "
            << aOutline->front()->endPoint.y << " 360\n";
        }
        else
        {
            aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
            << (aOutline->front()->startPoint.x / IDF_MM_TO_THOU) << " "
            << (aOutline->front()->startPoint.y / IDF_MM_TO_THOU) << " 0\n";

            aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
            << (aOutline->front()->endPoint.x / IDF_MM_TO_THOU) << " "
            << (aOutline->front()->endPoint.y / IDF_MM_TO_THOU) << " 360\n";
        }

        return !aBoardFile.fail();
    }

    // ensure that the very last point is the same as the very first point
    aOutline->back()-> endPoint = aOutline->front()->startPoint;

    if( single )
    {
        // only indices 0 (CCW) and 1 (CW) are valid; set the index according to
        // the outline's winding
        if( aOutline->IsCCW() )
            aIndex = 0;
        else
            aIndex = 1;
    }

    // check if we must reverse things
    if( ( aOutline->IsCCW() && ( aIndex > 0 ) )
        || ( ( !aOutline->IsCCW() ) && ( aIndex == 0 ) ) )
    {
        eo  = aOutline->begin();
        bo  = aOutline->end();
        --bo;

        // for the first item we write out both points
        if( unit == UNIT_MM )
        {
            if( aOutline->front()->angle < MIN_ANG && aOutline->front()->angle > -MIN_ANG )
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << aOutline->front()->endPoint.x << " "
                << aOutline->front()->endPoint.y << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << aOutline->front()->startPoint.x << " "
                << aOutline->front()->startPoint.y << " 0\n";
            }
            else
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << aOutline->front()->endPoint.x << " "
                << aOutline->front()->endPoint.y << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << aOutline->front()->startPoint.x << " "
                << aOutline->front()->startPoint.y << " "
                << setprecision(5) << -aOutline->front()->angle << "\n";
            }
        }
        else
        {
            if( aOutline->front()->angle < MIN_ANG && aOutline->front()->angle > -MIN_ANG )
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->endPoint.x / IDF_MM_TO_THOU) << " "
                << (aOutline->front()->endPoint.y / IDF_MM_TO_THOU) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->startPoint.x / IDF_MM_TO_THOU) << " "
                << (aOutline->front()->startPoint.y / IDF_MM_TO_THOU) << " 0\n";
            }
            else
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->endPoint.x / IDF_MM_TO_THOU) << " "
                << (aOutline->front()->endPoint.y / IDF_MM_TO_THOU) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->startPoint.x / IDF_MM_TO_THOU) << " "
                << (aOutline->front()->startPoint.y / IDF_MM_TO_THOU) << " "
                << setprecision(5) << -aOutline->front()->angle << "\n";
            }
        }

        // for all other segments we only write out the start point
        while( bo != eo )
        {
            if( unit == UNIT_MM )
            {
                if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                    << (*bo)->startPoint.x << " "
                    << (*bo)->startPoint.y << " 0\n";
                }
                else
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                    << (*bo)->startPoint.x << " "
                    << (*bo)->startPoint.y << " "
                    << setprecision(5) << -(*bo)->angle << "\n";
                }
            }
            else
            {
                if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->startPoint.x / IDF_MM_TO_THOU) << " "
                    << ((*bo)->startPoint.y / IDF_MM_TO_THOU) << " 0\n";
                }
                else
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->startPoint.x / IDF_MM_TO_THOU) << " "
                    << ((*bo)->startPoint.y / IDF_MM_TO_THOU) << " "
                    << setprecision(5) << -(*bo)->angle << "\n";
                }
            }

            --bo;
        }
    }
    else
    {
        bo  = aOutline->begin();
        eo  = aOutline->end();

        // for the first item we write out both points
        if( unit == UNIT_MM )
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << (*bo)->startPoint.x << " "
                << (*bo)->startPoint.y << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << (*bo)->endPoint.x << " "
                << (*bo)->endPoint.y << " 0\n";
            }
            else
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << (*bo)->startPoint.x << " "
                << (*bo)->startPoint.y << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                << (*bo)->endPoint.x << " "
                << (*bo)->endPoint.y << " "
                << setprecision(5) << (*bo)->angle << "\n";
            }
        }
        else
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->startPoint.x / IDF_MM_TO_THOU) << " "
                << ((*bo)->startPoint.y / IDF_MM_TO_THOU) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->endPoint.x / IDF_MM_TO_THOU) << " "
                << ((*bo)->endPoint.y / IDF_MM_TO_THOU) << " 0\n";
            }
            else
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->startPoint.x / IDF_MM_TO_THOU) << " "
                << ((*bo)->startPoint.y / IDF_MM_TO_THOU) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->endPoint.x / IDF_MM_TO_THOU) << " "
                << ((*bo)->endPoint.y / IDF_MM_TO_THOU) << " "
                << setprecision(5) << (*bo)->angle << "\n";
            }
        }

        ++bo;

        // for all other segments we only write out the last point
        while( bo != eo )
        {
            if( unit == UNIT_MM )
            {
                if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                    << (*bo)->endPoint.x << " "
                    << (*bo)->endPoint.y << " 0\n";
                }
                else
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(5)
                    << (*bo)->endPoint.x << " "
                    << (*bo)->endPoint.y << " "
                    << setprecision(5) << (*bo)->angle << "\n";
                }
            }
            else
            {
                if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->endPoint.x / IDF_MM_TO_THOU) << " "
                    << ((*bo)->endPoint.y / IDF_MM_TO_THOU) << " 0\n";
                }
                else
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->endPoint.x / IDF_MM_TO_THOU) << " "
                    << ((*bo)->endPoint.y / IDF_MM_TO_THOU) << " "
                    << setprecision(5) << (*bo)->angle << "\n";
                }
            }

            ++bo;
        }
    }

    return !aBoardFile.fail();
}

bool BOARD_OUTLINE::writeOutlines( std::ofstream& aBoardFile )
{
    if( outlines.empty() )
        return true;

    int idx = 0;
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    while( itS != itE )
    {
        if( !writeOutline( aBoardFile, *itS, idx++ ) )
            return false;

        ++itS;
    }

    return true;
}

void BOARD_OUTLINE::SetUnit( IDF3::IDF_UNIT aUnit )
{
    unit = aUnit;
    return;
}

IDF3::IDF_UNIT BOARD_OUTLINE::GetUnit( void )
{
    return unit;
}

bool BOARD_OUTLINE::SetThickness( double aThickness )
{
    if( aThickness < 0.0 )
        return false;

    thickness = aThickness;
    return true;
}

double BOARD_OUTLINE::GetThickness( void )
{
    return thickness;
}

bool BOARD_OUTLINE::ReadData( std::ifstream& aBoardFile, const std::string& aHeader )
{
    //  BOARD_OUTLINE (PANEL_OUTLINE)
    //      .BOARD_OUTLINE  [OWNER]
    //      [thickness]
    //      [outlines]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "invalid invocation; blank header line\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "section names may not be quoted:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( !CompareToken( ".BOARD_OUTLINE", token ) )
    {
        ERROR_IDF << "not a board outline:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "no OWNER; setting to UNOWNED\n";
        owner = UNOWNED;
    }
    else
    {
        if( !ParseOwner( token, owner ) )
        {
            ERROR_IDF << "invalid OWNER (reverting to UNOWNED): " << token << "\n";
            owner = UNOWNED;
        }
    }

    // check RECORD 2
    std::string iline;
    bool comment = false;
    std::streampos pos;

    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad .BOARD_OUTLINE section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .BOARD_OUTLINE section\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad .BOARD_OUTLINE section (no thickness)\n";
        return false;
    }

    std::stringstream teststr;
    teststr << token;

    teststr >> thickness;
    if( teststr.fail() )
    {
        ERROR_IDF << "bad .BOARD_OUTLINE section (invalid RECORD 2)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( unit == UNIT_THOU )
        thickness *= IDF_MM_TO_THOU;

    // read RECORD 3 values
    // XXX - check the return value - we may have empty lines and what-not
    readOutlines( aBoardFile );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad .BOARD_OUTLINE section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .BOARD_OUTLINE section\n";
        return false;
    }

    if( !CompareToken( ".END_BOARD_OUTLINE", iline ) )
    {
        ERROR_IDF << "bad .BOARD_OUTLINE section (no .END_BOARD_OUTLINE)\n";
        return false;
    }

    return true;
}

bool BOARD_OUTLINE::WriteData( std::ofstream& aBoardFile )
{
    writeComments( aBoardFile );

    // note: a BOARD_OUTLINE section is required, even if it is empty
    aBoardFile << ".BOARD_OUTLINE ";

    writeOwner( aBoardFile );

    if( unit == UNIT_MM )
        aBoardFile << setiosflags(ios::fixed) << setprecision(5) << thickness << "\n";
    else
        aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_MM_TO_THOU) << "\n";

    if( !writeOutlines( aBoardFile ) )
        return false;

    aBoardFile << ".END_BOARD_OUTLINE\n\n";

    return !aBoardFile.fail();
}

void BOARD_OUTLINE::Clear( void )
{
    comments.clear();
    ClearOutlines();

    owner = UNOWNED;
    return;
}

void BOARD_OUTLINE::SetParent( IDF3_BOARD* aParent )
{
    parent = aParent;
}

IDF3_BOARD* BOARD_OUTLINE::GetParent( void )
{
    return parent;
}

bool BOARD_OUTLINE::AddOutline( IDF_OUTLINE* aOutline )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    while( itS != itE )
    {
        if( *itS == aOutline )
            return false;

        ++itS;
    }

    outlines.push_back( aOutline );
    return true;
}

bool BOARD_OUTLINE::DelOutline( IDF_OUTLINE* aOutline )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    if( outlines.empty() )
        return false;

    // if there are more than 1 outlines it makes no sense to delete
    // the first outline (board outline) since that would have the
    // undesirable effect of substituting a cutout outline as the board outline
    if( aOutline == outlines.front() )
    {
        if( outlines.size() > 1 )
            return false;

        outlines.clear();
        return true;
    }

    while( itS != itE )
    {
        if( *itS == aOutline )
        {
            outlines.erase( itS );
            return true;
        }

        ++itS;
    }

    return false;
}

bool BOARD_OUTLINE::DelOutline( size_t aIndex )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    if( outlines.empty() )
        return false;

    if( aIndex >= outlines.size() )
        return false;

    if( aIndex == 0 )
    {
        // if there are more than 1 outlines it makes no sense to delete
        // the first outline (board outline) since that would have the
        // undesirable effect of substituting a cutout outline as the board outline
        if( outlines.size() > 1 )
            return false;

        delete *itS;
        outlines.clear();

        return true;
    }

    for( ; aIndex > 0; --aIndex )
        ++itS;

    delete *itS;
    outlines.erase( itS );

    return true;
}

const std::list< IDF_OUTLINE* >*const BOARD_OUTLINE::GetOutlines( void )
{
    return &outlines;
}

size_t BOARD_OUTLINE::OutlinesSize( void )
{
    return outlines.size();
}

IDF_OUTLINE* BOARD_OUTLINE::GetOutline( size_t aIndex )
{
    if( aIndex >= outlines.size() )
        return NULL;

    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();

    for( ; aIndex > 0; --aIndex )
        ++itS;

    return *itS;
}

IDF3::KEY_OWNER BOARD_OUTLINE::GetOwner( void )
{
    return owner;
}

bool BOARD_OUTLINE::SetOwner( IDF3::KEY_OWNER aOwner )
{
    // if this is a COMPONENT OUTLINE there can be no owner
    if( outlineType == IDF3::OTLN_COMPONENT )
        return true;

    // if no one owns the outline, any system may
    // set the owner
    if( owner == UNOWNED )
    {
        owner = aOwner;
        return true;
    }

    // if the outline is owned, only the owning
    // CAD system can make alterations
    if( parent == NULL )
        return false;

    if( owner == MCAD && parent->GetCadType() == CAD_MECH )
    {
        owner = aOwner;
        return true;
    }

    if( owner == ECAD && parent->GetCadType() == CAD_ELEC )
    {
        owner = aOwner;
        return true;
    }

    return false;
}

bool BOARD_OUTLINE::IsSingle( void )
{
    return single;
}

void BOARD_OUTLINE::ClearOutlines( void )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    while( itS != itE )
    {
        delete *itS;
        ++itS;
    }

    outlines.clear();
    return;
}

void BOARD_OUTLINE::AddComment( const std::string& aComment )
{
    if( aComment.empty() )
        return;

    comments.push_back( aComment );
    return;
}

size_t BOARD_OUTLINE::CommentsSize( void )
{
    return comments.size();
}

std::list< std::string >* BOARD_OUTLINE::GetComments( void )
{
    return &comments;
}

const std::string* BOARD_OUTLINE::GetComment( size_t aIndex )
{
    if( aIndex >= comments.size() )
        return NULL;

    std::list< std::string >::iterator itS = comments.begin();

    for( ; aIndex > 0; --aIndex )
        ++itS;

    return &(*itS);
}

bool  BOARD_OUTLINE::DeleteComment( size_t aIndex )
{
    if( aIndex >= comments.size() )
        return false;

    std::list< std::string >::iterator itS = comments.begin();

    for( ; aIndex > 0; --aIndex )
        ++itS;

    comments.erase( itS );
    return true;
}

void  BOARD_OUTLINE::ClearComments( void )
{
    comments.clear();
    return;
}


/*
 * CLASS: OTHER_OUTLINE
 */
OTHER_OUTLINE::OTHER_OUTLINE()
{
    outlineType = OTLN_OTHER;
    side = LYR_INVALID;
    single = true;

    return;
}

void OTHER_OUTLINE::SetOutlineIdentifier( const std::string aUniqueID )
{
    uniqueID = aUniqueID;
    return;
}

const std::string& OTHER_OUTLINE::GetOutlineIdentifier( void )
{
    return uniqueID;
}

bool OTHER_OUTLINE::SetSide( IDF3::IDF_LAYER aSide )
{
    switch( aSide )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
            side = aSide;
            break;

        default:
            ERROR_IDF << "invalid side (" << aSide << "); must be one of TOP/BOTTOM\n";
            side = LYR_INVALID;
            return false;
            break;
    }

    return true;
}

IDF3::IDF_LAYER OTHER_OUTLINE::GetSide( void )
{
    return side;
}

bool OTHER_OUTLINE::ReadData( std::ifstream& aBoardFile, const std::string& aHeader )
{
    // OTHER_OUTLINE/VIA_KEEPOUT
    //     .OTHER_OUTLINE  [OWNER]
    //     [outline identifier] [thickness] [board side: Top/Bot] {not present in VA\IA KEEPOUT}
    //     [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "invalid invocation; blank header line\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "section names may not be quoted:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( outlineType == OTLN_OTHER )
    {
        if( !CompareToken( ".OTHER_OUTLINE", token ) )
        {
            ERROR_IDF << "not an OTHER outline:\n";
            std::cerr << "\tLINE: " << aHeader << "\n";
            return false;
        }
    }
    else
    {
        if( !CompareToken( ".VIA_KEEPOUT", token ) )
        {
            ERROR_IDF << "not a VIA_KEEPOUT outline:\n";
            std::cerr << "\tLINE: " << aHeader << "\n";
            return false;
        }
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "no OWNER; setting to UNOWNED\n";
        owner = UNOWNED;
    }
    else
    {
        if( !ParseOwner( token, owner ) )
        {
            ERROR_IDF << "invalid OWNER (reverting to UNOWNED): " << token << "\n";
            owner = UNOWNED;
        }
    }

    std::string iline;
    bool comment = false;
    std::streampos pos;

    if( outlineType == OTLN_OTHER )
    {
        // check RECORD 2
        // [outline identifier] [thickness] [board side: Top/Bot]
        while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

        if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (premature end)\n";
            return false;
        }

        idx = 0;
        if( comment )
        {
            ERROR_IDF << "comment within .OTHER_OUTLINE section\n";
            return false;
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (no outline identifier)\n";
            return false;
        }

        uniqueID = token;

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (no thickness)\n";
            return false;
        }

        std::stringstream teststr;
        teststr << token;

        teststr >> thickness;
        if( teststr.fail() )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (invalid RECORD 2 reading thickness)\n";
            std::cerr << "\tLINE: " << iline << "\n";
            return false;
        }

        if( unit == UNIT_THOU )
            thickness *= IDF_MM_TO_THOU;

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (no board side)\n";
            return false;
        }

        if( !ParseIDFLayer( token, side ) || ( side != LYR_TOP && side != LYR_BOTTOM ) )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (invalid side, must be TOP/BOTTOM only)\n";
            std::cerr << "\tLINE: " << iline << "\n";
            return false;
        }
    }

    // read RECORD 3 values
    readOutlines( aBoardFile );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad .OTHER_OUTLINE/.VIA_KEEPOUT section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .OTHER_OUTLINE/.VIA_KEEPOUT section\n";
        return false;
    }

    if( outlineType == OTLN_OTHER )
    {
        if( !CompareToken( ".END_OTHER_OUTLINE", iline ) )
        {
            ERROR_IDF << "bad .OTHER_OUTLINE section (no .END_OTHER_OUTLINE)\n";
            return false;
        }
    }
    else
    {
        if( !CompareToken( ".END_VIA_KEEPOUT", iline ) )
        {
            ERROR_IDF << "bad .VIA_KEEPOUT section (no .END_VIA_KEEPOUT)\n";
            return false;
        }
    }

    return true;
}

bool OTHER_OUTLINE::WriteData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return true;

    writeComments( aBoardFile );

    // write RECORD 1
    if( outlineType == OTLN_OTHER )
        aBoardFile << ".OTHER_OUTLINE ";
    else
        aBoardFile << ".VIA_KEEPOUT ";

    writeOwner( aBoardFile );

    // write RECORD 2
    if( outlineType == OTLN_OTHER )
    {
        aBoardFile << "\"" << uniqueID << "\" ";

        if( unit == UNIT_MM )
            aBoardFile << setiosflags(ios::fixed) << setprecision(5) << thickness << " ";
        else
            aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_MM_TO_THOU) << " ";

        switch( side )
        {
            case LYR_TOP:
            case LYR_BOTTOM:
                WriteLayersText( aBoardFile, side );
                break;

            default:
                ERROR_IDF << "Invalid OTHER_OUTLINE side (neither top nor bottom): " << side << "\n";
                return false;
                break;
        }
    }

    // write RECORD 3
    if( !writeOutlines( aBoardFile ) )
        return false;

    // write RECORD 4
    if( outlineType == OTLN_OTHER )
        aBoardFile << ".END_OTHER_OUTLINE\n\n";
    else
        aBoardFile << ".END_VIA_KEEPOUT\n\n";

    return !aBoardFile.fail();
}

void OTHER_OUTLINE::Clear( void )
{
    side = LYR_INVALID;
    uniqueID.clear();

    BOARD_OUTLINE::Clear();

    return;
}


/*
 * CLASS: ROUTE_OUTLINE
 */
ROUTE_OUTLINE::ROUTE_OUTLINE()
{
    outlineType = OTLN_ROUTE;
    single = true;
    layers = LYR_INVALID;
}

void ROUTE_OUTLINE::SetLayers( IDF3::IDF_LAYER aLayer )
{
    layers = aLayer;
}

IDF3::IDF_LAYER ROUTE_OUTLINE::GetLayers( void )
{
    return layers;
}

bool ROUTE_OUTLINE::ReadData( std::ifstream& aBoardFile, const std::string& aHeader )
{
    //  ROUTE_OUTLINE (or ROUTE_KEEPOUT)
    //      .ROUTE_OUTLINE [OWNER]
    //      [layers]
    //      [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "invalid invocation; blank header line\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "section names may not be quoted:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( outlineType == OTLN_ROUTE )
    {
        if( !CompareToken( ".ROUTE_OUTLINE", token ) )
        {
            ERROR_IDF << "not a ROUTE outline:\n";
            std::cerr << "\tLINE: " << aHeader << "\n";
            return false;
        }
    }
    else
    {
        if( !CompareToken( ".ROUTE_KEEPOUT", token ) )
        {
            ERROR_IDF << "not a ROUTE KEEPOUT outline:\n";
            std::cerr << "\tLINE: " << aHeader << "\n";
            return false;
        }
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "no OWNER; setting to UNOWNED\n";
        owner = UNOWNED;
    }
    else
    {
        if( !ParseOwner( token, owner ) )
        {
            ERROR_IDF << "invalid OWNER (reverting to UNOWNED): " << token << "\n";
            owner = UNOWNED;
        }
    }

    // check RECORD 2
    // [layers: TOP, BOTTOM, BOTH, INNER, ALL]
    std::string iline;
    bool comment = false;
    std::streampos pos;

    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( !aBoardFile.good() )
    {
        ERROR_IDF << "bad .ROUTE_OUTLINE/KEEPOUT section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .ROUTE_OUTLINE/KEEPOUT section\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad .ROUTE_OUTLINE/KEEPOUT section (no layers specification)\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "bad .ROUTE_OUTLINE/KEEPOUT section (layers may not be quoted)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( !ParseIDFLayer( token, layers ) )
    {
        ERROR_IDF << "bad .ROUTE_OUTLINE/KEEPOUT section (invalid layer)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    // read RECORD 3 values
    readOutlines( aBoardFile );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad .ROUTE_OUTLINE/KEEPOUT section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .ROUTE_OUTLINE/KEEPOUT section\n";
        return false;
    }

    if( outlineType == OTLN_ROUTE )
    {
        if( !CompareToken( ".END_ROUTE_OUTLINE", iline ) )
        {
            ERROR_IDF << "bad .ROUTE_OUTLINE section (no .END_ROUTE_OUTLINE)\n";
            return false;
        }
    }
    else
    {
        if( !CompareToken( ".END_ROUTE_KEEPOUT", iline ) )
        {
            ERROR_IDF << "bad .ROUTE_KEEPOUT section (no .END_ROUTE_KEEPOUT)\n";
            return false;
        }
    }

    return true;
}


bool ROUTE_OUTLINE::WriteData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return true;

    if( layers == LYR_INVALID )
    {
        ERROR_IDF << "layer not specified\n";
        return false;
    }

    writeComments( aBoardFile );

    // write RECORD 1
    if( outlineType == OTLN_ROUTE )
        aBoardFile << ".ROUTE_OUTLINE ";
    else
        aBoardFile << ".ROUTE_KEEPOUT ";

    writeOwner( aBoardFile );

    // write RECORD 2
    WriteLayersText( aBoardFile, layers );
    aBoardFile << "\n";

    // write RECORD 3
    if( !writeOutlines( aBoardFile ) )
        return false;

    // write RECORD 4
    if( outlineType == OTLN_ROUTE )
        aBoardFile << ".END_ROUTE_OUTLINE\n\n";
    else
        aBoardFile << ".END_ROUTE_KEEPOUT\n\n";

    return !aBoardFile.fail();
}


void ROUTE_OUTLINE::Clear( void )
{
    BOARD_OUTLINE::Clear();
    layers = LYR_INVALID;
    return;
}


/*
 * CLASS: PLACE_OUTLINE
 */
PLACE_OUTLINE::PLACE_OUTLINE()
{
    outlineType = OTLN_PLACE;
    single = true;
    thickness = 0.0;
    side = LYR_INVALID;
}

void PLACE_OUTLINE::SetSide( IDF3::IDF_LAYER aSide )
{
    switch( aSide )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
        case LYR_BOTH:
            side = aSide;
            break;

        default:
            // XXX - throw
            ERROR_IDF << "invalid layer (" << aSide << "): must be one of TOP/BOTTOM/BOTH\n";
            side = LYR_INVALID;
            return;
            break;
    }

    return;
}

IDF3::IDF_LAYER PLACE_OUTLINE::GetSide( void )
{
    return side;
}

void PLACE_OUTLINE::SetMaxHeight( double aHeight )
{
    if( aHeight < 0.0 )
    {
        ERROR_IDF << "invalid height (must be >= 0.0); default to 0\n";
        thickness = 0.0;
        return;
    }

    thickness = aHeight;
    return;
}

double PLACE_OUTLINE::GetMaxHeight( void )
{
    return thickness;
}

bool PLACE_OUTLINE::ReadData( std::ifstream& aBoardFile, const std::string& aHeader )
{
    //  PLACE_OUTLINE/KEEPOUT
    //      .PLACE_OUTLINE [OWNER]
    //      [board side: Top/Bot/Both] [height]
    //      [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "invalid invocation; blank header line\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "section names may not be quoted:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( outlineType == OTLN_PLACE )
    {
        if( !CompareToken( ".PLACE_OUTLINE", token ) )
        {
            ERROR_IDF << "not a PLACE outline:\n";
            std::cerr << "\tLINE: " << aHeader << "\n";
            return false;
        }
    }
    else
    {
        if( !CompareToken( ".PLACE_KEEPOUT", token ) )
        {
            ERROR_IDF << "not a PLACE_KEEPOUT outline:\n";
            std::cerr << "\tLINE: " << aHeader << "\n";
            return false;
        }
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "no OWNER; setting to UNOWNED\n";
        owner = UNOWNED;
    }
    else
    {
        if( !ParseOwner( token, owner ) )
        {
            ERROR_IDF << "invalid OWNER (reverting to UNOWNED): " << token << "\n";
            owner = UNOWNED;
        }
    }

    // check RECORD 2
    // [board side: Top/Bot/Both] [height]
    std::string iline;
    bool comment = false;
    std::streampos pos;

    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( !aBoardFile.good() )
    {
        ERROR_IDF << "bad .PLACE_OUTLINE/KEEPOUT section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .PLACE_OUTLINE/KEEPOUT section\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad .PLACE_OUTLINE/KEEPOUT section (no board side information)\n";
        return false;
    }

    if( !ParseIDFLayer( token, side ) ||
        ( side != LYR_TOP && side != LYR_BOTTOM && side != LYR_BOTH ) )
    {
        ERROR_IDF << "bad .PLACE_OUTLINE/KEEPOUT section (invalid side, must be one of TOP/BOTTOM/BOTH)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad .PLACE_OUTLINE/KEEPOUT section (no height)\n";
        return false;
    }

    std::stringstream teststr;
    teststr << token;

    teststr >> thickness;
    if( teststr.fail() )
    {
        ERROR_IDF << "bad .PLACE_OUTLINE/KEEPOUT section (invalid RECORD 2 reading height)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( unit == UNIT_THOU )
        thickness *= IDF_MM_TO_THOU;

    // read RECORD 3 values
    readOutlines( aBoardFile );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad .PLACE_OUTLINE/KEEPOUT section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .PLACE_OUTLINE/KEEPOUT section\n";
        return false;
    }

    if( outlineType == OTLN_PLACE )
    {
        if( !GetIDFString( iline, token, quoted, idx )
            || !CompareToken( ".END_PLACE_OUTLINE", token ) )
        {
            ERROR_IDF << "bad .PLACE_OUTLINE section (no .END_PLACE_OUTLINE)\n";
            return false;
        }
    }
    else
    {
        if( !GetIDFString( iline, token, quoted, idx )
            || !CompareToken( ".END_PLACE_KEEPOUT", token ) )
        {
            ERROR_IDF << "bad .PLACE_KEEPOUT section (no .END_PLACE_KEEPOUT)\n";
            return false;
        }
    }

    return true;
}

bool PLACE_OUTLINE::WriteData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return true;

    writeComments( aBoardFile );

    // write RECORD 1
    if( outlineType == OTLN_PLACE )
        aBoardFile << ".PLACE_OUTLINE ";
    else
        aBoardFile << ".PLACE_KEEPOUT ";

    writeOwner( aBoardFile );

    // write RECORD 2
    switch( side )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
        case LYR_BOTH:
            WriteLayersText( aBoardFile, side );
            break;

        default:
            ERROR_IDF << "Invalid PLACE_OUTLINE/KEEPOUT side (" << side << "); must be one of TOP/BOTTOM/BOTH\n";
            return false;
            break;
    }

    aBoardFile << " ";

    if( unit == UNIT_MM )
        aBoardFile << setiosflags(ios::fixed) << setprecision(5) << thickness << "\n";
    else
        aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_MM_TO_THOU) << "\n";

    // write RECORD 3
    if( !writeOutlines( aBoardFile ) )
        return false;

    // write RECORD 4
    if( outlineType == OTLN_PLACE )
        aBoardFile << ".END_PLACE_OUTLINE\n\n";
    else
        aBoardFile << ".END_PLACE_KEEPOUT\n\n";

    return !aBoardFile.fail();
}

void PLACE_OUTLINE::Clear( void )
{
    BOARD_OUTLINE::Clear();
    thickness = 0.0;
    side = LYR_INVALID;
    return;
}


/*
 * CLASS: ROUTE_KEEPOUT
 */
ROUTE_KO_OUTLINE::ROUTE_KO_OUTLINE()
{
    outlineType = OTLN_ROUTE_KEEPOUT;
    return;
}


/*
 * CLASS: PLACE_KEEPOUT
 */
PLACE_KO_OUTLINE::PLACE_KO_OUTLINE()
{
    outlineType = OTLN_PLACE_KEEPOUT;
    return;
}


/*
 * CLASS: VIA_KEEPOUT
 */
VIA_KO_OUTLINE::VIA_KO_OUTLINE()
{
    outlineType = OTLN_VIA_KEEPOUT;
}


/*
 * CLASS: PLACEMENT GROUP (PLACE_REGION)
 */
GROUP_OUTLINE::GROUP_OUTLINE()
{
    outlineType = OTLN_GROUP_PLACE;
    thickness = 0.0;
    side = LYR_INVALID;
    single = true;
    return;
}

void GROUP_OUTLINE::SetSide( IDF3::IDF_LAYER aSide )
{
    switch( aSide )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
        case LYR_BOTH:
            side = aSide;
            break;

        default:
            // XXX throw
            ERROR_IDF << "invalid side (" << aSide << "); must be one of TOP/BOTTOM/BOTH\n";
            return;
            break;
    }

    return;
}

IDF3::IDF_LAYER GROUP_OUTLINE::GetSide( void )
{
    return side;
}

void GROUP_OUTLINE::SetGroupName( std::string aGroupName )
{
    groupName = aGroupName;
    return;
}

const std::string& GROUP_OUTLINE::GetGroupName( void )
{
    return groupName;
}

bool GROUP_OUTLINE::ReadData( std::ifstream& aBoardFile, const std::string& aHeader )
{
    //  Placement Group
    //      .PLACE_REGION [OWNER]
    //      [side: Top/Bot/Both ] [component group name]
    //      [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "invalid invocation; blank header line\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "section names may not be quoted:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( !CompareToken( ".PLACE_REGION", token ) )
    {
        ERROR_IDF << "not a PLACE_REGION outline:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "no OWNER; setting to UNOWNED\n";
        owner = UNOWNED;
    }
    else
    {
        if( !ParseOwner( token, owner ) )
        {
            ERROR_IDF << "invalid OWNER (reverting to UNOWNED): " << token << "\n";
            owner = UNOWNED;
        }
    }

    std::string iline;
    bool comment = false;
    std::streampos pos;

    // check RECORD 2
    // [side: Top/Bot/Both ] [component group name]
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( !aBoardFile.good() )
    {
        ERROR_IDF << "bad .PLACE_REGION section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .PLACE_REGION section\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad .PLACE_REGION section (no board side)\n";
        return false;
    }

    if( !ParseIDFLayer( token, side ) ||
        ( side != LYR_TOP && side != LYR_BOTTOM && side != LYR_BOTH ) )
    {
        ERROR_IDF << "bad .PLACE_REGION section (invalid side, must be TOP/BOTTOM/BOTH)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad .PLACE_REGION section (no outline identifier)\n";
        return false;
    }

    groupName = token;

    // read RECORD 3 values
    readOutlines( aBoardFile );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad .PLACE_REGION section (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within .PLACE_REGION section\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx )
        || !CompareToken( ".END_PLACE_REGION", token ) )
    {
        ERROR_IDF << "bad .PLACE_REGION section (no .END_PLACE_REGION)\n";
        return false;
    }

    return true;
}

bool GROUP_OUTLINE::WriteData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return true;

    writeComments( aBoardFile );

    // write RECORD 1
    aBoardFile << ".PLACE_REGION ";

    writeOwner( aBoardFile );

    // write RECORD 2
    switch( side )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
        case LYR_BOTH:
            WriteLayersText( aBoardFile, side );
            break;

        default:
            ERROR_IDF << "Invalid PLACE_REGION side (must be TOP/BOTTOM/BOTH): " << side << "\n";
            return false;
            break;
    }

    aBoardFile << " \"" << groupName << "\"\n";

    // write RECORD 3
    if( !writeOutlines( aBoardFile ) )
        return false;

    // write RECORD 4
    aBoardFile << ".END_PLACE_REGION\n\n";

    return !aBoardFile.fail();
}

void GROUP_OUTLINE::Clear( void )
{
    BOARD_OUTLINE::Clear();
    thickness = 0.0;
    side = LYR_INVALID;
    groupName.clear();
    return;
}

/*
 * CLASS: COMPONENT OUTLINE
 */
IDF3_COMP_OUTLINE::IDF3_COMP_OUTLINE()
{
    single = true;
    outlineType = OTLN_COMPONENT;
    compType = COMP_INVALID;
    refNum = 0;
    return;
}

bool IDF3_COMP_OUTLINE::readProperties( std::ifstream& aLibFile )
{
    bool quoted = false;
    bool comment = false;
    std::string iline;
    std::string token;
    std::streampos pos;
    std::string pname;      // property name
    std::string pval;       // property value
    int idx = 0;

    while( aLibFile.good() )
    {
        if( !FetchIDFLine( aLibFile, iline, comment, pos ) )
            continue;

        idx = 0;
        if( comment )
        {
            ERROR_IDF << "comment within component outline section\n";
            return false;
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF << "bad component outline section (no PROP)\n";
            return false;
        }

        if( quoted )
        {
            ERROR_IDF << "bad component outline section (PROP or .END may not be quoted)\n";
            return false;
        }

        if( token.size() >= 5 && CompareToken( ".END_", token.substr( 0, 5 ) ) )
        {
            aLibFile.seekg( pos );
            return true;
        }

        if( !CompareToken( "PROP", token ) )
        {
            ERROR_IDF << "invalid electrical outline; expecting PROP or .END_ELECTRICAL\n";
            std::cerr << "\tLINE: " << iline << "\n";
            return false;
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF << "bad component outline section (no prop name)\n";
            return false;
        }

        pname = token;

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF << "bad component outline section (no prop value)\n";
            return false;
        }

        pval = token;

        if( props.insert( pair< string, string >(pname, pval) ).second == false )
        {
            ERROR_IDF << "bad component outline: duplicate property name '" << pname << "'\n";
            return false;
        }
    }

    return !aLibFile.fail();
}

bool IDF3_COMP_OUTLINE::writeProperties( std::ofstream& aLibFile )
{
    if( props.empty() )
        return true;
    std::map< std::string, std::string >::const_iterator itS = props.begin();
    std::map< std::string, std::string >::const_iterator itE = props.end();

    while( itS != itE )
    {
        aLibFile << "PROP " << "\"" << itS->first << "\" \""
        << itS->second << "\"\n";
        ++itS;
    }

    return !aLibFile.fail();
}

bool IDF3_COMP_OUTLINE::ReadData( std::ifstream& aLibFile, const std::string& aHeader )
{
    //  .ELECTRICAL/.MECHANICAL
    //  [GEOM] [PART] [UNIT] [HEIGHT]
    //  [outline]
    //  [PROP] [prop name] [prop value]
    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ERROR_IDF << "invalid invocation; blank header line\n";
        return false;
    }

    if( quoted )
    {
        ERROR_IDF << "section names may not be quoted:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    if( CompareToken( ".ELECTRICAL", token ) )
    {
        compType = COMP_ELEC;
    }
    else if( CompareToken( ".MECHANICAL", token ) )
    {
        compType = COMP_MECH;
    }
    else
    {
        ERROR_IDF << "not a component outline:\n";
        std::cerr << "\tLINE: " << aHeader << "\n";
        return false;
    }

    // check RECORD 2
    // [GEOM] [PART] [UNIT] [HEIGHT]
    std::string iline;
    bool comment = false;
    std::streampos pos;

    while( aLibFile.good() && !FetchIDFLine( aLibFile, iline, comment, pos ) );

    if( !aLibFile.good() )
    {
        ERROR_IDF << "bad component outline data (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within a component outline section\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad component outline (no GEOMETRY NAME)\n";
        return false;
    }

    geometry = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad component outline (no PART NAME)\n";
        return false;
    }

    part = token;

    if( part.empty() && geometry.empty() )
    {
        ERROR_IDF << "bad component outline (both GEOMETRY and PART names are empty)\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad component outline (no unit type)\n";
        return false;
    }

    if( CompareToken( "MM", token ) )
    {
        unit = UNIT_MM;
    }
    else if( CompareToken( "THOU", token ) )
    {
        unit = UNIT_THOU;
    }
    else
    {
        ERROR_IDF << "bad component outline (invalid unit type)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF << "bad component outline (no height)\n";
        return false;
    }

    std::istringstream teststr;
    teststr.str( token );

    teststr >> thickness;
    if( teststr.fail() )
    {
        ERROR_IDF << "bad component outline (invalid height)\n";
        std::cerr << "\tLINE: " << iline << "\n";
        return false;
    }

    if( unit == UNIT_THOU )
        thickness *= IDF_MM_TO_THOU;

    // read RECORD 3 values
    readOutlines( aLibFile );

    if( compType == COMP_ELEC )
    {
        if( !readProperties( aLibFile ) )
            return false;
    }

    // check RECORD 4
    while( aLibFile.good() && !FetchIDFLine( aLibFile, iline, comment, pos ) );

    if( ( !aLibFile.good() && aLibFile.eof() ) || iline.empty() )
    {
        ERROR_IDF << "bad component outline data (premature end)\n";
        return false;
    }

    idx = 0;
    if( comment )
    {
        ERROR_IDF << "comment within component outline section\n";
        return false;
    }

    if( compType == COMP_ELEC )
    {
        if( !CompareToken( ".END_ELECTRICAL", iline ) )
        {
            ERROR_IDF << "bad component outline (no .END_ELECTRICAL)\n";
            return false;
        }
    }
    else
    {
        if( !CompareToken( ".END_MECHANICAL", iline ) )
        {
            ERROR_IDF << "corrupt .MECHANICAL outline\n";
            return false;
        }
    }

    return true;
}

bool IDF3_COMP_OUTLINE::WriteData( std::ofstream& aLibFile )
{
    if( compType != COMP_ELEC && compType != COMP_MECH )
    {
        ERROR_IDF << "component type not set or invalid\n";
        return false;
    }

    if( refNum == 0 )
        return true;    // nothing to do

    writeComments( aLibFile );

    // note: the outline section is required, even if it is empty
    if( compType == COMP_ELEC )
        aLibFile << ".ELECTRICAL\n";
    else
        aLibFile << ".MECHANICAL\n";

    // RECORD 2
    // [GEOM] [PART] [UNIT] [HEIGHT]
    aLibFile << "\"" << geometry << "\" \"" << part << "\" ";

    if( unit == UNIT_MM )
        aLibFile << "MM " << setiosflags(ios::fixed) << setprecision(5) << thickness << "\n";
    else
        aLibFile << "THOU " << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_MM_TO_THOU) << "\n";

    if( !writeOutlines( aLibFile ) )
        return false;

    if( compType == COMP_ELEC )
    {
        writeProperties( aLibFile );
        aLibFile << ".END_ELECTRICAL\n\n";
    }
    else
    {
        aLibFile << ".END_MECHANICAL\n\n";
    }

    return !aLibFile.fail();
}

void IDF3_COMP_OUTLINE::Clear( void )
{
    BOARD_OUTLINE::Clear();
    uid.clear();
    geometry.clear();
    part.clear();
    compType = COMP_INVALID;
    refNum = 0;
    props.clear();
    return;
}

void IDF3_COMP_OUTLINE::SetComponentClass( IDF3::COMP_TYPE aCompClass )
{
    switch( aCompClass )
    {
        case COMP_ELEC:
        case COMP_MECH:
            compType = aCompClass;
            break;

        default:
            // XXX - throw
            ERROR_IDF << "invalid component class (must be ELECTRICAL or MECHANICAL)\n";
            return;
            break;
    }

    return;
}

IDF3::COMP_TYPE IDF3_COMP_OUTLINE::GetComponentClass( void )
{
    return compType;
}


void IDF3_COMP_OUTLINE::SetGeomName( const std::string& aGeomName )
{
    geometry = aGeomName;
    uid.clear();
    return;
}

const std::string& IDF3_COMP_OUTLINE::GetGeomName( void )
{
    return geometry;
}

void IDF3_COMP_OUTLINE::SetPartName( const std::string& aPartName )
{
    part = aPartName;
    uid.clear();
    return;
}

const std::string& IDF3_COMP_OUTLINE::GetPartName( void )
{
    return part;
}

const std::string& IDF3_COMP_OUTLINE::GetUID( void )
{
    if( !uid.empty() )
        return uid;

    if( geometry.empty() && part.empty() )
        return uid;

    uid = geometry + "_" + part;

    return uid;
}


int IDF3_COMP_OUTLINE::IncrementRef( void )
{
    return ++refNum;
}

int IDF3_COMP_OUTLINE::DecrementRef( void )
{
    if( refNum == 0 )
    {
        ERROR_IDF << "BUG: decrementing refNum beyond 0\n";
        return 0;
    }

    --refNum;
    return refNum;
}

bool IDF3_COMP_OUTLINE::CreateDefaultOutline( const std::string &aGeom, const std::string &aPart )
{
    Clear();

    if( aGeom.empty() && aPart.empty() )
    {
        geometry  = "NOGEOM";
        part      = "NOPART";
        uid       = "NOGEOM_NOPART";
    }
    else
    {
        geometry  = aGeom;
        part      = aPart;
        uid       = aGeom + "_" + aPart;
    }

    compType  = COMP_ELEC;
    thickness = 5.0;
    unit      = UNIT_MM;

    // Create a star shape 5mm high with points on 5 and 3 mm circles
    double a, da;
    da = M_PI / 5.0;
    a = da / 2.0;

    IDF_POINT p1, p2;
    IDF_OUTLINE* ol = new IDF_OUTLINE;
    IDF_SEGMENT* sp;

    p1.x = 1.5 * cos( a );
    p1.y = 1.5 * sin( a );

    if( ol == NULL )
        return false;

    for( int i = 0; i < 10; ++i )
    {
        if( i & 1 )
        {
            p2.x = 2.5 * cos( a );
            p2.y = 2.5 * sin( a );
        }
        else
        {
            p2.x = 1.5 * cos( a );
            p2.y = 1.5 * sin( a );
        }

        sp = new IDF_SEGMENT( p1, p2 );

        if( sp == NULL )
        {
            Clear();
            return false;
        }

        ol->push( sp );
        a += da;
        p1 = p2;
    }

    a = da / 2.0;
    p2.x = 1.5 * cos( a );
    p2.y = 1.5 * sin( a );

    sp = new IDF_SEGMENT( p1, p2 );

    if( sp == NULL )
    {
        Clear();
        return false;
    }

    ol->push( sp );
    outlines.push_back( ol );

    return true;
}
