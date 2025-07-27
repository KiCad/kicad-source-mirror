/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017  Cirilo Bernardo
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


#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cerrno>
#include <algorithm>
#include <utility>
#include <wx/string.h>
#include <wx/filename.h>

#include "idf_parser.h"
#include "idf_helpers.h"
#include "streamwrapper.h"

using namespace std;
using namespace IDF3;


static bool MatchCompOutline( IDF3_COMP_OUTLINE* aOutlineA, IDF3_COMP_OUTLINE* aOutlineB )
{
    if( aOutlineA->GetComponentClass() != aOutlineB->GetComponentClass() )
        return false;

    if( aOutlineA->OutlinesSize() != aOutlineB->OutlinesSize() )
        return false;

    // are both outlines empty?
    if( aOutlineA->OutlinesSize() == 0 )
        return true;

    IDF_OUTLINE* opA = aOutlineA->GetOutline( 0 );
    IDF_OUTLINE* opB = aOutlineB->GetOutline( 0 );

    if( opA->size() != opB->size() )
        return false;

    if( opA->size() == 0 )
        return true;

    std::list<IDF_SEGMENT*>::iterator olAs = opA->begin();
    std::list<IDF_SEGMENT*>::iterator olAe = opA->end();
    std::list<IDF_SEGMENT*>::iterator olBs = opB->begin();

    while( olAs != olAe )
    {
        if( !(*olAs)->MatchesStart( (*olBs)->startPoint ) )
            return false;

        if( !(*olAs)->MatchesEnd( (*olBs)->endPoint ) )
            return false;

        ++olAs;
        ++olBs;
    }

    return true;
}


IDF3_COMP_OUTLINE_DATA::IDF3_COMP_OUTLINE_DATA()
{
    parent = nullptr;
    outline = nullptr;
    xoff = 0.0;
    yoff = 0.0;
    zoff = 0.0;
    aoff = 0.0;
}


IDF3_COMP_OUTLINE_DATA::IDF3_COMP_OUTLINE_DATA( IDF3_COMPONENT* aParent,
                                                IDF3_COMP_OUTLINE* aOutline )
{
    parent = aParent;
    outline = aOutline;
    xoff = 0.0;
    yoff = 0.0;
    zoff = 0.0;
    aoff = 0.0;

    if( aOutline )
        aOutline->incrementRef();
}


IDF3_COMP_OUTLINE_DATA::IDF3_COMP_OUTLINE_DATA( IDF3_COMPONENT* aParent,
                                                IDF3_COMP_OUTLINE* aOutline,
                                                double aXoff, double aYoff,
                                                double aZoff, double aAngleOff )
{
    parent = aParent;
    outline = aOutline;
    xoff = aXoff;
    yoff = aYoff;
    zoff = aZoff;
    aoff = aAngleOff;
}


IDF3_COMP_OUTLINE_DATA::~IDF3_COMP_OUTLINE_DATA()
{
    if( outline )
        outline->decrementRef();
}


#ifndef DISABLE_IDF_OWNERSHIP
bool IDF3_COMP_OUTLINE_DATA::checkOwnership( int aSourceLine, const char* aSourceFunc )
{
    if( !parent )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc << "():\n";
        ostr << "* BUG: IDF3_COMP_OUTLINE_DATA::parent not set; cannot enforce ownership rules\n";
        errormsg = ostr.str();

        return false;
    }

    IDF3::IDF_PLACEMENT placement = parent->GetPlacement();
    IDF3::CAD_TYPE parentCAD = parent->GetCadType();

    if( placement == PS_PLACED || placement == PS_UNPLACED )
        return true;

    if( placement == PS_MCAD && parentCAD == CAD_MECH )
        return true;

    if( placement == PS_ECAD && parentCAD == CAD_ELEC )
        return true;

    do
    {
        ostringstream ostr;
        ostr << "* " << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc << "():\n";
        ostr << "* ownership violation; CAD type is ";

        if( parentCAD == CAD_MECH )
            ostr << "MCAD ";
        else
            ostr << "ECAD ";

        ostr << "while outline owner is " << GetPlacementString( placement ) << "\n";
        errormsg = ostr.str();

    } while( 0 );

    return false;
}
#endif


bool IDF3_COMP_OUTLINE_DATA::SetOffsets( double aXoff, double aYoff,
                                         double aZoff, double aAngleOff )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    xoff = aXoff;
    yoff = aYoff;
    zoff = aZoff;
    aoff = aAngleOff;
    return true;
}


void IDF3_COMP_OUTLINE_DATA::GetOffsets( double& aXoff, double& aYoff,
                                         double& aZoff, double& aAngleOff )
{
    aXoff = xoff;
    aYoff = yoff;
    aZoff = zoff;
    aAngleOff = aoff;
}


void IDF3_COMP_OUTLINE_DATA::SetParent( IDF3_COMPONENT* aParent )
{
    parent = aParent;
}


bool IDF3_COMP_OUTLINE_DATA::SetOutline( IDF3_COMP_OUTLINE* aOutline )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    if( outline )
        outline->decrementRef();

    outline = aOutline;

    if( outline )
        outline->incrementRef();

    return true;
}


bool IDF3_COMP_OUTLINE_DATA::readPlaceData( std::istream &aBoardFile,
                                            IDF3::FILE_STATE& aBoardState,
                                            IDF3_BOARD *aBoard,
                                            IDF3::IDF_VERSION aIdfVersion,
                                            bool aNoSubstituteOutlines )
{
    if( !aBoard )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: invoked with no reference to the parent IDF_BOARD" ) );

    // clear out data possibly left over from previous use of the object
    outline = nullptr;
    parent  = nullptr;

    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;
    std::string uid;
    std::string refdes;
    IDF3::IDF_PLACEMENT placement = IDF3::PS_UNPLACED;
    IDF3::IDF_LAYER side = IDF3::LYR_TOP;

    // RECORD 2: 'package name', 'part number', 'Refdes' (any, NOREFDES, BOARD)
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: could not read PLACEMENT section\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( isComment )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: comment within PLACEMENT section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( !quoted && CompareToken( ".END_PLACEMENT", token ) )
    {
        aBoardState = IDF3::FILE_PLACEMENT;
        return false;
    }

    std::string ngeom = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: no PART NAME in PLACEMENT RECORD2\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    std::string npart = token;
    uid = ngeom + "_" + npart;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: no REFDES in PLACEMENT RECORD2\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( CompareToken( "NOREFDES", token ) )
    {
        // according to the IDF3.0 specification, this is a
        // mechanical component. The specification is defective
        // since it is impossible to associate mechanical
        // components with their holes unless the mechanical
        // component is given a unique RefDes. This class of defect
        // is one reason IDF does not work well in faithfully
        // conveying information between ECAD and MCAD.
        refdes = aBoard->GetNewRefDes();
    }
    else if( CompareToken( "BOARD", token ) )
    {
        ostringstream ostr;

        ostr << "UNSUPPORTED FEATURE\n";
        ostr << "* RefDes is 'BOARD', indicating this is a PANEL FILE (not supported)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }
    else if( CompareToken( "PANEL", token ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: RefDes in PLACEMENT RECORD2 is 'PANEL'\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }
    else if( token.empty() )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: empty RefDes string in PLACEMENT RECORD2\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }
    else
    {
        // note: perversely, spaces can be a valid RefDes
        refdes = token;
    }

    // V2: RECORD 3: X, Y, ROT, SIDE (top/bot), PLACEMENT (fixed, placed, unplaced)
    // V3: RECORD 3: X, Y, Z, ROT, SIDE (top/bot), PLACEMENT (placed, unplaced, mcad, ecad)
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* problems reading PLACEMENT SECTION, RECORD 3\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( isComment )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: comment within PLACEMENT section\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: X value must not be in quotes (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    istringstream istr;
    istr.str( token );

    istr >> xoff;
    if( istr.fail() )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: X value is not numeric (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: no Y value (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    istr.clear();
    istr.str( token );

    istr >> yoff;
    if( istr.fail() )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: Y value is not numeric (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( aIdfVersion > IDF_V2 )
    {
        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ostringstream ostr;

            ostr << "invalid IDFv3 file\n";
            ostr << "* violation: no Z value (PLACEMENT RECORD 3)\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        istr.clear();
        istr.str( token );

        istr >> zoff;
        if( istr.fail() )
        {
            ostringstream ostr;

            ostr << "invalid IDFv3 file\n";
            ostr << "* violation: Z value is not numeric (PLACEMENT RECORD 3)\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: no rotation value (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    istr.clear();
    istr.str( token );

    istr >> aoff;
    if( istr.fail() )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: rotation value is not numeric (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: no SIDE value (PLACEMENT RECORD 3)\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( CompareToken( "TOP", token ) )
    {
        side = IDF3::LYR_TOP;
    }
    else if( CompareToken( "BOTTOM", token ) )
    {
        side = IDF3::LYR_BOTTOM;
    }
    else
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: invalid SIDE value in PLACEMENT RECORD 3 ('";
        ostr << token << "'); must be one of TOP/BOTTOM\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: no PLACEMENT value in PLACEMENT RECORD 3\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( CompareToken( "PLACED", token ) )
    {
        placement = IDF3::PS_PLACED;
    }
    else if( CompareToken( "UNPLACED", token ) )
    {
        placement = IDF3::PS_UNPLACED;
    }
    else if( aIdfVersion > IDF_V2 && CompareToken( "MCAD", token ) )
    {
        placement = IDF3::PS_MCAD;
    }
    else if( aIdfVersion > IDF_V2 && CompareToken( "ECAD", token ) )
    {
        placement = IDF3::PS_ECAD;
    }
    else if( aIdfVersion < IDF_V3 && CompareToken( "FIXED", token ) )
    {
        if( aBoard->GetCadType() == CAD_ELEC )
            placement = IDF3::PS_MCAD;
        else
            placement = IDF3::PS_ECAD;
    }
    else
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* violation: invalid PLACEMENT value ('";
        ostr << token << "') in PLACEMENT RECORD 3\n";
        ostr << "* line: '" << iline << "'\n";
        ostr << "* file position: " << pos;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    outline = aBoard->GetComponentOutline( uid );

    if( outline == nullptr && !aNoSubstituteOutlines )
    {
        ERROR_IDF << "MISSING OUTLINE\n";
        cerr << "* GeomName( " << ngeom << " ), PartName( " << npart << " )\n";
        cerr << "* Substituting default outline.\n";
        outline = aBoard->GetInvalidOutline( ngeom, npart );

        if( outline == nullptr )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "\n* missing outline: cannot create default" ) );
    }

    if( aBoard->GetUnit() == IDF3::UNIT_THOU )
    {
        xoff *= IDF_THOU_TO_MM;
        yoff *= IDF_THOU_TO_MM;
        zoff *= IDF_THOU_TO_MM;
    }

    parent = aBoard->FindComponent( refdes );

    if( parent == nullptr )
    {
        IDF3_COMPONENT* cp;

        try
        {
            cp = new IDF3_COMPONENT( aBoard );
        }
        catch( std::bad_alloc& )
        {
            outline = nullptr;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "cannot create component object" ) );
        }

        cp->SetRefDes( refdes );
        cp->SetPosition( xoff, yoff, aoff, side );
        cp->SetPlacement( placement );

        xoff = 0;
        yoff = 0;
        aoff = 0;

        aBoard->AddComponent( cp );

        parent = cp;
    }
    else
    {
        double tX, tY, tA;
        IDF3::IDF_LAYER tL;

        if( parent->GetPosition( tX, tY, tA, tL ) )
        {
            if( side != tL )
            {
                outline = nullptr;
                ostringstream ostr;

                ostr << "invalid IDF file\n";
                ostr << "* violation: inconsistent PLACEMENT data; ";
                ostr << "* SIDE value has changed from " << GetLayerString( tL );
                ostr << " to " << GetLayerString( side ) << "\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* file position: " << pos;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            xoff -= tX;
            yoff -= tY;
            aoff -= tA;
        }
        else
        {
            parent->SetPosition( xoff, yoff, aoff, side );
            parent->SetPlacement( placement );

            xoff = 0;
            yoff = 0;
            aoff = 0;
        }

        if( placement != parent->GetPlacement() )
        {
            outline = nullptr;
            ostringstream ostr;

            ostr << "invalid IDF file\n";
            ostr << "* violation: inconsistent PLACEMENT data; ";
            ostr << "* PLACEMENT value has changed from ";
            ostr << GetPlacementString( parent->GetPlacement() );
            ostr << " to " << GetPlacementString( placement ) << "\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* file position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

    }

    // copy internal data to a new object and push it into the component's outline list
    IDF3_COMP_OUTLINE_DATA* cdp = new IDF3_COMP_OUTLINE_DATA;
    *cdp = *this;
    if( outline ) outline->incrementRef();
    outline = nullptr;

    if( !parent->AddOutlineData( cdp ) )
    {
        delete cdp;

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "could not add outline data object" ) );
    }

    return true;
}


void IDF3_COMP_OUTLINE_DATA::writePlaceData( std::ostream& aBoardFile,
                                             double aXpos, double aYpos, double aAngle,
                                             const std::string& aRefDes,
                                             IDF3::IDF_PLACEMENT aPlacement,
                                             IDF3::IDF_LAYER aSide )
{
    if( outline == nullptr )
        return;

    if( outline->GetUID().empty() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "empty GEOM and PART names" ) );

    if( aPlacement == PS_INVALID )
    {
        ERROR_IDF << "placement invalid (" << aRefDes << ":";
        aPlacement = PS_PLACED;
    }

    if( aSide != LYR_TOP && aSide != LYR_BOTTOM )
    {
        ostringstream ostr;
        ostr << "\n* invalid side (" << GetLayerString( aSide ) << "); ";
        ostr << "must be TOP or BOTTOM\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    // calculate the final position based on layer
    double xpos, ypos, ang;

    switch( aSide )
    {
        case LYR_TOP:
            xpos = aXpos + xoff;
            ypos = aYpos + yoff;
            ang  = aAngle + aoff;
            break;

        default:
            xpos = aXpos - xoff;
            ypos = aYpos + yoff;
            ang  = aAngle - aoff;
            break;
    }

    std::string arefdes = aRefDes;

    if( arefdes.empty() || !arefdes.compare( "~" )
        || ( arefdes.size() >= 8 && CompareToken( "NOREFDES", arefdes.substr(0, 8) ) ) )
        arefdes = "NOREFDES";

    aBoardFile << "\"" << outline->GetGeomName() << "\" \"" << outline->GetPartName() << "\" "
    << arefdes << "\n";

    IDF3::IDF_UNIT unit = UNIT_MM;

    if( parent )
        unit = parent->GetUnit();

    if( unit == UNIT_MM )
    {
        aBoardFile << setiosflags(ios::fixed) << setprecision(5) << xpos << " "
        << ypos << " " << setprecision(3) << zoff << " "
        << ang << " ";
    }
    else
    {
        aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (xpos / IDF_THOU_TO_MM) << " "
        << (ypos / IDF_THOU_TO_MM) << " "  << (zoff / IDF_THOU_TO_MM) << " "
        << setprecision(3) << ang << " ";
    }

    WriteLayersText( aBoardFile, aSide );

    switch( aPlacement )
    {
        case PS_PLACED:
            aBoardFile << " PLACED\n";
            break;

        case PS_UNPLACED:
            aBoardFile << " UNPLACED\n";
            break;

        case PS_MCAD:
            aBoardFile << " MCAD\n";
            break;

        default:
            aBoardFile << " ECAD\n";
            break;
    }
}


IDF3_COMPONENT::IDF3_COMPONENT( IDF3_BOARD* aParent )
{
    xpos   = 0.0;
    ypos   = 0.0;
    angle  = 0.0;

    hasPosition = false;
    placement   = PS_INVALID;
    layer       = LYR_INVALID;

    parent = aParent;
}


IDF3_COMPONENT::~IDF3_COMPONENT()
{
    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itcS = components.begin();
    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itcE = components.end();

    while( itcS != itcE )
    {
        delete *itcS;
        ++itcS;
    }

    components.clear();

    std::list< IDF_DRILL_DATA* >::iterator itdS = drills.begin();
    std::list< IDF_DRILL_DATA* >::iterator itdE = drills.end();

    while( itdS != itdE )
    {
        delete *itdS;
        ++itdS;
    }

    drills.clear();
}


#ifndef DISABLE_IDF_OWNERSHIP
bool IDF3_COMPONENT::checkOwnership( int aSourceLine, const char* aSourceFunc )
{
    if( !parent )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc << "():\n";
        ostr << "\n* BUG: parent not set";
        errormsg = ostr.str();

        return false;
    }

    IDF3::CAD_TYPE pcad = parent->GetCadType();

    switch( placement )
    {
        case PS_UNPLACED:
        case PS_PLACED:
        case PS_INVALID:
            break;

        case PS_MCAD:

            if( pcad != CAD_MECH )
            {
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "\n* ownership violation; internal CAD type (MCAD) conflicts with "
                        "PLACEMENT (";
                ostr << GetPlacementString( placement ) << ")";
                errormsg = ostr.str();

                return false;
            }

            break;

        case PS_ECAD:

            if( pcad != CAD_ELEC )
            {
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "\n* ownership violation; internal CAD type (MCAD) conflicts with "
                        "PLACEMENT (";
                ostr << GetPlacementString( placement ) << ")";
                errormsg = ostr.str();

                return false;
            }
            break;

        default:
            do
            {
                ostringstream ostr;
                ostr << "\n* BUG: unhandled internal placement value (" << placement << ")";
                errormsg = ostr.str();

                return false;
            } while( 0 );

            break;
    }

    return true;
}
#endif


void IDF3_COMPONENT::SetParent( IDF3_BOARD* aParent )
{
    parent = aParent;
    return;
}

IDF3::CAD_TYPE IDF3_COMPONENT::GetCadType( void )
{
    if( parent )
        return parent->GetCadType();

    return CAD_INVALID;
}

IDF3::IDF_UNIT IDF3_COMPONENT::GetUnit( void )
{
    if( parent )
        return parent->GetUnit();

    return UNIT_INVALID;
}

bool IDF3_COMPONENT::SetRefDes( const std::string& aRefDes )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    if( aRefDes.empty() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): invalid RefDes (empty)";
        errormsg = ostr.str();

        return false;
    }

    if( CompareToken( "PANEL", aRefDes ) )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr <<  "* BUG: PANEL is a reserved designator and may not be used by components";
        errormsg = ostr.str();

        return false;
    }

    refdes = aRefDes;
    return true;
}


const std::string& IDF3_COMPONENT::GetRefDes( void )
{
    return refdes;
}


IDF_DRILL_DATA* IDF3_COMPONENT::AddDrill( double aDia, double aXpos, double aYpos,
                                          IDF3::KEY_PLATING aPlating,
                                          const std::string& aHoleType,
                                          IDF3::KEY_OWNER aOwner )
{
    IDF_DRILL_DATA* dp = nullptr;

    try
    {
        dp = new IDF_DRILL_DATA( aDia, aXpos, aYpos, aPlating, refdes, aHoleType, aOwner );
    }
    catch( std::bad_alloc& )
    {
        return nullptr;
    }

    drills.push_back( dp );

    return dp;
}


IDF_DRILL_DATA* IDF3_COMPONENT::AddDrill( IDF_DRILL_DATA* aDrilledHole )
{
    if( !aDrilledHole )
        return nullptr;

    if( CompareToken( "PANEL", refdes ) )
    {
        ERROR_IDF;
        cerr << "\n* BUG: PANEL drills not supported at component level\n";
        return nullptr;
    }

    if( refdes.compare( aDrilledHole->GetDrillRefDes() ) )
    {
        ERROR_IDF;
        cerr << "\n* BUG: pushing an incorrect REFDES ('" << aDrilledHole->GetDrillRefDes();
        cerr << "') to component ('" << refdes << "')\n";
        return nullptr;
    }

    drills.push_back( aDrilledHole );

    return aDrilledHole;
}


bool IDF3_COMPONENT::DelDrill( double aDia, double aXpos, double aYpos )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    errormsg.clear();

    if( drills.empty() )
        return false;

    bool val = false;

    list< IDF_DRILL_DATA* >::iterator itS = drills.begin();
    list< IDF_DRILL_DATA* >::iterator itE = drills.end();

    while( !drills.empty() && itS != itE )
    {
        if( (*itS)->Matches( aDia, aXpos, aYpos ) )
        {
            val = true;
            delete *itS;
            itS = drills.erase( itS );
            continue;
        }

        ++itS;
    }

    return val;
}


bool IDF3_COMPONENT::DelDrill( IDF_DRILL_DATA* aDrill )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    errormsg.clear();

    if( drills.empty() )
        return false;

    list< IDF_DRILL_DATA* >::iterator itS = drills.begin();
    list< IDF_DRILL_DATA* >::iterator itE = drills.end();

    while( !drills.empty() && itS != itE )
    {
        if( *itS == aDrill )
        {
            delete *itS;
            drills.erase( itS );
            return true;
        }

        ++itS;
    }

    return false;
}


const std::list<IDF_DRILL_DATA*>* IDF3_COMPONENT::GetDrills( void )
{
    return &drills;
}

bool IDF3_COMPONENT::AddOutlineData( IDF3_COMP_OUTLINE_DATA* aComponentOutline )
{
    if( aComponentOutline == nullptr )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ <<
                "(): invalid aComponentOutline (nullptr)";
        errormsg = ostr.str();

        return false;
    }


    components.push_back( aComponentOutline );

    return true;
}


bool IDF3_COMPONENT::DeleteOutlineData( IDF3_COMP_OUTLINE_DATA* aComponentOutline )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    if( components.empty() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): component list is empty";
        errormsg = ostr.str();

        return false;
    }

    if( aComponentOutline == nullptr )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ <<
                "(): invalid aComponentOutline (nullptr)";
        errormsg = ostr.str();

        return false;
    }

    errormsg.clear();

    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itS = components.begin();
    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itE = components.end();

    while( itS != itE )
    {
        if( *itS == aComponentOutline )
        {
            delete *itS;
            components.erase( itS );
            return true;
        }

        ++itS;
    }

    return false;
}


bool IDF3_COMPONENT::DeleteOutlineData( size_t aIndex )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    if( aIndex >= components.size() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* aIndex (" << aIndex << ") out of range; list size is " << components.size();
        errormsg = ostr.str();

        return false;
    }

    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itS = components.begin();
    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itE = components.end();
    size_t idx = 0;

    while( itS != itE )
    {
        if( idx == aIndex )
        {
            delete *itS;
            components.erase( itS );
            return true;
        }

        ++idx;
        ++itS;
    }

    return false;
}


size_t IDF3_COMPONENT::GetOutlinesSize( void )
{
    return components.size();
}


const std::list< IDF3_COMP_OUTLINE_DATA* >* IDF3_COMPONENT::GetOutlinesData( void )
{
    return &components;
}


bool IDF3_COMPONENT::GetPosition( double& aXpos, double& aYpos, double& aAngle,
                                  IDF3::IDF_LAYER& aLayer )
{
    errormsg.clear();

    if( !hasPosition )
    {
        aXpos = 0.0;
        aYpos = 0.0;
        aAngle = 0.0;
        aLayer = IDF3::LYR_INVALID;
        return false;
    }

    aXpos = xpos;
    aYpos = ypos;
    aAngle = angle;
    aLayer = layer;
    return true;
}


bool IDF3_COMPONENT::SetPosition( double aXpos, double aYpos, double aAngle,
                                  IDF3::IDF_LAYER aLayer )
{
#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    errormsg.clear();

    switch( aLayer )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
            break;

        default:
            do
            {
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "\n* invalid side (must be TOP or BOTTOM only): " <<
                        GetLayerString( aLayer );
                errormsg = ostr.str();

                return false;
            } while( 0 );

            break;
    }

    if( hasPosition )
        return false;

    hasPosition = true;
    xpos = aXpos;
    ypos = aYpos;
    angle = aAngle;
    layer = aLayer;
    return true;
}


IDF3::IDF_PLACEMENT IDF3_COMPONENT::GetPlacement( void )
{
    return placement;
}


bool IDF3_COMPONENT::SetPlacement( IDF3::IDF_PLACEMENT aPlacementValue )
{
    if( aPlacementValue < PS_UNPLACED || aPlacementValue >= PS_INVALID )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "\n* invalid PLACEMENT value (" << aPlacementValue << ")";
        errormsg = ostr.str();

        return false;
    }

#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkOwnership( __LINE__, __FUNCTION__ ) )
        return false;
#endif

    placement = aPlacementValue;

    return true;
}


bool IDF3_COMPONENT::writeDrillData( std::ostream& aBoardFile )
{
    if( drills.empty() )
        return true;

    std::list< IDF_DRILL_DATA* >::iterator itS = drills.begin();
    std::list< IDF_DRILL_DATA* >::iterator itE = drills.end();

    while( itS != itE )
    {
        (*itS)->write( aBoardFile, GetUnit() );
        ++itS;
    }

    return true;
}


bool IDF3_COMPONENT::writePlaceData( std::ostream& aBoardFile )
{
    if( components.empty() )
        return true;

    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itS = components.begin();
    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itE = components.end();

    while( itS != itE )
    {
        (*itS)->writePlaceData( aBoardFile, xpos, ypos, angle, refdes, placement, layer );
        ++itS;
    }

    return true;
}


IDF3_BOARD::IDF3_BOARD( IDF3::CAD_TYPE aCadType )
{
    m_idfVer         = IDF_V3;
    m_cadType        = aCadType;
    m_userPrec       = 5;
    m_userScale      = 1.0;
    m_userXoff       = 0.0;
    m_userYoff       = 0.0;
    m_brdFileVersion = 0;
    m_libFileVersion = 0;
    m_refDesCounter  = 0;
    m_unit           = UNIT_MM;

    // unlike other outlines which are created as necessary,
    // the board outline always exists and its parent must
    // be set here
    m_boardOutline.setParent( this );
    m_boardOutline.setThickness( 1.6 );

    return;
}


IDF3_BOARD::~IDF3_BOARD()
{
    Clear();

    return;
}


const std::string& IDF3_BOARD::GetNewRefDes( void )
{
    ostringstream ostr;
    ostr << "NOREFDESn" << m_refDesCounter++;

    m_refDesString = ostr.str();

    return m_refDesString;
}


#ifndef DISABLE_IDF_OWNERSHIP
bool IDF3_BOARD::checkComponentOwnership( int aSourceLine, const char* aSourceFunc,
                                          IDF3_COMPONENT* aComponent )
{
    if( !aComponent )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc;
        ostr << "(): Invalid component pointer (nullptr)";
        m_errormsg = ostr.str();

        return false;
    }

    IDF3::IDF_PLACEMENT place = aComponent->GetPlacement();

    if( place == PS_PLACED || place == PS_UNPLACED )
        return true;

    if( place == PS_MCAD && m_cadType == CAD_MECH )
        return true;

    if( place == PS_ECAD && m_cadType == CAD_ELEC )
        return true;

    do
    {
        ostringstream ostr;
        ostr << "* " << __FILE__ << ":" << aSourceLine << ":" << aSourceFunc << "():\n";
        ostr << "* ownership violation; CAD type is ";

        if( m_cadType == CAD_MECH )
            ostr << "MCAD ";
        else
            ostr << "ECAD ";

        ostr << "while outline owner is " << GetPlacementString( place ) << "\n";
        m_errormsg = ostr.str();

    } while( 0 );

    return false;
}
#endif


bool IDF3_BOARD::setUnit( IDF3::IDF_UNIT aUnit, bool convert )
{
    switch( aUnit )
    {
    case UNIT_MM:
    case UNIT_THOU:
        m_unit = aUnit;
        break;

    case UNIT_TNM:
        ERROR_IDF << "\n* TNM unit is not supported; defaulting to mm\n";
        m_unit = UNIT_MM;
        break;

    default:
        do
        {
            ostringstream ostr;
            ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
            ostr << "* invalid board unit (" << aUnit << ")";
            m_errormsg = ostr.str();

            return false;
        } while( 0 );

        break;
    }

    // iterate through all owned OUTLINE objects (except IDF3_COMP_OUTLINE)
    // and set to the same unit

    m_boardOutline.SetUnit( aUnit );

    do
    {
        std::map<std::string, OTHER_OUTLINE*>::iterator its = m_otherOutlines.begin();
        std::map<std::string, OTHER_OUTLINE*>::iterator ite = m_otherOutlines.end();

        while( its != ite )
        {
            its->second->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<ROUTE_OUTLINE*>::iterator its = m_routeOutlines.begin();
        std::list<ROUTE_OUTLINE*>::iterator ite = m_routeOutlines.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<PLACE_OUTLINE*>::iterator its = m_placeOutlines.begin();
        std::list<PLACE_OUTLINE*>::iterator ite = m_placeOutlines.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<ROUTE_KO_OUTLINE*>::iterator its = m_routeKeepoutOutlines.begin();
        std::list<ROUTE_KO_OUTLINE*>::iterator ite = m_routeKeepoutOutlines.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<VIA_KO_OUTLINE*>::iterator its = m_viaKeepoutOutlines.begin();
        std::list<VIA_KO_OUTLINE*>::iterator ite = m_viaKeepoutOutlines.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<PLACE_KO_OUTLINE*>::iterator its = m_placeKeepoutOutlines.begin();
        std::list<PLACE_KO_OUTLINE*>::iterator ite = m_placeKeepoutOutlines.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::multimap<std::string, GROUP_OUTLINE*>::iterator its = m_groupOutlines.begin();
        std::multimap<std::string, GROUP_OUTLINE*>::iterator ite = m_groupOutlines.end();

        while( its != ite )
        {
            its->second->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    //iterate through all owned IDF3_COMP_OUTLINE objects and
    // set to the same unit IF convert = true
    if( convert )
    {
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator its = m_componentOutlines.begin();
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator ite = m_componentOutlines.end();

        while( its != ite )
        {
            its->second->SetUnit( aUnit );
            ++its;
        }

    }

    return true;
}


bool IDF3_BOARD::SetBoardThickness( double aBoardThickness )
{
    if( aBoardThickness <= 0.0 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): ";
        ostr << "board thickness (" << aBoardThickness << ") must be > 0";
        m_errormsg = ostr.str();

        return false;
    }

    if( !m_boardOutline.SetThickness( aBoardThickness ) )
    {
        m_errormsg = m_boardOutline.GetError();
        return false;
    }

    return true;
}


double IDF3_BOARD::GetBoardThickness( void )
{
    return m_boardOutline.GetThickness();
}


void IDF3_BOARD::readBrdDrills( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState )
{
    IDF_DRILL_DATA drill;

    while( drill.read( aBoardFile, m_unit, aBoardState, m_idfVer ) )
    {
        IDF_DRILL_DATA *dp = new IDF_DRILL_DATA;
        *dp = drill;

        if( AddDrill( dp ) == nullptr )
        {
            delete dp;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "\n* BUG: could not add drill data; cannot continue reading the file" ) );
        }
    }
}


void IDF3_BOARD::readBrdNotes( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState )
{
    IDF_NOTE note;

    while( note.readNote( aBoardFile, aBoardState, m_unit ) )
    {
        IDF_NOTE *np = new IDF_NOTE;
        *np = note;
        m_notes.push_back( np );
    }
}


void IDF3_BOARD::readBrdPlacement( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState,
                                   bool aNoSubstituteOutlines )
{
    IDF3_COMP_OUTLINE_DATA oldata;

    while( oldata.readPlaceData( aBoardFile, aBoardState, this, m_idfVer, aNoSubstituteOutlines ) );
}


void IDF3_BOARD::readBrdHeader( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState )
{
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;

    // RECORD 1: ".HEADER" must be the very first line
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading board header" ) );

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: first line must be .HEADER\n" ) );
    }

    if( !CompareToken( ".HEADER", iline ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* first line must be .HEADER and have no quotes or trailing text" ) );
    }

    // RECORD 2:
    //      File Type [str]: BOARD_FILE (PANEL_FILE not supported)
    //      IDF Version Number [float]: must be 3.0
    //      Source System [str]: ignored
    //      Date [str]: ignored
    //      Board File Version [int]: ignored
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading board header, RECORD 2" ) );

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: comment within .HEADER section" ) );
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* File Type in HEADER section must not be in quotes" ) );
    }

    if( !CompareToken( "BOARD_FILE", token ) )
    {
        ERROR_IDF;

        if( CompareToken( "PANEL_FILE", token ) )
        {
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "not a board file\n"
                              "* PANEL_FILE is not supported (expecting BOARD_FILE)" ) );
        }
        else
        {
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "invalid IDF file\n"
                              "* Expecting string: BOARD_FILE" ) );
        }
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: HEADER section, RECORD 2: no FIELD 2" ) );
    }

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: IDF Version must not be in quotes" ) );
    }

    if( !token.compare( "3.0" ) || !token.compare( "3." ) || !token.compare( "3" ) )
    {
        m_idfVer = IDF_V3;
    }
    else if( !token.compare( "2.0" ) || !token.compare( "2." ) || !token.compare( "2" ) )
    {
        m_idfVer = IDF_V2;
    }
    else
    {
        ostringstream ostr;

        ostr << "unsupported IDF version\n";
        ostr << "* Expecting version to be a variant of '3.0', '2.0' (value: '" << token << "')\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 3: no Source System string" ) );
    }

    m_brdSource = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 4: no Date string" ) );
    }

    m_brdDate = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 5: no Board File Version number" ) );
    }

    std::istringstream istr;
    istr.str( token );

    istr >> m_brdFileVersion;

    if( istr.fail() )
    {
        ERROR_IDF << "invalid Board File Version in header\n";
        cerr << "* Setting default version of 1\n";
        m_brdFileVersion = 1;
    }

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 5: Board File Version must not be in quotes" ) );
    }

    // RECORD 3:
    //      Board Name [str]: stored
    //      Units [str]: MM or THOU
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "\n* problems reading board header, RECORD 2" ) );

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: comment within .HEADER section" ) );
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    m_boardName = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 3, FIELD 1: no Board Name" ) );
    }

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 3, FIELD 2: UNIT may not be in quotes" ) );
    }

    if( CompareToken( "MM", token ) )
    {
        m_unit = IDF3::UNIT_MM;
    }
    else if( CompareToken( "THOU", token ) )
    {
        m_unit = IDF3::UNIT_THOU;
    }
    else if( ( m_idfVer == IDF_V2 ) && CompareToken( "TNM", token ) )
    {
        m_unit = IDF3::UNIT_TNM;
    }
    else
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* HEADER section, RECORD 3, FIELD 2: expecting MM or THOU (got '" << token <<
                "')\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    m_boardOutline.SetUnit( m_unit );

    // RECORD 4:
    //      .END_HEADER
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading board header, RECORD 4" ) );

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF file\n"
                          "* Violation of specification: comment within .HEADER section\n" ) );
    }

    if( !CompareToken( ".END_HEADER", iline ) )
    {
        ostringstream ostr;

        ostr << "invalid IDF file\n";
        ostr << "* Violation of specification: expected .END_HEADER\n";
        ostr << "* line: '" << iline << "'";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    aBoardState = IDF3::FILE_HEADER;
}


void IDF3_BOARD::readBrdSection( std::istream& aBoardFile, IDF3::FILE_STATE& aBoardState,
                                 bool aNoSubstituteOutlines )
{
    std::list< std::string > comments;  // comments associated with a section

    // Reads in .SECTION_ID or #COMMENTS
    // Expected SECTION IDs:
    //      .BOARD_OUTLINE
    //      .PANEL_OUTLINE (NOT SUPPORTED)
    //      .OTHER_OUTLINE
    //      .ROUTE_OUTLINE
    //      .PLACE_OUTLINE
    //      .ROUTE_KEEPOUT
    //      .VIA_KEEPOUT
    //      .PLACE_KEEPOUT
    //      .PLACE_REGION
    //      .DRILLED_HOLES
    //      .NOTES
    //      .PLACEMENT
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;

    while( aBoardFile.good() )
    {
        while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

        if( !aBoardFile.good() )
        {
            if( aBoardFile.eof() && aBoardState >= IDF3::FILE_HEADER &&
                aBoardState < IDF3::FILE_INVALID )
            {
                if( !comments.empty() )
                    ERROR_IDF << "[warning]: trailing comments in IDF file (comments will be lost)\n";

                return;
            }

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                              "problems reading board section" ) );
        }

        if( isComment )
        {
            comments.push_back( iline );
            continue;
        }

        // This must be a header
        GetIDFString( iline, token, quoted, idx );

        if( quoted )
        {
            ostringstream ostr;

            ostr << "invalid IDF file\n";
            ostr << "* Violation of specification: quoted string where SECTION HEADER expected\n";
            ostr << "* line: '" << iline << "'";
            ostr << "* position: " << pos;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( CompareToken( ".BOARD_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_HEADER )
            {
                aBoardState = IDF3::FILE_INVALID;
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: no HEADER section" ) );
            }

            m_boardOutline.readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    m_boardOutline.AddComment( *its );
                    ++its;
                }
            }

            aBoardState = IDF3::FILE_OUTLINE;
            return;
        }

        if( CompareToken( ".PANEL_OUTLINE", token ) )
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "PANEL_OUTLINE not supported" ) );

        if( CompareToken( ".OTHER_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .OTHER_OUTLINE" ) );

            OTHER_OUTLINE* op = nullptr;

            try
            {
                op = new OTHER_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "could not create OTHER_OUTLINE object" ) );
            }

            if( op == nullptr )

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            if( m_otherOutlines.emplace( op->GetOutlineIdentifier(), op ).second == false )
            {
                ostringstream ostr;
                ostr << "invalid IDF file\n";
                ostr << "* Violation of specification. Non-unique ID in OTHER_OUTLINE '";
                ostr << op->GetOutlineIdentifier() << "'\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* pos: " << pos;
                delete op;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            return;
        }

        if( CompareToken( ".ROUTE_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .ROUTE_OUTLINE" ) );
            }

            ROUTE_OUTLINE* op = nullptr;

            try
            {
                op = new ROUTE_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "could not create ROUTE_OUTLINE object" ) );
            }

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            m_routeOutlines.push_back( op );

            return;
        }

        if( CompareToken( ".PLACE_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACE_OUTLINE" ) );
            }

            PLACE_OUTLINE* op = nullptr;

            try
            {
                op = new PLACE_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "could not create PLACE_OUTLINE object" ) );
            }

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            m_placeOutlines.push_back( op );

            return;
        }

        if( CompareToken( ".ROUTE_KEEPOUT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .ROUTE_KEEPOUT" ) );
            }

            ROUTE_KO_OUTLINE* op = nullptr;

            try
            {
                op = new ROUTE_KO_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "could not create ROUTE_KEEPOUT object" ) );
            }

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            m_routeKeepoutOutlines.push_back( op );

            return;
        }

        if( CompareToken( ".VIA_KEEPOUT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .VIA_KEEPOUT" ) );
            }

            VIA_KO_OUTLINE* op = nullptr;

            try
            {
                op = new VIA_KO_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "could not create VIA_KEEPOUT object" ) );
            }

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            m_viaKeepoutOutlines.push_back( op );

            return;
        }

        if( CompareToken( ".PLACE_KEEPOUT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACE_KEEPOUT" ) );
            }

            PLACE_KO_OUTLINE* op = nullptr;
            try
            {
                op = new PLACE_KO_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "could not create PLACE_KEEPOUT object" ) );
            }

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            m_placeKeepoutOutlines.push_back( op );

            return;
        }

        if( CompareToken( ".PLACE_REGION", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACE_REGION" ) );
            }

            GROUP_OUTLINE* op = nullptr;

            try
            {
                op = new GROUP_OUTLINE( this );
            }
            catch( std::bad_alloc& )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "could not create PLACE_REGION object" ) );
            }

            op->SetUnit( m_unit );
            op->readData( aBoardFile, iline, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    op->AddComment( *its );
                    ++its;
                }
            }

            m_groupOutlines.emplace( op->GetGroupName(), op );

            return;
        }

        if( CompareToken( ".DRILLED_HOLES", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .DRILLED_HOLES" ) );
            }

            readBrdDrills( aBoardFile, aBoardState );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    m_drillComments.push_back( *its );
                    ++its;
                }
            }

            return;
        }

        if( CompareToken( ".NOTES", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .NOTES" ) );
            }

            if( m_idfVer < IDF_V3 )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDFv2 file\n"
                                  "* Violation of specification: NOTES section not in specification" ) );
            }

            readBrdNotes( aBoardFile, aBoardState );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    m_noteComments.push_back( *its );
                    ++its;
                }
            }

            return;
        }

        if( CompareToken( ".PLACEMENT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                  "invalid IDF file\n"
                                  "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACEMENT" ) );
            }

            readBrdPlacement( aBoardFile, aBoardState, aNoSubstituteOutlines );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    m_placeComments.push_back( *its );
                    ++its;
                }
            }

            return;
        }
    }
}


void IDF3_BOARD::readBoardFile( const std::string& aFileName, bool aNoSubstituteOutlines )
{
    OPEN_ISTREAM( brd, aFileName.c_str() );

    try
    {
        brd.exceptions ( std::ios_base::badbit );

        if( brd.fail() )
        {
            ostringstream ostr;
            ostr << "\n* could not open file: '" << aFileName << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        brd.imbue( std::locale( "C" ) );
        std::string iline;      // the input line
        bool isComment;         // true if a line just read in is a comment line
        std::streampos pos;
        IDF3::FILE_STATE state = IDF3::FILE_START;

        // note: as per IDFv3 specification:
        //      "The Header section must be the first section in the file, the second
        //       section must be the Outline section, and the last section must be the
        //       Placement section. All other sections may be in any order."

        // further notes: Except for the HEADER section, sections may be preceded by
        // comment lines which will be copied back out on write(). No comments may
        // be associated with the board file itself since the only logical location
        // for unambiguous association is at the end of the file, which is inconvenient
        // for large files.

        readBrdHeader( brd, state );

        // read the various sections
        while( state != IDF3::FILE_PLACEMENT && brd.good() )
            readBrdSection( brd, state, aNoSubstituteOutlines );

        if( !brd.good() )
        {
            // check if we have valid data
            if( brd.eof() && state >= IDF3::FILE_OUTLINE && state < IDF3::FILE_INVALID )
            {
                CLOSE_STREAM( brd );
                return;
            }

            CLOSE_STREAM( brd );
            ostringstream ostr;
            ostr << "\n* empty IDF file: '" << aFileName << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( brd.good() && state == IDF3::FILE_PLACEMENT )
        {
            // read in any trailing lines and report on ignored comments (minor fault)
            // and any non-comment item (non-compliance with IDFv3)
            while( brd.good() )
            {
                while( !FetchIDFLine( brd, iline, isComment, pos ) && brd.good() );

                // normally this is a fault but we have all the data in accordance with specs
                if( ( !brd.good() && !brd.eof() ) || iline.empty() )
                    break;

                if( isComment )
                {
                    ERROR_IDF << "[warning]: trailing comments after PLACEMENT\n";
                }
                else
                {
                    ostringstream ostr;
                    ostr << "\n* problems reading file: '" << aFileName << "'";

                    throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                                      "invalid IDF file\n"
                                      "* Violation of specification: non-comment lines after PLACEMENT section" ) );
                }
            }
        }
    }
    catch( const std::exception& )
    {
        brd.exceptions ( std::ios_base::goodbit );
        CLOSE_STREAM( brd );
        throw;
    }

    CLOSE_STREAM( brd );
}


void IDF3_BOARD::readLibSection( std::istream& aLibFile, IDF3::FILE_STATE& aLibState,
                                 IDF3_BOARD* aBoard )
{
    if( aBoard == nullptr )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "\n* BUG: invoked with nullptr reference aBoard" ) );
    }

    std::list< std::string > comments;  // comments associated with a section

    // Reads in .ELECTRICAL, .MECHANICAL or #COMMENTS
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;
    IDF3_COMP_OUTLINE* pout = nullptr;

    try
    {
        pout = new IDF3_COMP_OUTLINE( this );
    }
    catch( std::bad_alloc& )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "\n* memory allocation failure" ) );
    }

    while( aLibFile.good() )
    {
        while( !FetchIDFLine( aLibFile, iline, isComment, pos ) && aLibFile.good() );

        if( !aLibFile.good() && !aLibFile.eof() )
        {
            delete pout;
            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading library section" ) );
        }

        // no data was read; this only happens at eof()
        if( iline.empty() )
        {
            delete pout;
            return;
        }

        if( isComment )
        {
            comments.push_back( iline );
            continue;
        }

        // This must be a header
        GetIDFString( iline, token, quoted, idx );

        if( quoted )
        {
            ostringstream ostr;
            ostr << "invalid IDF library\n";
            ostr << "* Violation of specification: quoted string where .ELECTRICAL or .MECHANICAL expected\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* pos: " << pos;
            delete pout;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( CompareToken( ".ELECTRICAL", token ) || CompareToken( ".MECHANICAL", token ) )
        {
            pout->readData( aLibFile, token, m_idfVer );

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    pout->AddComment( *its );
                    ++its;
                }
            }

            IDF3_COMP_OUTLINE* cop = aBoard->GetComponentOutline( pout->GetUID() );

            if( cop == nullptr )
            {
                m_componentOutlines.emplace( pout->GetUID(), pout );
            }
            else
            {
                if( MatchCompOutline( pout, cop ) )
                {
                    delete pout;
                    // everything is fine; the outlines are genuine duplicates
                    return;
                }

                ostringstream ostr;
                ostr << "invalid IDF library\n";
                ostr << "duplicate Component Outline: '" << pout->GetUID() << "'\n";
                ostr << "* Violation of specification: multiple outlines have the same GEOM and PART name\n";
                ostr << "* line: '" << iline << "'\n";
                ostr << "* pos: " << pos;
                delete pout;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            return;
        }
        else
        {
            ostringstream ostr;
            ostr << "invalid IDF library\n";
            ostr << "* Expecting .ELECTRICAL or .MECHANICAL, got '" << token << "'\n";
            ostr << "* line: '" << iline << "'\n";
            ostr << "* pos: " << pos;
            delete pout;

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
    }

    delete pout;

    if( !aLibFile.eof() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading IDF library file" ) );
}


// read the library HEADER
void IDF3_BOARD::readLibHeader( std::istream& aLibFile, IDF3::FILE_STATE& aLibState )
{
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;

    // RECORD 1: ".HEADER" must be the very first line
    while( !FetchIDFLine( aLibFile, iline, isComment, pos ) && aLibFile.good() );

    if( !aLibFile.good() )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* premature end of file (no HEADER)" ) );
    }

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification: first line must be .HEADER" ) );
    }

    if( !CompareToken( ".HEADER", iline ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification:\n"
                          "* first line must be .HEADER and have no quotes or trailing text" ) );
    }

    // RECORD 2:
    //      File Type [str]: LIBRARY_FILE
    //      IDF Version Number [float]: must be 3.0
    //      Source System [str]: ignored
    //      Date [str]: ignored
    //      Library File Version [int]: ignored
    while( !FetchIDFLine( aLibFile, iline, isComment, pos ) && aLibFile.good() );

    if( !aLibFile.good() )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* premature end of HEADER" ) );
    }

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification: comment within .HEADER section" ) );
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification:\n"
                          "* file Type in HEADER section must not be in quotes" ) );
    }

    if( !CompareToken( "LIBRARY_FILE", token ) )
    {
        ostringstream ostr;
        ostr << "invalid IDF library\n";
        ostr << "* Expecting string: LIBRARY_FILE (got '" << token << "')\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification: HEADER section, RECORD 2: no FIELD 2" ) );
    }

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification: IDF Version must not be in quotes" ) );
    }

    if( !token.compare( "3.0" ) || !token.compare( "3." ) || !token.compare( "3" ) )
    {
        m_idfVer = IDF_V3;
    }
    else if( !token.compare( "2.0" ) || !token.compare( "2." ) || !token.compare( "2" ) )
    {
        m_idfVer = IDF_V2;
    }
    else
    {
        ostringstream ostr;

        ostr << "unsupported IDF version\n";
        ostr << "* Expecting version to be a variant of '3.0', '2.0' (value: '" << token << "')\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 3: no Source System string" ) );
    }

    m_libSource = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 4: no Date string" ) );
    }

    m_libDate = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 5: no Board File Version number" ) );
    }

    std::istringstream istr;
    istr.str( token );

    istr >> m_libFileVersion;

    if( istr.fail() )
    {
        ERROR_IDF << "invalid Library File Version in header\n";
        cerr << "* Setting default version of 1\n";
        m_libFileVersion = 1;
    }

    if( quoted )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification:\n"
                          "* HEADER section, RECORD 2, FIELD 5: Library File Version must not be in quotes" ) );
    }

    // RECORD 3:
    //      .END_HEADER
    while( !FetchIDFLine( aLibFile, iline, isComment, pos ) && aLibFile.good() );

    if( ( !aLibFile.good() && !aLibFile.eof() ) || iline.empty() )
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, "problems reading library header, RECORD 3" ) );

    if( isComment )
    {
        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__,
                          "invalid IDF library file\n"
                          "* Violation of specification: comment within .HEADER section" ) );
    }

    if( !CompareToken( ".END_HEADER", iline ) )
    {
        ostringstream ostr;
        ostr << "invalid IDF header\n";
        ostr << "* Violation of specification: expected .END_HEADER (got '" << iline << "')\n";

        throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
    }

    aLibState = IDF3::FILE_HEADER;
}


void IDF3_BOARD::readLibFile( const std::string& aFileName )
{
    OPEN_ISTREAM( lib, aFileName.c_str() );

    try
    {
        lib.exceptions ( std::ios_base::badbit );

        if( lib.fail() )
        {
            ostringstream ostr;
            ostr << "\n* could not open file: '" << aFileName << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        lib.imbue( std::locale( "C" ) );
        IDF3::FILE_STATE state = IDF3::FILE_START;

        readLibHeader( lib, state );

        while( lib.good() ) readLibSection( lib, state, this );
    }
    catch( const std::exception& )
    {
        lib.exceptions ( std::ios_base::goodbit );
        CLOSE_STREAM( lib );
        throw;
    }

    CLOSE_STREAM( lib );
}


bool IDF3_BOARD::ReadFile( const wxString& aFullFileName, bool aNoSubstituteOutlines )
{
    // 1. Check that the file extension is 'emn'
    // 2. Check if a file with extension 'emp' exists and read it
    // 3. Open the specified filename and read it

    wxFileName brdname( aFullFileName );
    wxFileName libname( aFullFileName );
    wxString ext = brdname.GetExt();

    if( !ext.Cmp( wxT( "EMN" ) ) )
    {
        libname.SetExt( wxT( "EMP" ) );
    }
    else if( !ext.Cmp( wxT( "emn" ) ) )
    {
        libname.SetExt( wxT( "emp" ) );
    }
    else
    {
        ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << "* [INFO] invalid file name: '" << aFullFileName.ToUTF8() << "'";

        m_errormsg = ostr.str();
    }


    brdname.SetExt( wxT( "emn" ) );

    std::string bfname = TO_UTF8( aFullFileName );

    if( !wxFileExists( bfname ) )
    {
        brdname.SetExt( wxT( "EMN" ) );
        libname.SetExt( wxT( "EMP" ) );
    }

    try
    {
        if( !brdname.IsOk() )
        {
            ostringstream ostr;
            ostr << "\n* invalid file name: '" << aFullFileName.ToUTF8() << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !brdname.FileExists() )
        {
            ostringstream ostr;
            ostr << "\n* no such file: '" << aFullFileName.ToUTF8() << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( !brdname.IsFileReadable() )
        {
            ostringstream ostr;
            ostr << "\n* cannot read file: '" << bfname << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        bfname = TO_UTF8( brdname.GetFullPath() );
        std::string lfname = TO_UTF8( libname.GetFullPath() );

        if( !libname.FileExists() )
        {
            // NOTE: Since this is a common case we simply proceed
            // with the assumption that there is no library file;
            // however we print a message to inform the user.
            ERROR_IDF;
            cerr << "no associated library file (*.emp)\n";
        }
        else if( !libname.IsFileReadable() )
        {
            ostringstream ostr;
            ostr << "\n* cannot read library file: '" << lfname << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }
        else
        {
            // read the library file before proceeding
            readLibFile( lfname );
        }

        // read the board file
        readBoardFile( bfname, aNoSubstituteOutlines );
    }
    catch( const std::exception& e )
    {
        Clear();
        m_errormsg = e.what();

        return false;
    }

    return true;
}


bool IDF3_BOARD::writeLibFile( const std::string& aFileName )
{
    OPEN_OSTREAM( lib, aFileName.c_str() );

    try
    {
        lib.exceptions( std::ios_base::failbit );

        if( lib.fail() )
        {
            ostringstream ostr;
            ostr << "\n* could not open file: '" << aFileName << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        lib.imbue( std::locale( "C" ) );
        wxDateTime tdate( time( nullptr ) );

        if( m_idfSource.empty() )
            m_idfSource = "KiCad-IDF Framework";

        ostringstream fileDate;
        fileDate << setfill( '0' ) << setw(4) << tdate.GetYear();
        fileDate << "/" << setw(2) << tdate.GetMonth() << "/" << tdate.GetDay();
        fileDate << "." << tdate.GetHour() << ":" << tdate.GetMinute() << ":" << tdate.GetSecond();
        m_libDate = fileDate.str();

        lib << ".HEADER\n";
        lib << "LIBRARY_FILE 3.0 \"Created by " << m_idfSource;
        lib << "\" " << m_libDate << " " << (++m_libFileVersion ) << "\n";
        lib << ".END_HEADER\n\n";

        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator its = m_componentOutlines.begin();
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator ite = m_componentOutlines.end();

        while( its != ite )
        {
            its->second->writeData( lib );
            ++its;
        }

    }
    catch( const std::exception& )
    {
        lib.exceptions( std::ios_base::goodbit );
        CLOSE_STREAM( lib );
        throw;
    }

    CLOSE_STREAM( lib );
    return true;
}


void IDF3_BOARD::writeBoardFile( const std::string& aFileName )
{
    OPEN_OSTREAM( brd, aFileName.c_str() );

    try
    {
        brd.exceptions( std::ostream::failbit );

        if( brd.fail() )
        {
            ostringstream ostr;
            ostr << "\n* could not open file: '" << aFileName << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        brd.imbue( std::locale( "C" ) );
        wxDateTime tdate( time( nullptr ) );

        if( m_idfSource.empty() )
            m_idfSource = "KiCad-IDF Framework";

        ostringstream fileDate;
        fileDate << setfill( '0' ) << setw(4) << tdate.GetYear();
        fileDate << "/" << setw(2) << tdate.GetMonth() << "/" << tdate.GetDay();
        fileDate << "." << tdate.GetHour() << ":" << tdate.GetMinute() << ":" << tdate.GetSecond();
        m_brdDate = fileDate.str();

        brd << ".HEADER\n";
        brd << "BOARD_FILE 3.0 \"Created by " << m_idfSource;
        brd << "\" " << m_brdDate << " " << (++m_brdFileVersion ) << "\n";

        if( m_boardName.empty() )
            brd << "\"BOARD WITH NO NAME\" ";
        else
            brd << "\"" << m_boardName << "\" ";

        brd << setw(1) << setfill( ' ' );

        if( m_unit == IDF3::UNIT_MM )
            brd << "MM\n";
        else
            brd << "THOU\n";

        brd << ".END_HEADER\n\n";

        // write the BOARD_OUTLINE
        m_boardOutline.writeData( brd );

        // OTHER outlines
        do
        {
            std::map<std::string, OTHER_OUTLINE*>::iterator its = m_otherOutlines.begin();
            std::map<std::string, OTHER_OUTLINE*>::iterator ite = m_otherOutlines.end();

            while(its != ite )
            {
                its->second->writeData( brd );
                ++its;
            }

        } while( 0 );

        // ROUTE outlines
        do
        {
            std::list<ROUTE_OUTLINE*>::iterator its = m_routeOutlines.begin();
            std::list<ROUTE_OUTLINE*>::iterator ite = m_routeOutlines.end();

            while( its != ite )
            {
                (*its)->writeData( brd );
                ++its;
            }

        } while( 0 );

        // PLACEMENT outlines
        do
        {
            std::list<PLACE_OUTLINE*>::iterator its = m_placeOutlines.begin();
            std::list<PLACE_OUTLINE*>::iterator ite = m_placeOutlines.end();

            while( its != ite )
            {
                (*its)->writeData( brd );
                ++its;
            }

        } while( 0 );

        // ROUTE KEEPOUT outlines
        do
        {
            std::list<ROUTE_KO_OUTLINE*>::iterator its = m_routeKeepoutOutlines.begin();
            std::list<ROUTE_KO_OUTLINE*>::iterator ite = m_routeKeepoutOutlines.end();

            while( its != ite )
            {
                (*its)->writeData( brd );
                ++its;
            }

        } while( 0 );

        // VIA KEEPOUT outlines
        do
        {
            std::list<VIA_KO_OUTLINE*>::iterator its = m_viaKeepoutOutlines.begin();
            std::list<VIA_KO_OUTLINE*>::iterator ite = m_viaKeepoutOutlines.end();

            while( its != ite )
            {
                (*its)->writeData( brd );
                ++its;
            }

        } while( 0 );

        // PLACE KEEPOUT outlines
        do
        {
            std::list<PLACE_KO_OUTLINE*>::iterator its = m_placeKeepoutOutlines.begin();
            std::list<PLACE_KO_OUTLINE*>::iterator ite = m_placeKeepoutOutlines.end();

            while( its != ite )
            {
                (*its)->writeData( brd );
                ++its;
            }

        } while( 0 );

        // PLACEMENT GROUP outlines
        do
        {
            std::multimap<std::string, GROUP_OUTLINE*>::iterator its = m_groupOutlines.begin();
            std::multimap<std::string, GROUP_OUTLINE*>::iterator ite = m_groupOutlines.end();

            while( its != ite )
            {
                its->second->writeData( brd );
                ++its;
            }

        } while( 0 );

        // Drilled holes
        do
        {
            std::list<std::string>::iterator itds = m_drillComments.begin();
            std::list<std::string>::iterator itde = m_drillComments.end();

            while( itds != itde )
            {
                brd << "# " << *itds << "\n";
                ++itds;
            }

            brd << ".DRILLED_HOLES\n";

            std::list<IDF_DRILL_DATA*>::iterator itbs = m_drills.begin();
            std::list<IDF_DRILL_DATA*>::iterator itbe = m_drills.end();

            while( itbs != itbe )
            {
                (*itbs)->write( brd, m_unit );
                ++itbs;
            }

            std::map<std::string, IDF3_COMPONENT*>::iterator itcs = m_components.begin();
            std::map<std::string, IDF3_COMPONENT*>::iterator itce = m_components.end();

            while( itcs != itce )
            {
                itcs->second->writeDrillData( brd );
                ++itcs;
            }

            brd << ".END_DRILLED_HOLES\n\n";
        } while( 0 );

        // Notes
        if( !m_notes.empty() )
        {
            std::list<std::string>::iterator itncs = m_noteComments.begin();
            std::list<std::string>::iterator itnce = m_noteComments.end();

            while( itncs != itnce )
            {
                brd << "# " << *itncs << "\n";
                ++itncs;
            }

            brd << ".NOTES\n";

            std::list<IDF_NOTE*>::iterator itns = m_notes.begin();
            std::list<IDF_NOTE*>::iterator itne = m_notes.end();

            while( itns != itne )
            {
                (*itns)->writeNote( brd, m_unit );
                ++itns;
            }

            brd << ".END_NOTES\n\n";

        }

        // Placement
        if( !m_components.empty() )
        {
            std::list<std::string>::iterator itpcs = m_placeComments.begin();
            std::list<std::string>::iterator itpce = m_placeComments.end();

            while( itpcs != itpce )
            {
                brd << "# " << *itpcs << "\n";
                ++itpcs;
            }

            std::map< std::string, IDF3_COMPONENT*>::iterator itcs = m_components.begin();
            std::map< std::string, IDF3_COMPONENT*>::iterator itce = m_components.end();

            // determine if there are any component outlines at all and avoid
            // writing an empty PLACEMENT section if there are no outlines.
            // this will cost a little time but prevents software such as
            // CircuitWorks from segfaulting on an empty section.

            bool hasOutlines = false;

            while( itcs != itce )
            {
                if( itcs->second->GetOutlinesSize() > 0 )
                {
                    itcs = m_components.begin();
                    hasOutlines = true;
                    break;
                }

                ++itcs;
            }

            if( hasOutlines )
            {
                brd << ".PLACEMENT\n";

                while( itcs != itce )
                {
                    itcs->second->writePlaceData( brd );
                    ++itcs;
                }

                brd << ".END_PLACEMENT\n";
            }

        }

    }
    catch( const std::exception& )
    {
        brd.exceptions( std::ios_base::goodbit );
        CLOSE_STREAM( brd );
        throw;
    }

    CLOSE_STREAM( brd );
}


bool IDF3_BOARD::WriteFile( const wxString& aFullFileName, bool aUnitMM, bool aForceUnitFlag )
{
    if( aUnitMM != IDF3::UNIT_THOU )
        setUnit( IDF3::UNIT_MM, aForceUnitFlag );
    else
        setUnit( IDF3::UNIT_THOU, aForceUnitFlag );

    // 1. Check that the file extension is 'emn'
    // 2. Write the *.emn file according to the IDFv3 spec
    // 3. Write the *.emp file according to the IDFv3 spec

    wxFileName brdname( aFullFileName );
    wxFileName libname( aFullFileName );

    brdname.SetExt( wxT( "emn" ) );
    libname.SetExt( wxT( "emp" ) );

    std::string bfname = TO_UTF8( aFullFileName );

    try
    {
        if( !brdname.IsOk() )
        {
            ostringstream ostr;
            ostr << "\n* invalid file name: '" << bfname << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        if( brdname.FileExists() && !brdname.IsFileWritable() )
        {
            ostringstream ostr;
            ostr << "cannot overwrite existing board file\n";
            ostr << "* filename: '" << bfname << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        bfname = TO_UTF8( brdname.GetFullPath() );
        std::string lfname = TO_UTF8( libname.GetFullPath() );

        if( libname.FileExists() && !libname.IsFileWritable() )
        {
            ostringstream ostr;
            ostr << "cannot overwrite existing library file\n";
            ostr << "* filename: '" << lfname << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        writeLibFile( lfname );
        writeBoardFile( bfname );

    }
    catch( const std::exception& e )
    {
        m_errormsg = e.what();

        return false;
    }

    return true;
}


bool IDF3_BOARD::SetBoardVersion( int aVersion )
{
    if( aVersion < 0 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "*  board version (" << aVersion << ") must be >= 0";
        m_errormsg = ostr.str();

        return false;
    }

    m_brdFileVersion = aVersion;

    return true;
}


bool IDF3_BOARD::SetLibraryVersion( int aVersion )
{
    if( aVersion < 0 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* library version (" << aVersion << ") must be >= 0";
        m_errormsg = ostr.str();

        return false;
    }

    m_libFileVersion = aVersion;

    return true;
}


bool IDF3_BOARD::SetUserScale( double aScaleFactor )
{
    if( aScaleFactor == 0.0 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* BUG: user scale factor must not be 0";
        m_errormsg = ostr.str();

        return false;
    }

    m_userScale = aScaleFactor;
    return true;
}


bool IDF3_BOARD::SetUserPrecision( int aPrecision )
{
    if( aPrecision < 1 || aPrecision > 8 )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* precision value (" << aPrecision << ") must be 1..8";
        m_errormsg = ostr.str();

        return false;
    }

    m_userPrec = aPrecision;
    return true;
}


void IDF3_BOARD::GetUserOffset( double& aXoff, double& aYoff )
{
    aXoff = m_userXoff;
    aYoff = m_userYoff;
}


void IDF3_BOARD::SetUserOffset( double aXoff, double aYoff )
{
    m_userXoff = aXoff;
    m_userYoff = aYoff;
}


bool IDF3_BOARD::AddBoardOutline( IDF_OUTLINE* aOutline )
{
    if( !m_boardOutline.AddOutline( aOutline ) )
    {
        m_errormsg = m_boardOutline.GetError();

        return false;
    }

    return true;
}


bool IDF3_BOARD::DelBoardOutline( IDF_OUTLINE* aOutline )
{
    if( !m_boardOutline.DelOutline( aOutline ) )
    {
        m_errormsg = m_boardOutline.GetError();
        return false;
    }

    return true;
}


bool IDF3_BOARD::DelBoardOutline( size_t aIndex )
{
    if( !m_boardOutline.DelOutline( aIndex ) )
    {
        m_errormsg = m_boardOutline.GetError();
        return false;
    }

    return true;
}


size_t IDF3_BOARD::GetBoardOutlinesSize( void )
{
    return m_boardOutline.OutlinesSize();
}


IDF_DRILL_DATA* IDF3_BOARD::AddBoardDrill( double aDia, double aXpos, double aYpos,
                                           IDF3::KEY_PLATING aPlating,
                                           const std::string& aHoleType,
                                           IDF3::KEY_OWNER aOwner )
{
    IDF_DRILL_DATA* drill = nullptr;

    try
    {
        drill = new IDF_DRILL_DATA( aDia, aXpos, aYpos, aPlating, "BOARD", aHoleType, aOwner );
    }
    catch( std::bad_alloc& )
    {
        return nullptr;
    }

    m_drills.push_back( drill );

    return drill;
}


IDF_DRILL_DATA* IDF3_BOARD::AddDrill( IDF_DRILL_DATA* aDrilledHole )
{
    if( !aDrilledHole )
        return nullptr;

    // note: PANEL drills are essentially BOARD drills which
    // the panel requires to be present
    if( CompareToken( "BOARD", aDrilledHole->GetDrillRefDes() )
        || CompareToken( "PANEL", aDrilledHole->GetDrillRefDes() ) )
    {
        m_drills.push_back( aDrilledHole );
        return aDrilledHole;
    }

    return addCompDrill( aDrilledHole );
}


bool IDF3_BOARD::DelBoardDrill( double aDia, double aXpos, double aYpos )
{
    m_errormsg.clear();

    std::list<IDF_DRILL_DATA*>::iterator sp = m_drills.begin();
    std::list<IDF_DRILL_DATA*>::iterator ep = m_drills.end();
    bool rval = false;

    while( sp != ep )
    {
        if( (*sp)->Matches( aDia, aXpos, aYpos ) )
        {
#ifndef DISABLE_IDF_OWNERSHIP
            IDF3::KEY_OWNER keyo = (*sp)->GetDrillOwner();

            if( keyo == UNOWNED || ( keyo == MCAD && m_cadType == CAD_MECH )
                || ( keyo == ECAD && m_cadType == CAD_ELEC ) )
            {
                rval = true;
                delete *sp;
                sp = m_drills.erase( sp );
                continue;
            }
            else
            {
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "* ownership violation; drill owner (";

                switch( keyo )
                {
                case ECAD:
                    ostr << "ECAD";
                    break;

                case MCAD:
                    ostr << "MCAD";
                    break;

                default:
                    ostr << "invalid: " << keyo;
                    break;
                }

                ostr << ") may not be modified by ";

                if( m_cadType == CAD_MECH )
                    ostr << "MCAD";
                else
                    ostr << "ECAD";

                m_errormsg = ostr.str();

                ++sp;
                continue;
            }
#else
            rval = true;
            delete *sp;
            sp = m_drills.erase( sp );
            continue;
#endif
        }

        ++sp;
    }

    return rval;
}


bool IDF3_BOARD::AddSlot( double aWidth, double aLength, double aOrientation, double aX, double aY )
{
    if( aWidth < IDF_MIN_DIA_MM )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* slot width (" << aWidth << ") must be >= " << IDF_MIN_DIA_MM;
        m_errormsg = ostr.str();

        return false;
    }

    if( aLength < IDF_MIN_DIA_MM )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* slot length (" << aLength << ") must be >= " << IDF_MIN_DIA_MM;
        m_errormsg = ostr.str();

        return false;
    }

    IDF_POINT c[2];     // centers
    IDF_POINT pt[4];

    double a1 = aOrientation / 180.0 * M_PI;
    double a2 = a1 + M_PI_2;
    double d1 = aLength / 2.0;
    double d2 = aWidth / 2.0;
    double sa1 = sin( a1 );
    double ca1 = cos( a1 );
    double dsa2 = d2 * sin( a2 );
    double dca2 = d2 * cos( a2 );

    c[0].x  = aX + d1 * ca1;
    c[0].y  = aY + d1 * sa1;

    c[1].x  = aX - d1 * ca1;
    c[1].y  = aY - d1 * sa1;

    pt[0].x = c[0].x - dca2;
    pt[0].y = c[0].y - dsa2;

    pt[1].x = c[1].x - dca2;
    pt[1].y = c[1].y - dsa2;

    pt[2].x = c[1].x + dca2;
    pt[2].y = c[1].y + dsa2;

    pt[3].x = c[0].x + dca2;
    pt[3].y = c[0].y + dsa2;

    IDF_OUTLINE* outline = nullptr;

    try
    {
        outline = new IDF_OUTLINE;
    }
    catch( std::bad_alloc& )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* could not create an outline object";
        m_errormsg = ostr.str();

        return false;
    }

    // first straight run
    IDF_SEGMENT* seg = new IDF_SEGMENT( pt[0], pt[1] );
    outline->push( seg );

    // first 180 degree cap
    seg = new IDF_SEGMENT( c[1], pt[1], -180.0, true );
    outline->push( seg );

    // final straight run
    seg = new IDF_SEGMENT( pt[2], pt[3] );
    outline->push( seg );

    // final 180 degree cap
    seg = new IDF_SEGMENT( c[0], pt[3], -180.0, true );
    outline->push( seg );

    if( !m_boardOutline.addOutline( outline ) )
    {
        m_errormsg = m_boardOutline.GetError();
        return false;
    }

    return true;
}


IDF_DRILL_DATA* IDF3_BOARD::addCompDrill( double aDia, double aXpos, double aYpos,
                                          IDF3::KEY_PLATING aPlating,
                                          const std::string& aHoleType,
                                          IDF3::KEY_OWNER aOwner,
                                          const std::string& aRefDes )
{
    // first find the matching component; if it doesn't exist we must create it somehow -
    // question is, do we need a component outline at this stage or can those be added later?
    //
    // Presumably we can create a component with no outline and add the outlines later.
    // If a component is created and an outline specified but the outline is not loaded,
    // we're screwed if (a) we have already read the library file (*.emp) or (b) we don't
    // know the filename

    std::string refdes = aRefDes;

    // note: for components 'NOREFDES' would be assigned a Unique ID, but for holes
    // there is no way of associating the hole with the correct entity (if any)
    // so a hole added with "NOREFDES" goes to a generic component "NOREFDES"
    if( refdes.empty() )
        refdes = "NOREFDES";

    // check if the target is BOARD or PANEL
    if( CompareToken( "BOARD", refdes ) )
        return AddBoardDrill( aDia, aXpos, aYpos, aPlating, aHoleType, aOwner );

    if( CompareToken( "PANEL", refdes ) )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* PANEL data not supported";
        m_errormsg = ostr.str();

        return nullptr;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator ref = m_components.find( refdes );

    if( ref == m_components.end() )
    {
        // create the item
        IDF3_COMPONENT* comp = nullptr;
        try
        {
            comp = new IDF3_COMPONENT( this );
        }
        catch( std::bad_alloc& )
        {
            ostringstream ostr;
            ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
            ostr << "* could not create new component object";
            m_errormsg = ostr.str();

            return nullptr;
        }

        comp->SetParent( this );
        comp->SetRefDes( refdes );
        ref = m_components.emplace( comp->GetRefDes(), comp ).first;
    }

    // add the drill
    IDF_DRILL_DATA* dp = ref->second->AddDrill( aDia, aXpos, aYpos, aPlating, aHoleType, aOwner );

    if( !dp )
    {
        m_errormsg = ref->second->GetError();
        return nullptr;
    }

    return dp;
}


IDF_DRILL_DATA* IDF3_BOARD::addCompDrill( IDF_DRILL_DATA* aDrilledHole )
{
    if( !aDrilledHole )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): nullptr pointer";
        m_errormsg = ostr.str();

        return nullptr;
    }

    if( CompareToken( "PANEL", aDrilledHole->GetDrillRefDes() ) )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
        ostr << "* PANEL data not supported";
        m_errormsg = ostr.str();

        return nullptr;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator ref = m_components.find( aDrilledHole->GetDrillRefDes() );

    if( ref == m_components.end() )
    {
        // create the item
        IDF3_COMPONENT* comp;
        try
        {
            comp = new IDF3_COMPONENT( this );
        }
        catch( std::bad_alloc& )
        {
            ostringstream ostr;
            ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
            ostr << "* could not create new component object";
            m_errormsg = ostr.str();

            return nullptr;
        }

        comp->SetParent( this );
        comp->SetRefDes( aDrilledHole->GetDrillRefDes() );
        ref = m_components.emplace( comp->GetRefDes(), comp ).first;
    }

    IDF_DRILL_DATA* dp = ref->second->AddDrill( aDrilledHole );

    if( !dp )
    {
        m_errormsg = ref->second->GetError();
        return nullptr;
    }

    return dp;
}


bool IDF3_BOARD::delCompDrill( double aDia, double aXpos, double aYpos, const std::string& aRefDes )
{
    m_errormsg.clear();

    std::map<std::string, IDF3_COMPONENT*>::iterator ref = m_components.find( aRefDes );

    if( ref == m_components.end() )
        return false;

    if( !ref->second->DelDrill( aDia, aXpos, aYpos ) )
    {
        m_errormsg = ref->second->GetError();
        return false;
    }

    return true;
}


bool IDF3_BOARD::AddComponent( IDF3_COMPONENT* aComponent )
{
    if( !aComponent )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__;
        ostr << "(): Invalid component pointer (nullptr)";
        m_errormsg = ostr.str();

        return false;
    }

    if( m_components.emplace( aComponent->GetRefDes(), aComponent ).second == false )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): \n";
        ostr << "* duplicate RefDes ('" << aComponent->GetRefDes() << "')";
        m_errormsg = ostr.str();

        return false;
    }

    return true;
}


bool IDF3_BOARD::DelComponent( IDF3_COMPONENT* aComponent )
{
    m_errormsg.clear();

#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkComponentOwnership( __LINE__, __FUNCTION__, aComponent ) )
        return false;
#endif

    std::map<std::string, IDF3_COMPONENT*>::iterator it = m_components.find( aComponent->GetRefDes() );

    if( it == m_components.end() )
        return false;

    delete it->second;
    m_components.erase( it );

    return true;
}


bool IDF3_BOARD::DelComponent( size_t aIndex )
{
    if( aIndex >= m_components.size() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): \n";
        ostr << "* aIndex (" << aIndex << ") out of range (" << m_components.size() << ")";
        m_errormsg = ostr.str();

        return false;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator it = m_components.begin();

    while( aIndex-- > 0 ) ++it;

#ifndef DISABLE_IDF_OWNERSHIP
    if( !checkComponentOwnership( __LINE__, __FUNCTION__, it->second ) )
        return false;
#endif

    delete it->second;
    m_components.erase( it );

    return true;
}


size_t IDF3_BOARD::GetComponentsSize( void )
{
    return m_components.size();
}


std::map< std::string, IDF3_COMPONENT* >* IDF3_BOARD::GetComponents( void )
{
    return &m_components;
}


IDF3_COMPONENT* IDF3_BOARD::FindComponent( const std::string& aRefDes )
{
    std::map<std::string, IDF3_COMPONENT*>::iterator it = m_components.find( aRefDes );

    if( it == m_components.end() )
        return nullptr;

    return it->second;
}


IDF3_COMP_OUTLINE* IDF3_BOARD::GetComponentOutline( const wxString& aFullFileName )
{
    std::string fname = TO_UTF8( aFullFileName );
    wxFileName idflib( aFullFileName );

    if( !idflib.IsOk() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): \n";
        cerr << "* invalid file name: '" << fname << "'";
        m_errormsg = ostr.str();

        return nullptr;
    }

    if( !idflib.FileExists() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): \n";
        cerr << "* no such file: '" << fname  << "'";
        m_errormsg = ostr.str();

        return nullptr;
    }

    if( !idflib.IsFileReadable() )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): \n";
        cerr << "* cannot read file: '" << fname << "'";
        m_errormsg = ostr.str();

        return nullptr;
    }

    std::map<std::string, std::string>::iterator itm = m_uidFileList.find( fname );

    if( itm != m_uidFileList.end() )
        return GetComponentOutline( itm->second );

    IDF3_COMP_OUTLINE* cp = nullptr;

    try
    {
        cp = new IDF3_COMP_OUTLINE( this );
    }
    catch( std::bad_alloc& )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): \n";
        cerr << "* failed to create outline\n";
        cerr << "* filename: '" << fname << "'";
        m_errormsg = ostr.str();

        return nullptr;
    }

    OPEN_ISTREAM( model, fname.c_str() );

    try
    {
        model.exceptions ( std::ios_base::badbit );

        if( model.fail() )
        {
            ostringstream ostr;
            ostr << "\n* could not open file: '" << fname << "'";

            throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
        }

        model.imbue( std::locale( "C" ) );
        std::string iline;      // the input line
        bool isComment;         // true if a line just read in is a comment line
        std::streampos pos;

        while( true )
        {
            while( !FetchIDFLine( model, iline, isComment, pos ) && model.good() );

            if( !model.good() )
            {
                ostringstream ostr;
                ostr << "\n* problems reading file: '" << fname << "'";
                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }

            // accept comment lines, .ELECTRICAL, or .MECHANICAL only
            if( isComment )
            {
                cp->AddComment( iline );
                continue;
            }

            if( CompareToken( ".ELECTRICAL", iline ) || CompareToken( ".MECHANICAL", iline ) )
            {
                cp->readData( model, iline, m_idfVer );
                break;
            }
            else
            {
                ostringstream ostr;
                ostr << "faulty IDF component definition\n";
                ostr << "* Expecting .ELECTRICAL or .MECHANICAL, got '" << iline << "'\n";
                cerr << "* File: '" << fname << "'\n";

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            }
        }
    }
    catch( const std::exception& e )
    {
        delete cp;
        model.exceptions ( std::ios_base::goodbit );
        CLOSE_STREAM( model );
        m_errormsg = e.what();
        return nullptr;
    }

    CLOSE_STREAM( model );

    // check the unique ID against the list from library components
    std::list<std::string>::iterator lsts = m_uidLibList.begin();
    std::list<std::string>::iterator lste = m_uidLibList.end();
    std::string                      uid = cp->GetUID();
    IDF3_COMP_OUTLINE*               oldp = nullptr;

    while( lsts != lste )
    {
        if( ! lsts->compare( uid ) )
        {
            oldp = GetComponentOutline( uid );

            if( MatchCompOutline( cp, oldp ) )
            {
                // everything is fine; the outlines are genuine duplicates; delete the copy
                delete cp;

                // make sure we can find the item via its filename
                m_uidFileList.emplace( fname, uid );

                // return the pointer to the original
                return oldp;
            }
            else
            {
                delete cp;
                ostringstream ostr;
                ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                ostr << "* duplicate UID for different Component Outlines: '" << uid << "'\n";
                ostr << "* original loaded from library, duplicate in current file\n";
                ostr << "* file: '" << fname << "'";

                m_errormsg = ostr.str();
                return nullptr;
            }
        }

        ++lsts;
    }

    // if we got this far then any duplicates are from files previously read
    oldp = GetComponentOutline( uid );

    if( oldp == nullptr )
    {
        // everything is fine, there are no existing entries
        m_uidFileList.emplace( fname, uid );
        m_componentOutlines.emplace( uid, cp );

        return cp;
    }

    if( MatchCompOutline( cp, oldp ) )
    {
        // everything is fine; the outlines are genuine duplicates; delete the copy
        delete cp;

        // make sure we can find the item via its other filename
        m_uidFileList.emplace( fname, uid );

        // return the pointer to the original
        return oldp;
    }

    delete cp;

    // determine the file name of the first instance
    std::map<std::string, std::string>::iterator ufls = m_uidFileList.begin();
    std::map<std::string, std::string>::iterator ufle = m_uidFileList.end();
    std::string                                  oldfname;

    while( ufls != ufle )
    {
        if( ! ufls->second.compare( uid ) )
        {
            oldfname = ufls->first;
            break;
        }

        ++ufls;
    }

    ostringstream ostr;
    ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
    ostr << "* duplicate UID for different Component Outlines: '" << uid << "'\n";
    ostr << "* original file: '" << oldfname << "'\n";
    ostr << "* this file: '" << fname << "'";

    m_errormsg = ostr.str();
    return nullptr;
}


IDF3_COMP_OUTLINE* IDF3_BOARD::GetComponentOutline( const std::string& aComponentID )
{
    std::map< std::string, IDF3_COMP_OUTLINE*>::iterator its = m_componentOutlines.find( aComponentID );

    if( its != m_componentOutlines.end() )
        return its->second;

    return nullptr;
}


IDF3_COMP_OUTLINE* IDF3_BOARD::GetInvalidOutline( const std::string& aGeomName,
                                                  const std::string& aPartName )
{
    std::string uid;
    bool empty = false;

    if( aGeomName.empty() && aPartName.empty() )
    {
        uid = "NOGEOM_NOPART";
        empty = true;
    }
    else
    {
        uid = aGeomName + "_" + aPartName;
    }

    IDF3_COMP_OUTLINE* cp = GetComponentOutline( uid );

    if( cp != nullptr )
        return cp;

    try
    {
        cp = new IDF3_COMP_OUTLINE( this );
    }
    catch( std::bad_alloc& )
    {
        ostringstream ostr;
        ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "(): ";
        cerr << "could not create new outline";
        m_errormsg = ostr.str();

        return nullptr;
    }

    if( empty )
        cp->CreateDefaultOutline( "", "" );
    else
        cp->CreateDefaultOutline( aGeomName, aPartName );

    m_componentOutlines.emplace( cp->GetUID(), cp );

    return cp;
}


void IDF3_BOARD::Clear( void )
{
    // preserve the board thickness
    double thickness = m_boardOutline.GetThickness();

    m_idfSource.clear();
    m_brdSource.clear();
    m_libSource.clear();
    m_brdDate.clear();
    m_libDate.clear();
    m_uidFileList.clear();
    m_uidLibList.clear();
    m_brdFileVersion = 0;
    m_libFileVersion = 0;
    m_refDesCounter = 0;
    m_refDesString.clear();

    // delete comment lists
    m_noteComments.clear();
    m_drillComments.clear();
    m_placeComments.clear();

    // delete notes
    while( !m_notes.empty() )
    {
        delete m_notes.front();
        m_notes.pop_front();
    }

    // delete drill list
    do
    {
        std::list<IDF_DRILL_DATA*>::iterator ds = m_drills.begin();
        std::list<IDF_DRILL_DATA*>::iterator de = m_drills.end();

        while( ds != de )
        {
            delete *ds;
            ++ds;
        }

        m_drills.clear();
    } while( 0 );


    // delete components
    do
    {
        std::map<std::string, IDF3_COMPONENT*>::iterator cs = m_components.begin();
        std::map<std::string, IDF3_COMPONENT*>::iterator ce = m_components.end();

        while( cs != ce )
        {
            delete cs->second;
            ++cs;
        }

        m_components.clear();
    } while( 0 );


    // delete component outlines
    do
    {
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator cs = m_componentOutlines.begin();
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator ce = m_componentOutlines.end();

        while( cs != ce )
        {
            delete cs->second;
            ++cs;
        }

        m_componentOutlines.clear();
    } while( 0 );


    // delete OTHER outlines
    do
    {
        std::map<std::string, OTHER_OUTLINE*>::iterator os = m_otherOutlines.begin();
        std::map<std::string, OTHER_OUTLINE*>::iterator oe = m_otherOutlines.end();

        while( os != oe )
        {
            delete os->second;
            ++os;
        }

        m_otherOutlines.clear();
    } while( 0 );


    // delete ROUTE outlines
    do
    {
        std::list<ROUTE_OUTLINE*>::iterator os = m_routeOutlines.begin();
        std::list<ROUTE_OUTLINE*>::iterator oe = m_routeOutlines.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        m_routeOutlines.clear();
    } while( 0 );


    // delete PLACE outlines
    do
    {
        std::list<PLACE_OUTLINE*>::iterator os = m_placeOutlines.begin();
        std::list<PLACE_OUTLINE*>::iterator oe = m_placeOutlines.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        m_placeOutlines.clear();
    } while( 0 );


    // delete ROUTE KEEPOUT outlines
    do
    {
        std::list<ROUTE_KO_OUTLINE*>::iterator os = m_routeKeepoutOutlines.begin();
        std::list<ROUTE_KO_OUTLINE*>::iterator oe = m_routeKeepoutOutlines.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        m_routeKeepoutOutlines.clear();
    } while( 0 );


    // delete VIA KEEPOUT outlines
    do
    {
        std::list<VIA_KO_OUTLINE*>::iterator os = m_viaKeepoutOutlines.begin();
        std::list<VIA_KO_OUTLINE*>::iterator oe = m_viaKeepoutOutlines.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        m_viaKeepoutOutlines.clear();
    } while( 0 );


    // delete PLACEMENT KEEPOUT outlines
    do
    {
        std::list<PLACE_KO_OUTLINE*>::iterator os = m_placeKeepoutOutlines.begin();
        std::list<PLACE_KO_OUTLINE*>::iterator oe = m_placeKeepoutOutlines.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        m_placeKeepoutOutlines.clear();
    } while( 0 );


    // delete PLACEMENT GROUP outlines
    do
    {
        std::multimap<std::string, GROUP_OUTLINE*>::iterator os = m_groupOutlines.begin();
        std::multimap<std::string, GROUP_OUTLINE*>::iterator oe = m_groupOutlines.end();

        while( os != oe )
        {
            delete os->second;
            ++os;
        }

        m_groupOutlines.clear();
    } while( 0 );

    m_boardName.clear();
    m_boardOutline.setThickness( thickness );

    m_unit      = UNIT_MM;
    m_userScale = 1.0;
    m_userXoff  = 0.0;
    m_userYoff  = 0.0;
}
