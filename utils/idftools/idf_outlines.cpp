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


static std::string GetOutlineTypeString( IDF3::OUTLINE_TYPE aOutlineType )
{
    switch( aOutlineType )
    {
        case OTLN_BOARD:
            return ".BOARD_OUTLINE";

        case OTLN_OTHER:
            return ".OTHER_OUTLINE";

        case OTLN_PLACE:
            return ".PLACEMENT_OUTLINE";

        case OTLN_ROUTE:
            return ".ROUTE_OUTLINE";

        case OTLN_PLACE_KEEPOUT:
            return ".PLACE_KEEPOUT";

        case OTLN_ROUTE_KEEPOUT:
            return ".ROUTE_KEEPOUT";

        case OTLN_VIA_KEEPOUT:
            return ".VIA_KEEPOUT";

        case OTLN_GROUP_PLACE:
            return ".PLACE_REGION";

        case OTLN_COMPONENT:
            return "COMPONENT OUTLINE";

        default:
            break;
    }

    std::ostringstream ostr;
    ostr << "[INVALID OUTLINE TYPE VALUE]:" << aOutlineType;

    return ostr.str();
}

#ifndef DISABLE_IDF_OWNERSHIP
static bool CheckOwnership( int aSourceLine, const char* aSourceFunc,
                            IDF3_BOARD* aParent, IDF3::KEY_OWNER aOwnerCAD,
                            IDF3::OUTLINE_TYPE aOutlineType, std::string& aErrorString )
{
    if( aParent == NULL )
    {
        ostringstream ostr;
        ostr << "* " << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc << "():\n";
        ostr << "* BUG: outline's parent not set; cannot enforce ownership rules\n";
        ostr << "* outline type: " << GetOutlineTypeString( aOutlineType );
        aErrorString = ostr.str();

        return false;
    }

    // note: component outlines have no owner so we don't care about
    // who modifies them
    if( aOwnerCAD == UNOWNED || aOutlineType == IDF3::OTLN_COMPONENT )
        return true;

    IDF3::CAD_TYPE parentCAD = aParent->GetCadType();

    if( aOwnerCAD == MCAD && parentCAD == CAD_MECH )
        return true;

    if( aOwnerCAD == ECAD && parentCAD == CAD_ELEC )
        return true;

    do
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc << "():\n";
        ostr << "* ownership violation; CAD type is ";

        if( parentCAD == CAD_MECH )
            ostr << "MCAD ";
        else
            ostr << "ECAD ";

        ostr << "while outline owner is " << GetOwnerString( aOwnerCAD ) << "\n";
        ostr << "* outline type: " << GetOutlineTypeString( aOutlineType );
        aErrorString = ostr.str();

    } while( 0 );

    return false;
}
#endif


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
    clear();
    return;
}

IDF3::OUTLINE_TYPE BOARD_OUTLINE::GetOutlineType( void )
{
    return outlineType;
}

void BOARD_OUTLINE::readOutlines( std::ifstream& aBoardFile, IDF3::IDF_VERSION aIdfVersion )
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
    clearOutlines();

    while( aBoardFile.good() )
    {
        if( !FetchIDFLine( aBoardFile, iline, comment, pos ) )
            continue;

        idx = 0;
        GetIDFString( iline, entry, quoted, idx );

        if( quoted )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 1 of " << GetOutlineTypeString( outlineType );
            ostr << " is quoted\n";
            ostr << "* line: '" << iline << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        // check for the end of the section
        if( entry.size() >= 5 && CompareToken( ".END_", entry.substr( 0, 5 ) ) )
        {
            // rewind to the start of the last line; the routine invoking
            // this is responsible for checking that the current '.END_ ...'
            // matches the section header.
            if(aBoardFile.eof())
                aBoardFile.clear();

            aBoardFile.seekg( pos );

            if( outlines.size() > 0 )
            {
                if( npts > 0 && !closed )
                {
                    ostringstream ostr;
                    ostr << "invalid outline (not closed)\n";
                    ostr << "* file position: " << pos;

                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                }

                // verify winding
                if( !single )
                {
                    if( !outlines.front()->IsCCW() )
                    {
                        ERROR_IDF << "invalid IDF3 file (BOARD_OUTLINE)\n";
                        cerr << "* WARNING: first outline is not in CCW order\n";
                        return;
                    }

                    if( outlines.size() > 1 && outlines.back()->IsCCW() && !outlines.back()->IsCircle() )
                    {
                        ERROR_IDF << "invalid IDF3 file (BOARD_OUTLINE)\n";
                        cerr << "* WARNING: final cutout does not have points in CW order\n";
                        cerr << "* file position: " << pos << "\n";
                        return;
                    }
                }
            }

            return;
        }

        tstr.clear();
        tstr << entry;

        tstr >> tmp;
        if( tstr.fail() )
        {
            if( outlineType == OTLN_COMPONENT && CompareToken( "PROP", entry ) )
            {
                aBoardFile.seekg( pos );
                return;
            }

            do{
                ostringstream ostr;

                ostr << "\n* invalid outline: RECORD 3, FIELD 1 of " << GetOutlineTypeString( outlineType );
                ostr << " is not numeric\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );

            } while( 0 );
        }

        if( tmp != loopidx )
        {
            // index change
            if( npts > 0 && !closed )
            {
                ostringstream ostr;
                ostr << "invalid outline ( outline # " << loopidx << " not closed)\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            if( tmp < 0 )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: RECORD 3, FIELD 1 of " << GetOutlineTypeString( outlineType );
                ostr << " is invalid\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
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
                            clearOutlines();
                            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                              "memory allocation failed" ) );
                        }

                        outlines.push_back( op );
                        loopidx = tmp;
                    }
                    else
                    {
                        ostringstream ostr;

                        ostr << "\n* invalid outline: RECORD 3, FIELD 1 of " << GetOutlineTypeString( outlineType );
                        ostr << " is invalid (must be 0 or 1)\n";
                        ostr << "* line: '" << iline << "'\n";
                        ostr << "* file position: " << pos;

                        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                    }
                }
                else
                {
                    // outline *MUST* have a Loop Index of 0
                    if( tmp != 0 )
                    {
                        ostringstream ostr;

                        ostr << "\n* invalid outline: RECORD 3, FIELD 1 of " << GetOutlineTypeString( outlineType );
                        ostr << " is invalid (must be 0)\n";
                        ostr << "* line: '" << iline << "'\n";
                        ostr << "* file position: " << pos;

                        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                    }

                    op = new IDF_OUTLINE;

                    if( op == NULL )
                    {
                        clearOutlines();
                        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                          "memory allocation failed" ) );
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
                    ostringstream ostr;

                    ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType );
                    ostr << " section may only have one outline\n";
                    ostr << "* line: '" << iline << "'\n";
                    ostr << "* file position: " << pos;

                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                }

                if( tmp - loopidx != 1 )
                {
                    ostringstream ostr;

                    ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType );
                    ostr << " section must have cutouts in numeric order from 1 onwards\n";
                    ostr << "* line: '" << iline << "'\n";
                    ostr << "* file position: " << pos;

                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                }

                // verify winding of previous outline
                if( ( loopidx == 0 && !op->IsCCW() )
                    || ( loopidx > 0 && op->IsCCW() ) )
                {
                    ostringstream ostr;

                    ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                    ostr << "* violation of loop point order rules by Loop Index " << loopidx << "\n";
                    ostr << "* line: '" << iline << "'\n";
                    ostr << "* file position: " << pos;

                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                }

                op = new IDF_OUTLINE;

                if( op == NULL )
                {
                    clearOutlines();
                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                      "memory allocation failed" ) );
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
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 1 of " << GetOutlineTypeString( outlineType );
            ostr << " is invalid\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, entry, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 2 of ";
            ostr << GetOutlineTypeString( outlineType ) << " does not exist\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( quoted )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 2 of ";
            ostr << GetOutlineTypeString( outlineType ) << " must not be in quotes\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        tstr.clear();
        tstr << entry;

        tstr >> x;
        if( tstr.fail() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 2 of ";
            ostr << GetOutlineTypeString( outlineType ) << " is an invalid X value\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, entry, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 3 of ";
            ostr << GetOutlineTypeString( outlineType ) << " does not exist\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( quoted )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 3 of ";
            ostr << GetOutlineTypeString( outlineType ) << " must not be in quotes\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        tstr.clear();
        tstr << entry;

        tstr >> y;
        if( tstr.fail() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 3 of ";
            ostr << GetOutlineTypeString( outlineType ) << " is an invalid Y value\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, entry, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 4 of ";
            ostr << GetOutlineTypeString( outlineType ) << " does not exist\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( quoted )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 4 of ";
            ostr << GetOutlineTypeString( outlineType ) << " must not be in quotes\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        tstr.clear();
        tstr << entry;

        tstr >> ang;
        if( tstr.fail() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: RECORD 3, FIELD 4 of ";
            ostr << GetOutlineTypeString( outlineType ) << " is not a valid angle\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        // the line was successfully read; convert to mm if necessary
        if( unit == UNIT_THOU )
        {
            x *= IDF_THOU_TO_MM;
            y *= IDF_THOU_TO_MM;
        }
        else if( ( aIdfVersion == IDF_V2 ) && ( unit == UNIT_TNM ) )
        {
            x *= IDF_TNM_TO_MM;
            y *= IDF_TNM_TO_MM;
        }
        else if( unit != UNIT_MM )
        {
            ostringstream ostr;
            ostr << "\n* BUG: invalid UNIT type: " << unit;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( npts++ == 0 )
        {
            // first point
            prePt.x = x;
            prePt.y = y;

            // ensure that the first point is not an arc specification
            if( ang < -MIN_ANG || ang > MIN_ANG )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: RECORD 3 of ";
                ostr << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: first point of an outline has a non-zero angle\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }
        }
        else
        {
            // Nth point
            if( closed )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: RECORD 3 of ";
                ostr << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: adding a segment to a closed outline\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
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
                clearOutlines();
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "memory allocation failed" ) );
            }

            if( sp->IsCircle() )
            {
                // this is  a circle; the loop is closed
                if( op->size() != 0 )
                {
                    delete sp;

                    ostringstream ostr;

                    ostr << "\n* invalid outline: RECORD 3 of ";
                    ostr << GetOutlineTypeString( outlineType ) << "\n";
                    ostr << "* violation: adding a circle to a non-empty outline\n";
                    ostr << "* line: '" << iline << "'\n";
                    ostr << "* file position: " << pos;

                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
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
    // 1. ideally we would ensure that there are no arcs with a radius of 0

    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                      "problems reading file (premature end of outline)" ) );

    return;
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

void BOARD_OUTLINE::writeOutline( std::ofstream& aBoardFile, IDF_OUTLINE* aOutline, size_t aIndex )
{
    std::list<IDF_SEGMENT*>::iterator bo;
    std::list<IDF_SEGMENT*>::iterator eo;

    if( !aOutline )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: NULL outline pointer" ) );

    if( aOutline->size() == 1 )
    {
        if( !aOutline->front()->IsCircle() )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "bad outline (single segment item, not circle)" ) );

        if( single )
            aIndex = 0;

        // NOTE: a circle always has an angle of 360, never -360,
        // otherwise SolidWorks chokes on the file.
        if( unit != UNIT_THOU )
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
            << (aOutline->front()->startPoint.x / IDF_THOU_TO_MM) << " "
            << (aOutline->front()->startPoint.y / IDF_THOU_TO_MM) << " 0\n";

            aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
            << (aOutline->front()->endPoint.x / IDF_THOU_TO_MM) << " "
            << (aOutline->front()->endPoint.y / IDF_THOU_TO_MM) << " 360\n";
        }

        return;
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
        if( unit != UNIT_THOU )
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
                << setprecision(2) << -aOutline->front()->angle << "\n";
            }
        }
        else
        {
            if( aOutline->front()->angle < MIN_ANG && aOutline->front()->angle > -MIN_ANG )
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->endPoint.x / IDF_THOU_TO_MM) << " "
                << (aOutline->front()->endPoint.y / IDF_THOU_TO_MM) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->startPoint.x / IDF_THOU_TO_MM) << " "
                << (aOutline->front()->startPoint.y / IDF_THOU_TO_MM) << " 0\n";
            }
            else
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->endPoint.x / IDF_THOU_TO_MM) << " "
                << (aOutline->front()->endPoint.y / IDF_THOU_TO_MM) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << (aOutline->front()->startPoint.x / IDF_THOU_TO_MM) << " "
                << (aOutline->front()->startPoint.y / IDF_THOU_TO_MM) << " "
                << setprecision(2) << -aOutline->front()->angle << "\n";
            }
        }

        // for all other segments we only write out the start point
        while( bo != eo )
        {
            if( unit != UNIT_THOU )
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
                    << setprecision(2) << -(*bo)->angle << "\n";
                }
            }
            else
            {
                if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->startPoint.x / IDF_THOU_TO_MM) << " "
                    << ((*bo)->startPoint.y / IDF_THOU_TO_MM) << " 0\n";
                }
                else
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->startPoint.x / IDF_THOU_TO_MM) << " "
                    << ((*bo)->startPoint.y / IDF_THOU_TO_MM) << " "
                    << setprecision(2) << -(*bo)->angle << "\n";
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
        if( unit != UNIT_THOU )
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
                << setprecision(2) << (*bo)->angle << "\n";
            }
        }
        else
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->startPoint.x / IDF_THOU_TO_MM) << " "
                << ((*bo)->startPoint.y / IDF_THOU_TO_MM) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->endPoint.x / IDF_THOU_TO_MM) << " "
                << ((*bo)->endPoint.y / IDF_THOU_TO_MM) << " 0\n";
            }
            else
            {
                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->startPoint.x / IDF_THOU_TO_MM) << " "
                << ((*bo)->startPoint.y / IDF_THOU_TO_MM) << " 0\n";

                aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                << ((*bo)->endPoint.x / IDF_THOU_TO_MM) << " "
                << ((*bo)->endPoint.y / IDF_THOU_TO_MM) << " "
                << setprecision(2) << (*bo)->angle << "\n";
            }
        }

        ++bo;

        // for all other segments we only write out the last point
        while( bo != eo )
        {
            if( unit != UNIT_THOU )
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
                    << setprecision(2) << (*bo)->angle << "\n";
                }
            }
            else
            {
                if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->endPoint.x / IDF_THOU_TO_MM) << " "
                    << ((*bo)->endPoint.y / IDF_THOU_TO_MM) << " 0\n";
                }
                else
                {
                    aBoardFile << aIndex << " " << setiosflags(ios::fixed) << setprecision(1)
                    << ((*bo)->endPoint.x / IDF_THOU_TO_MM) << " "
                    << ((*bo)->endPoint.y / IDF_THOU_TO_MM) << " "
                    << setprecision(2) << (*bo)->angle << "\n";
                }
            }

            ++bo;
        }
    }

    return;
}

void BOARD_OUTLINE::writeOutlines( std::ofstream& aBoardFile )
{
    if( outlines.empty() )
        return;

    int idx = 0;
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    while( itS != itE )
    {
        writeOutline( aBoardFile, *itS, idx++ );
        ++itS;
    }

    return;
}

bool BOARD_OUTLINE::SetUnit( IDF3::IDF_UNIT aUnit )
{
    // note: although UNIT_TNM is accepted here without reservation,
    // this can only affect data being read from a file.
    if( aUnit != UNIT_MM && aUnit != UNIT_THOU && aUnit != UNIT_TNM )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* BUG: invalid IDF UNIT (must be one of UNIT_MM or UNIT_THOU): " << aUnit << "\n";
        ostr << "* outline type: " << GetOutlineTypeString( outlineType );
        errormsg = ostr.str();

        return false;
    }

    unit = aUnit;
    return true;
}

IDF3::IDF_UNIT BOARD_OUTLINE::GetUnit( void )
{
    return unit;
}

bool BOARD_OUTLINE::setThickness( double aThickness )
{
    if( aThickness < 0.0 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* BUG: aThickness < 0.0\n";
        ostr << "* outline type: " << GetOutlineTypeString( outlineType );
        errormsg = ostr.str();

        return false;
    }

    thickness = aThickness;
    return true;
}

bool BOARD_OUTLINE::SetThickness( double aThickness )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    return setThickness( aThickness );
}

double BOARD_OUTLINE::GetThickness( void )
{
    return thickness;
}

void BOARD_OUTLINE::readData( std::ifstream& aBoardFile, const std::string& aHeader,
                              IDF3::IDF_VERSION aIdfVersion )
{
    //  BOARD_OUTLINE (PANEL_OUTLINE)
    //      .BOARD_OUTLINE  [OWNER]
    //      [thickness]
    //      [outlines]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;
    std::streampos pos;

    pos = aBoardFile.tellg();

    if( !GetIDFString( aHeader, token, quoted, idx ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "invalid invocation: blank header line" ) );

    if( quoted )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: section names may not be in quotes\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !CompareToken( ".BOARD_OUTLINE", token ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: not a board outline\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
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
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within .BOARD_OUTLINE section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no thickness specified\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    std::stringstream teststr;
    teststr << token;

    teststr >> thickness;
    if( teststr.fail() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: invalid RECORD 2 (thickness)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( unit == UNIT_THOU )
    {
        thickness *= IDF_THOU_TO_MM;
    }
    else if( ( aIdfVersion == IDF_V2 ) && ( unit == UNIT_TNM ) )
    {
        thickness *= IDF_TNM_TO_MM;
    }
    else if( unit != UNIT_MM )
    {
        ostringstream ostr;
        ostr << "\n* BUG: invalid UNIT type: " << unit;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    // for some unknown reason IDF allows 0 or negative thickness, but this
    // is a problem so we fix it here
    if( thickness <= 0.0 )
    {
        if( thickness == 0.0 )
        {
            ERROR_IDF << "\n* WARNING: setting board thickness to default 1.6mm (";
            cerr << thickness << ")\n";
            thickness = 1.6;
        }
        else
        {
            thickness = -thickness;
            ERROR_IDF << "\n* WARNING: setting board thickness to positive number (";
            cerr << thickness << ")\n";
        }
    }

    // read RECORD 3 values
    readOutlines( aBoardFile, aIdfVersion );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !CompareToken( ".END_BOARD_OUTLINE", iline ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no .END_BOARD_OUTLINE found\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    return;
}


void BOARD_OUTLINE::writeData( std::ofstream& aBoardFile )
{
    writeComments( aBoardFile );

    // note: a BOARD_OUTLINE section is required, even if it is empty
    aBoardFile << ".BOARD_OUTLINE ";

    writeOwner( aBoardFile );

    if( unit != UNIT_THOU )
        aBoardFile << setiosflags(ios::fixed) << setprecision(5) << thickness << "\n";
    else
        aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_THOU_TO_MM) << "\n";

    writeOutlines( aBoardFile );

    aBoardFile << ".END_BOARD_OUTLINE\n\n";

    return;
}

void BOARD_OUTLINE::clear( void )
{
    comments.clear();
    clearOutlines();

    owner = UNOWNED;
    return;
}

bool BOARD_OUTLINE::Clear( void )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    clear();

    return true;
}

void BOARD_OUTLINE::setParent( IDF3_BOARD* aParent )
{
    parent = aParent;
}

IDF3_BOARD* BOARD_OUTLINE::GetParent( void )
{
    return parent;
}

bool BOARD_OUTLINE::addOutline( IDF_OUTLINE* aOutline )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    try
    {
        while( itS != itE )
        {
            if( *itS == aOutline )
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "duplicate outline pointer" ) );

            ++itS;
        }

        outlines.push_back( aOutline );

    }
    catch( const std::exception& e )
    {
        errormsg = e.what();

        return false;
    }

    return true;
}

bool BOARD_OUTLINE::AddOutline( IDF_OUTLINE* aOutline )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    return addOutline( aOutline );
}

bool BOARD_OUTLINE::DelOutline( IDF_OUTLINE* aOutline )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();
    std::list< IDF_OUTLINE* >::iterator itE = outlines.end();

    if( !aOutline )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* BUG: NULL aOutline pointer\n";
        ostr << "* outline type: " << GetOutlineTypeString( outlineType );
        errormsg = ostr.str();

        return false;
    }

    if( outlines.empty() )
    {
        errormsg.clear();
        return false;
    }

    // if there are more than 1 outlines it makes no sense to delete
    // the first outline (board outline) since that would have the
    // undesirable effect of substituting a cutout outline as the board outline
    if( aOutline == outlines.front() )
    {
        if( outlines.size() > 1 )
        {
            ostringstream ostr;
            ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
            ostr << "* BUG: attempting to delete first outline in list\n";
            ostr << "* outline type: " << GetOutlineTypeString( outlineType );
            errormsg = ostr.str();

            return false;
        }

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

    errormsg.clear();
    return false;
}


bool BOARD_OUTLINE::DelOutline( size_t aIndex )
{
    std::list< IDF_OUTLINE* >::iterator itS = outlines.begin();

    if( outlines.empty() )
    {
        errormsg.clear();
        return false;
    }

    if( aIndex >= outlines.size() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* BUG: index out of bounds (" << aIndex << " / " << outlines.size() << ")\n";
        ostr << "* outline type: " << GetOutlineTypeString( outlineType );
        errormsg = ostr.str();

        return false;
    }

    if( aIndex == 0 )
    {
        // if there are more than 1 outlines it makes no sense to delete
        // the first outline (board outline) since that would have the
        // undesirable effect of substituting a cutout outline as the board outline
        if( outlines.size() > 1 )
        {
            ostringstream ostr;
            ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
            ostr << "* BUG: attempting to delete first outline in list\n";
            ostr << "* outline type: " << GetOutlineTypeString( outlineType );
            errormsg = ostr.str();

            return false;
        }

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
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr <<  "* aIndex (" << aIndex << ") is out of range (" << outlines.size() << ")";
        errormsg = ostr.str();

        return NULL;
    }

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
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    owner = aOwner;
    return true;
}

bool BOARD_OUTLINE::IsSingle( void )
{
    return single;
}

void BOARD_OUTLINE::clearOutlines( void )
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
OTHER_OUTLINE::OTHER_OUTLINE( IDF3_BOARD* aParent )
{
    setParent( aParent );
    outlineType = OTLN_OTHER;
    side = LYR_INVALID;
    single = false;

    return;
}

bool OTHER_OUTLINE::SetOutlineIdentifier( const std::string aUniqueID )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    uniqueID = aUniqueID;

    return true;
}

const std::string& OTHER_OUTLINE::GetOutlineIdentifier( void )
{
    return uniqueID;
}

bool OTHER_OUTLINE::SetSide( IDF3::IDF_LAYER aSide )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    switch( aSide )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
            side = aSide;
            break;

        default:
            do{
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "* BUG: invalid side (" << aSide << "); must be one of TOP/BOTTOM\n";
                ostr << "* outline type: " << GetOutlineTypeString( outlineType );
                errormsg = ostr.str();
            } while( 0 );

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

void OTHER_OUTLINE::readData( std::ifstream& aBoardFile, const std::string& aHeader,
                              IDF3::IDF_VERSION aIdfVersion )
{
    // OTHER_OUTLINE/VIA_KEEPOUT
    //     .OTHER_OUTLINE  [OWNER]
    //     [outline identifier] [thickness] [board side: Top/Bot] {not present in VA\IA KEEPOUT}
    //     [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;
    std::streampos pos = aBoardFile.tellg();

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        ostringstream ostr;
        ostr << "\n* BUG: invalid invocation: blank header line\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( quoted )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: section names must not be in quotes\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( outlineType == OTLN_OTHER )
    {
        if( !CompareToken( ".OTHER_OUTLINE", token ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* BUG: not an .OTHER outline\n";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }
    else
    {
        if( !CompareToken( ".VIA_KEEPOUT", token ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* BUG: not a .VIA_KEEPOUT outline\n";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
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

    if( outlineType == OTLN_OTHER )
    {
        // check RECORD 2
        // [outline identifier] [thickness] [board side: Top/Bot]
        while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

        if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: premature end\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        idx = 0;
        if( comment )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: comment within .OTHER_OUTLINE section\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no outline identifier\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        uniqueID = token;

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no thickness\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        std::stringstream teststr;
        teststr << token;

        teststr >> thickness;
        if( teststr.fail() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: invalid thickness\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( unit == UNIT_THOU )
        {
            thickness *= IDF_THOU_TO_MM;
        }
        else if( ( aIdfVersion == IDF_V2 ) && ( unit == UNIT_TNM ) )
        {
            thickness *= IDF_TNM_TO_MM;
        }
        else if( unit != UNIT_MM )
        {
            ostringstream ostr;
            ostr << "\n* BUG: invalid UNIT type: " << unit;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( aIdfVersion == IDF_V2 )
        {
            side = LYR_TOP;
        }
        else
        {
            if( !GetIDFString( iline, token, quoted, idx ) )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: no board side\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            if( !ParseIDFLayer( token, side ) || ( side != LYR_TOP && side != LYR_BOTTOM ) )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: invalid side (must be TOP or BOTTOM only)\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }
        }

    }

    // read RECORD 3 values
    readOutlines( aBoardFile, aIdfVersion );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( outlineType == OTLN_OTHER )
    {
        if( !CompareToken( ".END_OTHER_OUTLINE", iline ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no .END_OTHER_OUTLINE found\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }
    else
    {
        if( !CompareToken( ".END_VIA_KEEPOUT", iline ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no .END_VIA_KEEPOUT found\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    return;
}

void OTHER_OUTLINE::writeData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return;

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

        if( unit != UNIT_THOU )
            aBoardFile << setiosflags(ios::fixed) << setprecision(5) << thickness << " ";
        else
            aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_THOU_TO_MM) << " ";

        switch( side )
        {
            case LYR_TOP:
            case LYR_BOTTOM:
                WriteLayersText( aBoardFile, side );
                break;

            default:
                do{
                    ostringstream ostr;
                    ostr << "\n* invalid OTHER_OUTLINE side (neither top nor bottom): ";
                    ostr << side;
                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
                } while( 0 );

                break;
        }
    }

    // write RECORD 3
    writeOutlines( aBoardFile );

    // write RECORD 4
    if( outlineType == OTLN_OTHER )
        aBoardFile << ".END_OTHER_OUTLINE\n\n";
    else
        aBoardFile << ".END_VIA_KEEPOUT\n\n";

    return;
}


bool OTHER_OUTLINE::Clear( void )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    clear();
    side = LYR_INVALID;
    uniqueID.clear();

    return true;
}


/*
 * CLASS: ROUTE_OUTLINE
 */
ROUTE_OUTLINE::ROUTE_OUTLINE( IDF3_BOARD* aParent )
{
    setParent( aParent );
    outlineType = OTLN_ROUTE;
    single = true;
    layers = LYR_INVALID;
}

bool ROUTE_OUTLINE::SetLayers( IDF3::IDF_LAYER aLayer )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    layers = aLayer;

    return true;
}

IDF3::IDF_LAYER ROUTE_OUTLINE::GetLayers( void )
{
    return layers;
}

void ROUTE_OUTLINE::readData( std::ifstream& aBoardFile, const std::string& aHeader,
                              IDF3::IDF_VERSION aIdfVersion )
{
    //  ROUTE_OUTLINE (or ROUTE_KEEPOUT)
    //      .ROUTE_OUTLINE [OWNER]
    //      [layers]
    //      [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;
    std::streampos pos = aBoardFile.tellg();

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: invalid invocation; blank header line" ) );
    }

    if( quoted )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: section names must not be in quotes\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( outlineType == OTLN_ROUTE )
    {
        if( !CompareToken( ".ROUTE_OUTLINE", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "\n* BUG: not a ROUTE outline" ) );
    }
    else
    {
        if( !CompareToken( ".ROUTE_KEEPOUT", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "\n* BUG: not a ROUTE KEEPOUT outline" ) );
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
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

    if( aIdfVersion > IDF_V2 || outlineType == OTLN_ROUTE_KEEPOUT )
    {
        while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

        if( !aBoardFile.good() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: premature end\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        idx = 0;
        if( comment )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: comment within a section\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no layers specification\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( quoted )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: layers specification must not be in quotes\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !ParseIDFLayer( token, layers ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: invalid layers specification\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( aIdfVersion == IDF_V2 )
        {
            if( layers == LYR_INNER || layers == LYR_ALL )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: IDFv2 allows only TOP/BOTTOM/BOTH; layer was '";
                ostr << token << "'\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }
        }

    }   // RECORD 2, conditional > IDFv2 or ROUTE_KO_OUTLINE
    else
    {
        layers = LYR_ALL;
    }

    // read RECORD 3 values
    readOutlines( aBoardFile, aIdfVersion );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( outlineType == OTLN_ROUTE )
    {
        if( !CompareToken( ".END_ROUTE_OUTLINE", iline ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no .END_ROUTE_OUTLINE found\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }
    else
    {
        if( !CompareToken( ".END_ROUTE_KEEPOUT", iline ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no .END_ROUTE_KEEPOUT found\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    return;
}


void ROUTE_OUTLINE::writeData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return;

    if( layers == LYR_INVALID )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "layer not specified" ) );

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
    writeOutlines( aBoardFile );

    // write RECORD 4
    if( outlineType == OTLN_ROUTE )
        aBoardFile << ".END_ROUTE_OUTLINE\n\n";
    else
        aBoardFile << ".END_ROUTE_KEEPOUT\n\n";

    return;
}


bool ROUTE_OUTLINE::Clear( void )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    clear();
    layers = LYR_INVALID;

    return true;
}


/*
 * CLASS: PLACE_OUTLINE
 */
PLACE_OUTLINE::PLACE_OUTLINE( IDF3_BOARD* aParent )
{
    setParent( aParent );
    outlineType = OTLN_PLACE;
    single = true;
    thickness = -1.0;
    side = LYR_INVALID;
}


bool PLACE_OUTLINE::SetSide( IDF3::IDF_LAYER aSide )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    switch( aSide )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
        case LYR_BOTH:
            side = aSide;
            break;

        default:
            do{
                side = LYR_INVALID;
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "* BUG: invalid layer (" << aSide << "): must be one of TOP/BOTTOM/BOTH\n";
                ostr << "* outline type: " << GetOutlineTypeString( outlineType );
                errormsg = ostr.str();

                return false;
            } while( 0 );

            break;
    }

    return true;
}


IDF3::IDF_LAYER PLACE_OUTLINE::GetSide( void )
{
    return side;
}


bool PLACE_OUTLINE::SetMaxHeight( double aHeight )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    if( aHeight < 0.0 )
    {
        thickness = 0.0;

        do{
            ostringstream ostr;
            ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
            ostr << "* BUG: invalid height (" << aHeight << "): must be >= 0.0";
            ostr << "* outline type: " << GetOutlineTypeString( outlineType );
            errormsg = ostr.str();

            return false;
        } while( 0 );
    }

    thickness = aHeight;
    return true;
}

double PLACE_OUTLINE::GetMaxHeight( void )
{
    return thickness;
}

void PLACE_OUTLINE::readData( std::ifstream& aBoardFile, const std::string& aHeader,
                              IDF3::IDF_VERSION aIdfVersion )
{
    //  PLACE_OUTLINE/KEEPOUT
    //      .PLACE_OUTLINE [OWNER]
    //      [board side: Top/Bot/Both] [height]
    //      [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;
    std::streampos pos = aBoardFile.tellg();

    if( !GetIDFString( aHeader, token, quoted, idx ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: invalid invocation: blank header line\n" ) );

    if( quoted )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: section name must not be in quotes\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( outlineType == OTLN_PLACE )
    {
        if( !CompareToken( ".PLACE_OUTLINE", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "\n* BUG: not a .PLACE_OUTLINE" ) );
    }
    else
    {
        if( !CompareToken( ".PLACE_KEEPOUT", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "\n* BUG: not a .PLACE_KEEPOUT" ) );
    }

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
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

    if( aIdfVersion > IDF_V2 || outlineType == OTLN_PLACE_KEEPOUT )
    {
        while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

        if( !aBoardFile.good() )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: premature end\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        idx = 0;
        if( comment )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: comment within the section\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no board side information\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !ParseIDFLayer( token, side ) ||
            ( side != LYR_TOP && side != LYR_BOTTOM && side != LYR_BOTH ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: invalid board side: must be one of TOP/BOTTOM/BOTH\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( GetIDFString( iline, token, quoted, idx ) )
        {
            std::stringstream teststr;
            teststr << token;

            teststr >> thickness;

            if( teststr.fail() )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: invalid height\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            if( thickness < 0.0 )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: thickness < 0\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            if( unit == UNIT_THOU )
            {
                thickness *= IDF_THOU_TO_MM;
            }
            else if( ( aIdfVersion == IDF_V2 ) && ( unit == UNIT_TNM ) )
            {
                thickness *= IDF_TNM_TO_MM;
            }
            else if( unit != UNIT_MM )
            {
                ostringstream ostr;
                ostr << "\n* BUG: invalid UNIT type: " << unit;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            if( thickness < 0.0 )
                thickness = 0.0;

        }
        else
        {
            // for OTLN_PLACE, thickness may be omitted, but is required for OTLN_PLACE_KEEPOUT
            if( outlineType == OTLN_PLACE_KEEPOUT )
            {
                ostringstream ostr;

                ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
                ostr << "* violation: missing thickness\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            thickness = -1.0;
        }
    }
    else
    {
        side = LYR_TOP;
        thickness = 0.0;
    }

    // read RECORD 3 values
    readOutlines( aBoardFile, aIdfVersion );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( outlineType == OTLN_PLACE )
    {
        if( !GetIDFString( iline, token, quoted, idx )
            || !CompareToken( ".END_PLACE_OUTLINE", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid .PLACE_OUTLINE section: no .END_PLACE_OUTLINE found" ) );
    }
    else
    {
        if( !GetIDFString( iline, token, quoted, idx )
            || !CompareToken( ".END_PLACE_KEEPOUT", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid .PLACE_KEEPOUT section: no .END_PLACE_KEEPOUT found" ) );
    }

    return;
}

void PLACE_OUTLINE::writeData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return;

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
            do
            {
                ostringstream ostr;
                ostr << "\n* invalid PLACE_OUTLINE/KEEPOUT side (";
                ostr << side << "); must be one of TOP/BOTTOM/BOTH";
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            } while( 0 );

            break;
    }

    // thickness is optional for OTLN_PLACE, but mandatory for OTLN_PLACE_KEEPOUT
    if( thickness < 0.0 && outlineType == OTLN_PLACE_KEEPOUT)
    {
        aBoardFile << "\n";
    }
    else
    {
        aBoardFile << " ";

        if( unit != UNIT_THOU )
            aBoardFile << setiosflags(ios::fixed) << setprecision(5) << thickness << "\n";
        else
            aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_THOU_TO_MM) << "\n";
    }

    // write RECORD 3
    writeOutlines( aBoardFile );

    // write RECORD 4
    if( outlineType == OTLN_PLACE )
        aBoardFile << ".END_PLACE_OUTLINE\n\n";
    else
        aBoardFile << ".END_PLACE_KEEPOUT\n\n";

    return;
}


bool PLACE_OUTLINE::Clear( void )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    clear();
    thickness = 0.0;
    side = LYR_INVALID;

    return true;
}


/*
 * CLASS: ROUTE_KEEPOUT
 */
ROUTE_KO_OUTLINE::ROUTE_KO_OUTLINE( IDF3_BOARD* aParent )
    : ROUTE_OUTLINE( aParent )
{
    outlineType = OTLN_ROUTE_KEEPOUT;
    return;
}


/*
 * CLASS: PLACE_KEEPOUT
 */
PLACE_KO_OUTLINE::PLACE_KO_OUTLINE( IDF3_BOARD* aParent )
    : PLACE_OUTLINE( aParent )
{
    outlineType = OTLN_PLACE_KEEPOUT;
    return;
}


/*
 * CLASS: VIA_KEEPOUT
 */
VIA_KO_OUTLINE::VIA_KO_OUTLINE( IDF3_BOARD* aParent )
    : OTHER_OUTLINE( aParent )
{
    single = true;
    outlineType = OTLN_VIA_KEEPOUT;
}


/*
 * CLASS: PLACEMENT GROUP (PLACE_REGION)
 */
GROUP_OUTLINE::GROUP_OUTLINE( IDF3_BOARD* aParent )
{
    setParent( aParent );
    outlineType = OTLN_GROUP_PLACE;
    thickness = 0.0;
    side = LYR_INVALID;
    single = true;
    return;
}


bool GROUP_OUTLINE::SetSide( IDF3::IDF_LAYER aSide )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    switch( aSide )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
        case LYR_BOTH:
            side = aSide;
            break;

        default:
            do{
                ostringstream ostr;
                ostr << "invalid side (" << aSide << "); must be one of TOP/BOTTOM/BOTH\n";
                ostr << "* outline type: " << GetOutlineTypeString( outlineType );
                errormsg = ostr.str();

                return false;
            } while( 0 );

            break;
    }

    return true;
}


IDF3::IDF_LAYER GROUP_OUTLINE::GetSide( void )
{
    return side;
}


bool GROUP_OUTLINE::SetGroupName( std::string aGroupName )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    groupName = aGroupName;

    return true;
}


const std::string& GROUP_OUTLINE::GetGroupName( void )
{
    return groupName;
}


void GROUP_OUTLINE::readData( std::ifstream& aBoardFile, const std::string& aHeader,
                              IDF3::IDF_VERSION aIdfVersion )
{
    //  Placement Group
    //      .PLACE_REGION [OWNER]
    //      [side: Top/Bot/Both ] [component group name]
    //      [outline]

    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;
    std::streampos pos = aBoardFile.tellg();

    if( !GetIDFString( aHeader, token, quoted, idx ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: invalid invocation: blank header line" ) );

    if( quoted )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: section name must not be in quotes\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !CompareToken( ".PLACE_REGION", token ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: not a .PLACE_REGION" ) );

    if( !GetIDFString( aHeader, token, quoted, idx ) )
    {
        if( aIdfVersion > IDF_V2 )
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

    // check RECORD 2
    // [side: Top/Bot/Both ] [component group name]
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( !aBoardFile.good() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no board side specified\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !ParseIDFLayer( token, side ) ||
        ( side != LYR_TOP && side != LYR_BOTTOM && side != LYR_BOTH ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: invalid board side, must be one of TOP/BOTTOM/BOTH\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no outline identifier\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    groupName = token;

    // read RECORD 3 values
    readOutlines( aBoardFile, aIdfVersion );

    // check RECORD 4
    while( aBoardFile.good() && !FetchIDFLine( aBoardFile, iline, comment, pos ) );

    if( ( !aBoardFile.good() && aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx )
        || !CompareToken( ".END_PLACE_REGION", token ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* invalid .PLACE_REGION section: no .END_PLACE_REGION found" ) );

    return;
}


void GROUP_OUTLINE::writeData( std::ofstream& aBoardFile )
{
    // this section is optional; do not write if not required
    if( outlines.empty() )
        return;

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
            do{
                ostringstream ostr;
                ostr << "\n* invalid PLACE_REGION side (must be TOP/BOTTOM/BOTH): ";
                ostr << side;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            } while( 0 );

            break;
    }

    aBoardFile << " \"" << groupName << "\"\n";

    // write RECORD 3
    writeOutlines( aBoardFile );

    // write RECORD 4
    aBoardFile << ".END_PLACE_REGION\n\n";

    return;
}

bool GROUP_OUTLINE::Clear( void )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    clear();
    thickness = 0.0;
    side = LYR_INVALID;
    groupName.clear();

    return true;
}

/*
 * CLASS: COMPONENT OUTLINE
 */
IDF3_COMP_OUTLINE::IDF3_COMP_OUTLINE( IDF3_BOARD* aParent )
{
    setParent( aParent );
    single = true;
    outlineType = OTLN_COMPONENT;
    compType = COMP_INVALID;
    refNum = 0;
    return;
}

void IDF3_COMP_OUTLINE::readProperties( std::ifstream& aLibFile )
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
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: comment within section\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: bad property section (no PROP)\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( quoted )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: PROP or .END must not be quoted\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( token.size() >= 5 && CompareToken( ".END_", token.substr( 0, 5 ) ) )
        {
            if(aLibFile.eof())
                aLibFile.clear();

            aLibFile.seekg( pos );
            return;
        }

        if( !CompareToken( "PROP", token ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: expecting PROP or .END_ELECTRICAL\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no PROP name\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        pname = token;

        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no PROP value\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        pval = token;

        if( props.insert( pair< string, string >(pname, pval) ).second == false )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: duplicate property name \"" << pname << "\"\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    return;
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

void IDF3_COMP_OUTLINE::readData( std::ifstream& aLibFile, const std::string& aHeader,
                                  IDF3::IDF_VERSION aIdfVersion )
{
    //  .ELECTRICAL/.MECHANICAL
    //  [GEOM] [PART] [UNIT] [HEIGHT]
    //  [outline]
    //  [PROP] [prop name] [prop value]
    // check RECORD 1
    std::string token;
    bool quoted = false;
    int  idx = 0;
    std::streampos pos = aLibFile.tellg();

    if( !GetIDFString( aHeader, token, quoted, idx ) )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: invalid invocation: blank header line" ) );

    if( quoted )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: section name must not be in quotes\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
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
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: expecting .ELECTRICAL or .MECHANICAL header\n";
        ostr << "* line: '" << aHeader << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    // check RECORD 2
    // [GEOM] [PART] [UNIT] [HEIGHT]
    std::string iline;
    bool comment = false;

    while( aLibFile.good() && !FetchIDFLine( aLibFile, iline, comment, pos ) );

    if( !aLibFile.good() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no GEOMETRY NAME\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    geometry = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no PART NAME\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    part = token;

    if( part.empty() && geometry.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: both GEOMETRY and PART names are empty\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no UNIT type\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( CompareToken( "MM", token ) )
    {
        unit = UNIT_MM;
    }
    else if( CompareToken( "THOU", token ) )
    {
        unit = UNIT_THOU;
    }
    else if( aIdfVersion == IDF_V2 && !CompareToken( "TNM", token ) )
    {
        unit = UNIT_TNM;
    }
    else
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: invalid UNIT '" << token << "': must be one of MM or THOU\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: no height specified\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    std::istringstream teststr;
    teststr.str( token );

    teststr >> thickness;
    if( teststr.fail() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: invalid height '" << token << "'\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( unit == UNIT_THOU )
    {
        thickness *= IDF_THOU_TO_MM;
    }
    else if( ( aIdfVersion == IDF_V2 ) && ( unit == UNIT_TNM ) )
    {
        thickness *= IDF_TNM_TO_MM;
    }
    else if( unit != UNIT_MM )
    {
        ostringstream ostr;
        ostr << "\n* BUG: invalid UNIT type: " << unit;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    // read RECORD 3 values
    readOutlines( aLibFile, aIdfVersion );

    if( compType == COMP_ELEC && aIdfVersion > IDF_V2 )
        readProperties( aLibFile );

    // check RECORD 4
    while( aLibFile.good() && !FetchIDFLine( aLibFile, iline, comment, pos ) );

    if( ( !aLibFile.good() && aLibFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: premature end\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    if( comment )
    {
        ostringstream ostr;

        ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
        ostr << "* violation: comment within section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( compType == COMP_ELEC )
    {
        if( !CompareToken( ".END_ELECTRICAL", iline ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no .END_ELECTRICAL found\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }
    else
    {
        if( !CompareToken( ".END_MECHANICAL", iline ) )
        {
            ostringstream ostr;

            ostr << "\n* invalid outline: " << GetOutlineTypeString( outlineType ) << "\n";
            ostr << "* violation: no .END_MECHANICAL found\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    return;
}


void IDF3_COMP_OUTLINE::writeData( std::ofstream& aLibFile )
{
    if( refNum == 0 )
        return;    // nothing to do

    if( compType != COMP_ELEC && compType != COMP_MECH )
    {
        ostringstream ostr;
        ostr << "\n* component type not set or invalid: " << compType;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    writeComments( aLibFile );

    // note: the outline section is required, even if it is empty
    if( compType == COMP_ELEC )
        aLibFile << ".ELECTRICAL\n";
    else
        aLibFile << ".MECHANICAL\n";

    // RECORD 2
    // [GEOM] [PART] [UNIT] [HEIGHT]
    aLibFile << "\"" << geometry << "\" \"" << part << "\" ";

    if( unit != UNIT_THOU )
        aLibFile << "MM " << setiosflags(ios::fixed) << setprecision(5) << thickness << "\n";
    else
        aLibFile << "THOU " << setiosflags(ios::fixed) << setprecision(1) << (thickness / IDF_THOU_TO_MM) << "\n";

    writeOutlines( aLibFile );

    if( compType == COMP_ELEC )
    {
        writeProperties( aLibFile );
        aLibFile << ".END_ELECTRICAL\n\n";
    }
    else
    {
        aLibFile << ".END_MECHANICAL\n\n";
    }

    return;
}


bool IDF3_COMP_OUTLINE::Clear( void )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !CheckOwnership( __LINE__, __FUNCTION__, parent, owner, outlineType, errormsg ) )
        return false;
#endif

    clear();
    uid.clear();
    geometry.clear();
    part.clear();
    compType = COMP_INVALID;
    refNum = 0;
    props.clear();

    return true;
}

bool IDF3_COMP_OUTLINE::SetComponentClass( IDF3::COMP_TYPE aCompClass )
{
    switch( aCompClass )
    {
        case COMP_ELEC:
        case COMP_MECH:
            compType = aCompClass;
            break;

        default:
            do{
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "* BUG: invalid component class (must be ELECTRICAL or MECHANICAL): ";
                ostr << aCompClass << "\n";
                errormsg = ostr.str();

                return false;
            } while( 0 );

            break;
    }

    return true;
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


int IDF3_COMP_OUTLINE::incrementRef( void )
{
    return ++refNum;
}

int IDF3_COMP_OUTLINE::decrementRef( void )
{
    if( refNum == 0 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* BUG:  decrementing refNum beyond 0";
        errormsg = ostr.str();

        return -1;
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
