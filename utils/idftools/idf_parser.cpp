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
#include <fstream>
#include <sstream>
#include <cmath>
#include <cerrno>
#include <algorithm>

#include <idf_parser.h>
#include <idf_helpers.h>

using namespace std;
using namespace IDF3;

/*
 * CLASS: IDF3_COMP_OUTLINE_DATA
 * This represents the outline placement
 * information and other data specific to
 * each component instance.
 */
IDF3_COMP_OUTLINE_DATA::IDF3_COMP_OUTLINE_DATA()
{
    parent = NULL;
    outline = NULL;
    xoff = 0.0;
    yoff = 0.0;
    zoff = 0.0;
    aoff = 0.0;

    return;
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
        aOutline->IncrementRef();

    return;
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
    return;
}

IDF3_COMP_OUTLINE_DATA::~IDF3_COMP_OUTLINE_DATA()
{
    if( outline )
        outline->DecrementRef();

    return;
}

void IDF3_COMP_OUTLINE_DATA::SetOffsets( double aXoff, double aYoff,
                                         double aZoff, double aAngleOff )
{
    xoff = aXoff;
    yoff = aYoff;
    zoff = aZoff;
    aoff = aAngleOff;
    return;
}

void IDF3_COMP_OUTLINE_DATA::GetOffsets( double& aXoff, double& aYoff,
                                         double& aZoff, double& aAngleOff )
{
    aXoff = xoff;
    aYoff = yoff;
    aZoff = zoff;
    aAngleOff = aoff;
    return;
}


void IDF3_COMP_OUTLINE_DATA::SetParent( IDF3_COMPONENT* aParent )
{
    parent = aParent;
}

void IDF3_COMP_OUTLINE_DATA::SetOutline( IDF3_COMP_OUTLINE* aOutline )
{
    if( outline )
        outline->DecrementRef();

    outline = aOutline;

    if( outline )
        outline->IncrementRef();

    return;
}

bool IDF3_COMP_OUTLINE_DATA::ReadPlaceData( std::ifstream &aBoardFile,
                                            IDF3::FILE_STATE& aBoardState, IDF3_BOARD *aBoard )
{
    if( !aBoard )
    {
        ERROR_IDF;
        cerr << "BUG: invoked with no reference to the parent IDF_BOARD\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    // clear out data possibly left over from previous use of the object
    outline = NULL;
    parent  = NULL;

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
        ERROR_IDF;
        cerr << "problems reading PLACEMENT SECTION\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: comment within a section (PLACEMENT)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( !quoted && CompareToken( ".END_PLACEMENT", token ) )
    {
        errno = 0;
        aBoardState = IDF3::FILE_PLACEMENT;
        return false;
    }

    std::string ngeom = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no PART NAME in PLACEMENT RECORD2\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    std::string npart = token;
    uid = ngeom + "_" + npart;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no REFDES in PLACEMENT RECORD2\n";
        cerr << "* Line: '" << iline << "'\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
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
        refdes = token;
    }
    else if( CompareToken( "BOARD", token ) )
    {
        ERROR_IDF;
        cerr << "unsupported feature\n";
        cerr << "* RefDes is 'BOARD', indicating this is a PANEL FILE (not supported)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }
    else if( CompareToken( "PANEL", token ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: RefDes in PLACEMENT RECORD2 is 'PANEL'\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }
    else if( token.empty() )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: empty RefDes string in PLACEMENT RECORD2\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }
    else
    {
        // note: perversely, spaces can be a valid RefDes
        refdes = token;
    }

    // RECORD 3: X, Y, Z, ROT, SIDE (top/bot), PLACEMENT (placed, unplaced, mcad, ecad)
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
    {
        ERROR_IDF;
        cerr << "problems reading PLACEMENT SECTION, RECORD 3\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: comment within a section (PLACEMENT)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: X value must not be in quotes (PLACEMENT RECORD 3)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    istringstream istr;
    istr.str( token );

    istr >> xoff;
    if( istr.fail() )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: X value is not numeric (PLACEMENT RECORD 3)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no Y value in PLACEMENT RECORD 3\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    istr.clear();
    istr.str( token );

    istr >> yoff;
    if( istr.fail() )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: Y value is not numeric (PLACEMENT RECORD 3)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no Z value in PLACEMENT RECORD 3\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    istr.clear();
    istr.str( token );

    istr >> zoff;
    if( istr.fail() )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: Z value is not numeric (PLACEMENT RECORD 3)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no rotation value in PLACEMENT RECORD 3\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    istr.clear();
    istr.str( token );

    istr >> aoff;
    if( istr.fail() )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: rotation value is not numeric (PLACEMENT RECORD 3)\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no SIDE value in PLACEMENT RECORD 3\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
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
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: SIDE value in PLACEMENT RECORD 3 is invalid ('";
        cerr << token << "')\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: no PLACEMENT value in PLACEMENT RECORD 3\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( CompareToken( "PLACED", token ) )
    {
        placement = IDF3::PS_PLACED;
    }
    else if( CompareToken( "UNPLACED", token ) )
    {
        placement = IDF3::PS_UNPLACED;
    }
    else if( CompareToken( "MCAD", token ) )
    {
        placement = IDF3::PS_MCAD;
    }
    else if( CompareToken( "ECAD", token ) )
    {
        placement = IDF3::PS_ECAD;
    }
    else
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: PLACEMENT value in PLACEMENT RECORD 3 is invalid ('";
        cerr << token << "')\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    outline = aBoard->GetComponentOutline( uid );

    if( outline == NULL )
    {
        ERROR_IDF << "MISSING OUTLINE\n";
        cerr << "* GeomName( " << ngeom << " ), PartName( " << npart << " )\n";
        cerr << "* Substituting default outline.\n";
        outline = aBoard->GetInvalidOutline( ngeom, npart );

        if( outline == NULL )
        {
            ERROR_IDF << "cannot create outline object\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }
    }

    if( aBoard->GetUnit() == IDF3::UNIT_THOU )
    {
        xoff *= IDF_MM_TO_THOU;
        yoff *= IDF_MM_TO_THOU;
        zoff *= IDF_MM_TO_THOU;
    }

    parent = aBoard->FindComponent( refdes );

    if( parent == NULL )
    {
        IDF3_COMPONENT* cp = new IDF3_COMPONENT( aBoard );

        if( cp == NULL )
        {
            ERROR_IDF << "cannot create component object\n";
            aBoardState = IDF3::FILE_INVALID;
            outline = NULL;
            return false;
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
                ERROR_IDF << "inconsistent PLACEMENT data\n";
                cerr << "* SIDE value has changed from " << GetLayerString( tL );
                cerr << " to " << GetLayerString( side ) << "\n";
                aBoardState = IDF3::FILE_INVALID;
                outline = NULL;
                return false;
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
            ERROR_IDF << "inconsistent PLACEMENT data\n";
            cerr << "* placement value has changed from " << GetPlacementString( parent->GetPlacement() );
            cerr << " to " << GetPlacementString( placement ) << "\n";
            cerr << "* line: '" << iline << "'\n";
            aBoardState = IDF3::FILE_INVALID;
            outline = NULL;
            return false;
        }

    }

    // copy internal data to a new object and push it into the component's outline list
    IDF3_COMP_OUTLINE_DATA* cdp = new IDF3_COMP_OUTLINE_DATA;
    *cdp = *this;
    outline->IncrementRef();
    outline = NULL;

    if( !parent->AddOutlineData( cdp ) )
    {
        ERROR_IDF << "could not add outline data object\n";
        aBoardState = IDF3::FILE_INVALID;
        delete cdp;
        return false;
    }

    return true;
}

bool IDF3_COMP_OUTLINE_DATA::WritePlaceData( std::ofstream& aBoardFile,
                                             double aXpos, double aYpos, double aAngle,
                                             const std::string aRefDes,
                                             IDF3::IDF_PLACEMENT aPlacement,
                                             IDF3::IDF_LAYER aSide )
{
    if( outline == NULL )
        return true;

    if( outline->GetUID().empty() )
    {
        ERROR_IDF << "invalid GEOM/PART names\n";
        return false;
    }

    if( aPlacement == PS_INVALID )
    {
        ERROR_IDF << "placement invalid; defaulting to PLACED\n";
        aPlacement = PS_PLACED;
    }

    if( aSide != LYR_TOP && aSide != LYR_BOTTOM )
    {
        ERROR_IDF << "invalid side (" << aSide << "); must be TOP or BOTTOM\n";
        return false;
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
        aBoardFile << setiosflags(ios::fixed) << setprecision(1) << (xpos / IDF_MM_TO_THOU) << " "
        << (ypos / IDF_MM_TO_THOU) << " "  << (zoff / IDF_MM_TO_THOU) << " "
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

    return !aBoardFile.fail();
}


/*
 * CLASS: IDF3_COMPONENT
 *
 * This represents a component and its associated
 * IDF outlines and ancillary data (position, etc)
 */
IDF3_COMPONENT::IDF3_COMPONENT()
{
    xpos   = 0.0;
    ypos   = 0.0;
    angle  = 0.0;
    parent = NULL;

    hasPosition = false;
    placement   = PS_INVALID;
    layer       = LYR_INVALID;
    return;
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
    return;
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

    return;
}

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
    if( aRefDes.empty() )
    {
        ERROR_IDF << "invalid RefDes (empty)\n";
        return false;
    }

    if( CompareToken( "PANEL", aRefDes ) )
    {
        ERROR_IDF;
        cerr << "\n*BUG: PANEL is a reserved designator and may not be used by components\n";
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
                                          const std::string aHoleType,
                                          IDF3::KEY_OWNER aOwner )
{
    IDF_DRILL_DATA* dp = new IDF_DRILL_DATA( aDia, aXpos, aYpos, aPlating,
                                             refdes, aHoleType, aOwner );

    if( dp == NULL )
        return NULL;

    drills.push_back( dp );

    return dp;
}

IDF_DRILL_DATA* IDF3_COMPONENT::AddDrill( IDF_DRILL_DATA* aDrilledHole )
{
    if( !aDrilledHole )
        return NULL;

    if( CompareToken( "PANEL", refdes ) )
    {
        ERROR_IDF;
        cerr << "\n*BUG: PANEL drills not supported\n";
        return NULL;
    }

    if( refdes.compare( aDrilledHole->GetDrillRefDes() ) )
    {
        ERROR_IDF;
        cerr << "\n*BUG: pushing an incorrect REFDES ('" << aDrilledHole->GetDrillRefDes();
        cerr << "') to component ('" << refdes << "')\n";
        return NULL;
    }

    drills.push_back( aDrilledHole );

    return aDrilledHole;
}


bool IDF3_COMPONENT::DelDrill( double aDia, double aXpos, double aYpos )
{
    if( drills.empty() )
        return false;

    // XXX - throw on ownership violation

    bool val = false;

    list< IDF_DRILL_DATA* >::iterator itS = drills.begin();
    list< IDF_DRILL_DATA* >::iterator itE = drills.end();

    while( !drills.empty() && itS != itE )
    {
        if( (*itS)->Matches( aDia, aXpos, aYpos ) )
        {
            val = true;
            delete *itS;
            drills.erase( itS );
            itS = drills.begin();
            itE = drills.end();
            continue;
        }
        ++itS;
    }

    return val;
}

bool IDF3_COMPONENT::DelDrill( IDF_DRILL_DATA* aDrill )
{
    if( drills.empty() )
        return false;

    // XXX - throw on ownership violation

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

const std::list< IDF_DRILL_DATA* >*const IDF3_COMPONENT::GetDrills( void )
{
    return &drills;
}

bool IDF3_COMPONENT::AddOutlineData( IDF3_COMP_OUTLINE_DATA* aComponentOutline )
{
    if( aComponentOutline == NULL )
        return false;

    components.push_back( aComponentOutline );

    return true;
}

bool IDF3_COMPONENT::DeleteOutlineData( IDF3_COMP_OUTLINE_DATA* aComponentOutline )
{
    if( components.empty() || aComponentOutline == NULL )
        return false;

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
    if( aIndex >= components.size() )
        return false;

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

const std::list< IDF3_COMP_OUTLINE_DATA* >*const IDF3_COMPONENT::GetOutlinesData( void )
{
    return &components;
}

bool IDF3_COMPONENT::GetPosition( double& aXpos, double& aYpos, double& aAngle,
                                  IDF3::IDF_LAYER& aLayer )
{
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

bool IDF3_COMPONENT::SetPosition( double aXpos, double aYpos, double aAngle, IDF3::IDF_LAYER aLayer )
{
    switch( aLayer )
    {
        case LYR_TOP:
        case LYR_BOTTOM:
            break;

        default:
            ERROR_IDF << "invalid side (must be TOP or BOTTOM only): " << aLayer << "\n";
            return false;
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


void IDF3_COMPONENT::SetPlacement( IDF3::IDF_PLACEMENT aPlacementValue )
{
    // XXX - throw on ownership violation or invalid placement value
    if( aPlacementValue < PS_UNPLACED || aPlacementValue >= PS_INVALID )
        return;

    placement = aPlacementValue;
    return;
}

bool IDF3_COMPONENT::WriteDrillData( std::ofstream& aBoardFile )
{
    if( drills.empty() )
        return true;

    std::list< IDF_DRILL_DATA* >::iterator itS = drills.begin();
    std::list< IDF_DRILL_DATA* >::iterator itE = drills.end();

    while( itS != itE )
    {
        if( !(*itS)->Write( aBoardFile, GetUnit() ) )
            return false;

        ++itS;
    }

    return true;
}

bool IDF3_COMPONENT::WritePlaceData( std::ofstream& aBoardFile )
{
    if( components.empty() )
        return true;

    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itS = components.begin();
    std::list< IDF3_COMP_OUTLINE_DATA* >::iterator itE = components.end();

    while( itS != itE )
    {
        if( !(*itS)->WritePlaceData( aBoardFile, xpos, ypos, angle, refdes, placement, layer ) )
            return false;

        ++itS;
    }

    return true;
}


IDF3_BOARD::IDF3_BOARD( IDF3::CAD_TYPE aCadType )
{
    state          = FILE_START;
    cadType        = aCadType;
    userPrec       = 5;
    userScale      = 1.0;
    userXoff       = 0.0;
    userYoff       = 0.0;
    brdFileVersion = 0;
    libFileVersion = 0;

    // unlike other outlines which are created as necessary,
    // the board outline always exists and its parent must
    // be set here
    olnBoard.SetParent( this );
    olnBoard.SetThickness( 1.6 );

    return;
}

IDF3_BOARD::~IDF3_BOARD()
{
    Clear();

    return;
}

IDF3::CAD_TYPE IDF3_BOARD::GetCadType( void )
{
    return cadType;
}

void IDF3_BOARD::SetBoardName( std::string aBoardName )
{
    boardName = aBoardName;
    return;
}

const std::string& IDF3_BOARD::GetBoardName( void )
{
    return boardName;
}

bool IDF3_BOARD::setUnit( IDF3::IDF_UNIT aUnit, bool convert )
{
    switch( aUnit )
    {
    case UNIT_MM:
    case UNIT_THOU:
        unit = aUnit;
        break;

    default:
        ERROR_IDF << "invalid board unit\n";
        return false;
        break;
    }

    // iterate through all owned OUTLINE objects (except IDF3_COMP_OUTLINE)
    // and set to the same unit

    olnBoard.SetUnit( aUnit );

    do
    {
        std::map< std::string, OTHER_OUTLINE*>::iterator its = olnOther.begin();
        std::map< std::string, OTHER_OUTLINE*>::iterator ite = olnOther.end();

        while( its != ite )
        {
            its->second->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<ROUTE_OUTLINE*>::iterator its = olnRoute.begin();
        std::list<ROUTE_OUTLINE*>::iterator ite = olnRoute.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<PLACE_OUTLINE*>::iterator its = olnPlace.begin();
        std::list<PLACE_OUTLINE*>::iterator ite = olnPlace.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<ROUTE_KO_OUTLINE*>::iterator its = olnRouteKeepout.begin();
        std::list<ROUTE_KO_OUTLINE*>::iterator ite = olnRouteKeepout.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<VIA_KO_OUTLINE*>::iterator its = olnViaKeepout.begin();
        std::list<VIA_KO_OUTLINE*>::iterator ite = olnViaKeepout.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::list<PLACE_KO_OUTLINE*>::iterator its = olnPlaceKeepout.begin();
        std::list<PLACE_KO_OUTLINE*>::iterator ite = olnPlaceKeepout.end();

        while( its != ite )
        {
            (*its)->SetUnit( aUnit );
            ++its;
        }

    } while( 0 );

    do
    {
        std::map<std::string, GROUP_OUTLINE*>::iterator its = olnGroup.begin();
        std::map<std::string, GROUP_OUTLINE*>::iterator ite = olnGroup.end();

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
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator its = compOutlines.begin();
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator ite = compOutlines.end();

        while( its != ite )
        {
            its->second->SetUnit( aUnit );
            ++its;
        }

    }

    return true;
}


IDF3::IDF_UNIT IDF3_BOARD::GetUnit( void )
{
    return unit;
}


bool IDF3_BOARD::SetBoardThickness( double aBoardThickness )
{
    if( aBoardThickness <= 0.0 )
    {
        ERROR_IDF << "board thickness must be > 0\n";
        return false;
    }

    return olnBoard.SetThickness( aBoardThickness );
}


double IDF3_BOARD::GetBoardThickness( void )
{
    return olnBoard.GetThickness();
}


// read the DRILLED HOLES section
bool IDF3_BOARD::readBrdDrills( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState )
{
    IDF_DRILL_DATA drill;

    while( drill.Read( aBoardFile, unit, aBoardState ) )
    {
        if( CompareToken( "PANEL", drill.GetDrillRefDes() ) )
        {
            ERROR_IDF;
            cerr << "\n[INFO]: Dropping unsupported drill refdes: 'PANEL' (not supported)\n";
            continue;
        }

        IDF_DRILL_DATA *dp = new IDF_DRILL_DATA;
        *dp = drill;
        if( AddDrill( dp ) == NULL )
        {
            delete dp;
            ERROR_IDF;
            cerr << "\n* BUG: could not add drill data; cannot continue reading the file\n";
            aBoardState = FILE_INVALID;
            return false;
        }
    }

    if( errno == 0 && aBoardState != IDF3::FILE_INVALID )
        return true;

    return false;

}


// read the NOTES section
bool IDF3_BOARD::readBrdNotes( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState )
{
    IDF_NOTE note;

    while( note.ReadNote( aBoardFile, aBoardState, unit ) )
    {
        IDF_NOTE *np = new IDF_NOTE;
        *np = note;
        notes.push_back( np );
    }

    if( errno == 0 && aBoardState != IDF3::FILE_INVALID )
        return true;

    return false;
}


// read the component placement section
bool IDF3_BOARD::readBrdPlacement( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState )
{
    IDF3_COMP_OUTLINE_DATA oldata;

    while( oldata.ReadPlaceData( aBoardFile, aBoardState, this ) );

    if( errno == 0 && aBoardState != IDF3::FILE_INVALID )
        return true;

    ERROR_IDF << "problems reading board PLACEMENT section\n";

    return false;

}


// read the board HEADER
bool IDF3_BOARD::readBrdHeader( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState )
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
    {
        ERROR_IDF;
        cerr << "problems reading board header\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: first line must be .HEADER\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !CompareToken( ".HEADER", iline ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: first line must be .HEADER and have no quotes or trailing text\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    // RECORD 2:
    //      File Type [str]: BOARD_FILE (PANEL_FILE not supported)
    //      IDF Version Number [float]: must be 3.0
    //      Source System [str]: ignored
    //      Date [str]: ignored
    //      Board File Version [int]: ignored
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
    {
        ERROR_IDF;
        cerr << "problems reading board header, RECORD 2\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: comment within .HEADER section\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: File Type in HEADER section must not be in quotes\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !CompareToken( "BOARD_FILE", token ) )
    {
        ERROR_IDF;

        if( CompareToken( "PANEL_FILE", token ) )
        {
            cerr << "not a board file\n";
            cerr << "* PANEL_FILE is not supported (expecting BOARD_FILE)\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }
        else
        {
            cerr << "invalid IDFv3 file\n";
            cerr << "* Expecting string: BOARD_FILE\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2: no FIELD 2\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: IDF Version must not be in quotes\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( token.compare( "3.0" ) && token.compare( "3." ) && token.compare( "3" ) )
    {
        ERROR_IDF;
        cerr << "unsupported IDF version\n";
        cerr << "* Expecting version to be one of '3.0', '3.', or '3' (value: '" << token << "')\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2, FIELD 3: no Source System string\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }
    brdSource = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2, FIELD 4: no Date string\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }
    brdDate = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2, FIELD 5: no Board File Version number\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }
    std::istringstream istr;
    istr.str( token );

    istr >> brdFileVersion;

    if( istr.fail() )
    {
        ERROR_IDF << "invalid Board File Version in header\n";
        cerr << "* Setting default version of 1\n";
        brdFileVersion = 1;
    }

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: HEADER section, RECORD 2, FIELD 5: Board File Version must not be in quotes\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    // RECORD 3:
    //      Board Name [str]: stored
    //      Units [str]: MM or THOU
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( !aBoardFile.good() )
    {
        ERROR_IDF;
        cerr << "problems reading board header, RECORD 2\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: comment within .HEADER section\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    boardName = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification for HEADER section, RECORD 3, FIELD 1: no Board Name\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: HEADER section, RECORD 3, FIELD 2: UNIT may not be in quotes\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( CompareToken( "MM", token ) )
    {
        unit = IDF3::UNIT_MM;
    }
    else if( CompareToken( "THOU", token ) )
    {
        unit = IDF3::UNIT_THOU;
    }
    else
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* HEADER section, RECORD 3, FIELD 2: expecting MM or THOU (got '" << token << "')\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    olnBoard.SetUnit( unit );

    // RECORD 4:
    //      .END_HEADER
    while( !FetchIDFLine( aBoardFile, iline, isComment, pos ) && aBoardFile.good() );

    if( ( !aBoardFile.good() && !aBoardFile.eof() ) || iline.empty() )
    {
        ERROR_IDF;
        cerr << "problems reading board header, RECORD 4\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: comment within .HEADER section\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    if( !CompareToken( ".END_HEADER", iline ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 file\n";
        cerr << "* Violation of specification: expected .END_HEADER (got '" << iline << "')\n";
        aBoardState = IDF3::FILE_INVALID;
        return false;
    }

    aBoardState = IDF3::FILE_HEADER;
    return true;
}


// read individual board sections; pay attention to IDFv3 section specifications
bool IDF3_BOARD::readBrdSection( std::ifstream& aBoardFile, IDF3::FILE_STATE& aBoardState )
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
    //      .NOTES (NOT YET SUPPORTED: NOTES SECTION WILL BE SKIPPED FOR NOW)
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
            if( aBoardFile.eof() && aBoardState >= IDF3::FILE_HEADER && aBoardState < IDF3::FILE_INVALID )
                return true;

            ERROR_IDF;
            cerr << "problems reading board section\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }

        if( isComment )
        {
            comments.push_back( iline );
            continue;
        }

        // This must be a header
        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF;
            cerr << "problems reading board section\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }

        if( quoted )
        {
            ERROR_IDF;
            cerr << "invalid IDFv3 file\n";
            cerr << "* Violation of specification: quoted string where SECTION HEADER expected\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }

        if( CompareToken( ".BOARD_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_HEADER )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: no HEADER section\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            if( !olnBoard.ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    olnBoard.AddComment( *its );
                    ++its;
                }
            }

            aBoardState = IDF3::FILE_OUTLINE;
            return true;
        }

        if( CompareToken( ".PANEL_OUTLINE", token ) )
        {
            ERROR_IDF;
            cerr << "PANEL_OUTLINE not supported\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }

        if( CompareToken( ".OTHER_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .OTHER_OUTLINE\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            OTHER_OUTLINE* op = new OTHER_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create OTHER_OUTLINE object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the OTHER_OUTLINE section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            if( olnOther.insert( pair<string, OTHER_OUTLINE*>(op->GetOutlineIdentifier(), op) ).second == false )
            {
                ERROR_IDF;
                cerr << "* Violation of specification. Non-unique ID in OTHER_OUTLINE '";
                cerr << op->GetOutlineIdentifier() << "'\n";
                delete op;
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            return true;
        }

        if( CompareToken( ".ROUTE_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .ROUTE_OUTLINE\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            ROUTE_OUTLINE* op = new ROUTE_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create ROUTE_OUTLINE object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the ROUTE_OUTLINE section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            olnRoute.push_back( op );

            return true;
        }

        if( CompareToken( ".PLACE_OUTLINE", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACE_OUTLINE\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            PLACE_OUTLINE* op = new PLACE_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create PLACE_OUTLINE object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the PLACE_OUTLINE section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            olnPlace.push_back( op );

            return true;
        }

        if( CompareToken( ".ROUTE_KEEPOUT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .ROUTE_KEEPOUT\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            ROUTE_KO_OUTLINE* op = new ROUTE_KO_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create ROUTE_KEEPOUT object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the ROUTE_KEEPOUT section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            olnRouteKeepout.push_back( op );

            return true;
        }

        if( CompareToken( ".VIA_KEEPOUT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .VIA_KEEPOUT\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            VIA_KO_OUTLINE* op = new VIA_KO_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create VIA_KEEPOUT object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the VIA_KEEPOUT section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            olnViaKeepout.push_back( op );

            return true;
        }

        if( CompareToken( ".PLACE_KEEPOUT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACE_KEEPOUT\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            PLACE_KO_OUTLINE* op = new PLACE_KO_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create PLACE_KEEPOUT object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the PLACE_KEEPOUT section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            olnPlaceKeepout.push_back( op );

            return true;
        }

        if( CompareToken( ".PLACE_REGION", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACE_REGION\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            GROUP_OUTLINE* op = new GROUP_OUTLINE;

            if( op == NULL )
            {
                ERROR_IDF;
                cerr << "could not create PLACE_REGION object\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

            op->SetUnit( unit );

            if( !op->ReadData( aBoardFile, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading the PLACE_REGION section\n";
                aBoardState = IDF3::FILE_ERROR;
                return false;
            }

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

            olnGroup.insert( pair<string, GROUP_OUTLINE*>(op->GetGroupName(), op) );

            return true;
        }

        if( CompareToken( ".DRILLED_HOLES", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .DRILLED_HOLES\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            if( !readBrdDrills( aBoardFile, aBoardState ) )
            {
                if( !aBoardFile.good() || aBoardState == IDF3::FILE_INVALID )
                {
                    ERROR_IDF << "could not read board DRILLED HOLES section\n";
                    return false;
                }
            }

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    drillComments.push_back( *its );
                    ++its;
                }
            }

            return true;
        }

        if( CompareToken( ".NOTES", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .NOTES\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            if( !readBrdNotes( aBoardFile, aBoardState ) )
            {
                if( !aBoardFile.good() || aBoardState == IDF3::FILE_INVALID )
                {
                    ERROR_IDF << "could not read board NOTES section\n";
                    return false;
                }
            }

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    noteComments.push_back( *its );
                    ++its;
                }
            }

            return true;
        }

        if( CompareToken( ".PLACEMENT", token ) )
        {
            if( aBoardState != IDF3::FILE_OUTLINE )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 file\n";
                cerr << "* Violation of specification: expecting .BOARD_OUTLINE, have .PLACEMENT\n";
                aBoardState = IDF3::FILE_INVALID;
                return false;
            }

            if( !readBrdPlacement( aBoardFile, aBoardState ) )
            {
                if( !aBoardFile.good() || aBoardState == IDF3::FILE_INVALID )
                {
                    ERROR_IDF << "could not read board PLACEMENT section\n";
                    return false;
                }
            }

            if( !comments.empty() )
            {
                std::list<std::string>::iterator its = comments.begin();
                std::list<std::string>::iterator ite = comments.end();

                while( its != ite )
                {
                    placeComments.push_back( *its );
                    ++its;
                }
            }

            return true;
        }
    }   // while( aBoardFile.good()

    if( !aBoardFile.good() )
    {
        if( !aBoardFile.eof() || aBoardState < IDF3::FILE_OUTLINE || aBoardState >= IDF3::FILE_INVALID )
        {
            ERROR_IDF;
            cerr << "problems reading board section\n";
            aBoardState = IDF3::FILE_INVALID;
            return false;
        }
    }

    return true;
}   // readBrdSection()


// read the board file data
bool IDF3_BOARD::readBoardFile( const std::string& aFileName )
{
    std::ifstream brd;

    brd.open( aFileName.c_str(), std::ios_base::in );

    if( !brd.is_open() )
    {
        ERROR_IDF;
        cerr << "could not open file: '" << aFileName << "'\n";
        return false;
    }

    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    IDF3::FILE_STATE state = IDF3::FILE_START;

    // note: as per IDFv3 specification:
    //      "The Header section must be the first section in the file, the second
    //       section must be the Outline section, and the last section must be the
    //       Placement section. All other sections may be in any order."

    // further notes: Except for the HEADER section, sections may be preceeded by
    // comment lines which will be copied back out on write(). No comments may
    // be associated with the board file itself since the only logical location
    // for unambiguous association is at the end of the file, which is inconvenient
    // for large files.

    if( !readBrdHeader( brd, state ) )
    {
        ERROR_IDF;
        cerr << "could not find a valid header\n";
        brd.close();
        return false;
    }

    // read the various sections
    while( readBrdSection( brd, state ) && state != IDF3::FILE_PLACEMENT && !brd.eof() );

    if( state == IDF3::FILE_INVALID )
    {
        brd.close();
        ERROR_IDF;
        cerr << "problems reading file: '" << aFileName << "'\n";
        return false;
    }

    if( !brd.good() )
    {
        // check if we have valid data
        if( brd.eof() && state >= IDF3::FILE_OUTLINE && state < IDF3::FILE_INVALID )
        {
            brd.close();
            return true;
        }

        brd.close();
        ERROR_IDF;
        cerr << "problems reading file: '" << aFileName << "'\n";
        return false;
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
                ERROR_IDF << "invalid IDF3 file\n";
                cerr << "* Violation of specification: non-comment lines after PLACEMENT section\n";
                Clear();
                brd.close();
                return false;
            }
        }
    }

    brd.close();
    return true;
} // readBoardFile()


// read the library sections (outlines)
bool IDF3_BOARD::readLibSection( std::ifstream& aLibFile, IDF3::FILE_STATE& aLibState, IDF3_BOARD* aBoard )
{
    if( aBoard == NULL )
    {
        ERROR_IDF << "BUG: invoked with NULL reference aBoard\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    std::list< std::string > comments;  // comments associated with a section

    // Reads in .ELECTRICAL, .MECHANICAL or #COMMENTS
    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;
    int idx = 0;
    bool quoted = false;
    std::string token;
    IDF3_COMP_OUTLINE *pout = new IDF3_COMP_OUTLINE;

    while( aLibFile.good() )
    {
        while( !FetchIDFLine( aLibFile, iline, isComment, pos ) && aLibFile.good() );

        if( !aLibFile.good() && !aLibFile.eof() )
        {
            ERROR_IDF;
            cerr << "problems reading library section\n";
            aLibState = IDF3::FILE_INVALID;
            return false;
        }

        // no data was read; this only happens at eof()
        if( iline.empty() )
            return true;

        if( isComment )
        {
            comments.push_back( iline );
            continue;
        }

        // This must be a header
        if( !GetIDFString( iline, token, quoted, idx ) )
        {
            ERROR_IDF;
            cerr << "problems reading library section\n";
            aLibState = IDF3::FILE_INVALID;
            return false;
        }

        if( quoted )
        {
            ERROR_IDF;
            cerr << "invalid IDFv3 library\n";
            cerr << "* Violation of specification: quoted string where .ELECTRICAL or .MECHANICAL expected\n";
            aLibState = IDF3::FILE_INVALID;
            return false;
        }

        if( CompareToken( ".ELECTRICAL", token ) || CompareToken( ".MECHANICAL", token ) )
        {
            if( !pout->ReadData( aLibFile, token ) )
            {
                ERROR_IDF;
                cerr << "invalid IDFv3 library [faulty section]\n";
                aLibState = IDF3::FILE_INVALID;
                return false;
            }

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

            if( cop == NULL )
            {
                compOutlines.insert( pair<const std::string, IDF3_COMP_OUTLINE*>( pout->GetUID(), pout ) );
            }
            else
            {
                ERROR_IDF;
                cerr << "duplicate Component Outline: '" << pout->GetUID() << "'\n";
                delete pout;
            }

            return true;
        }
        else
        {
            ERROR_IDF;
            cerr << "invalid IDFv3 library\n";
            cerr << "* Expecting .ELECTRICAL or .MECHANICAL, got '" << token << "'\n";
            aLibState = IDF3::FILE_INVALID;
            return false;
        }

    }

    ERROR_IDF;
    cerr << "problems reading library section\n";
    aLibState = IDF3::FILE_INVALID;
    return false;
}


// read the library HEADER
bool IDF3_BOARD::readLibHeader( std::ifstream& aLibFile, IDF3::FILE_STATE& aLibState )
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
        ERROR_IDF;
        cerr << "problems reading library header\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: first line must be .HEADER\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( !CompareToken( ".HEADER", iline ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: first line must be .HEADER and have no quotes or trailing text\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
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
        ERROR_IDF;
        cerr << "problems reading library header, RECORD 2\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: comment within .HEADER section\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    idx = 0;
    GetIDFString( iline, token, quoted, idx );

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: File Type in HEADER section must not be in quotes\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( !CompareToken( "LIBRARY_FILE", token ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Expecting string: LIBRARY_FILE (got '" << token << "')\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2: no FIELD 2\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: IDF Version must not be in quotes\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( token.compare( "3.0" ) && token.compare( "3." ) && token.compare( "3" ) )
    {
        ERROR_IDF;
        cerr << "unsupported IDF library version\n";
        cerr << "* Expecting version to be one of '3.0', '3.', or '3' (value: '" << token << "')\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2, FIELD 3: no Source System string\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }
    libSource = token;


    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2, FIELD 4: no Date string\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }
    libDate = token;

    if( !GetIDFString( iline, token, quoted, idx ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification for HEADER section, RECORD 2, FIELD 5: no Board File Version number\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }
    std::istringstream istr;
    istr.str( token );

    istr >> libFileVersion;

    if( istr.fail() )
    {
        ERROR_IDF << "invalid Library File Version in header\n";
        cerr << "* Setting default version of 1\n";
        libFileVersion = 1;
    }

    if( quoted )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: HEADER section, RECORD 2, FIELD 5: Library File Version must not be in quotes\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    // RECORD 3:
    //      .END_HEADER
    while( !FetchIDFLine( aLibFile, iline, isComment, pos ) && aLibFile.good() );

    if( ( !aLibFile.good() && !aLibFile.eof() ) || iline.empty() )
    {
        ERROR_IDF;
        cerr << "problems reading library header, RECORD 3\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( isComment )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 library\n";
        cerr << "* Violation of specification: comment within .HEADER section\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    if( !CompareToken( ".END_HEADER", iline ) )
    {
        ERROR_IDF;
        cerr << "invalid IDFv3 header\n";
        cerr << "* Violation of specification: expected .END_HEADER (got '" << iline << "')\n";
        aLibState = IDF3::FILE_INVALID;
        return false;
    }

    aLibState = IDF3::FILE_HEADER;
    return true;
}


// read the library file data
bool IDF3_BOARD::readLibFile( const std::string& aFileName )
{
    std::ifstream lib;

    lib.open( aFileName.c_str(), std::ios_base::in );

    if( !lib.is_open() )
    {
        ERROR_IDF;
        cerr << "could not open file: '" << aFileName << "'\n";
        return false;
    }

    IDF3::FILE_STATE state = IDF3::FILE_START;

    if( !readLibHeader( lib, state ) )
    {
        ERROR_IDF;
        cerr << "[IDF library] could not find a valid header\n";
        lib.close();
        return false;
    }

    // read the library sections
    while( readLibSection( lib, state, this ) && lib.good() );

    if( state <= IDF3::FILE_START || state >= IDF3::FILE_INVALID )
    {
        lib.close();
        ERROR_IDF;
        cerr << "problems reading file: '" << aFileName << "'\n";
        return false;
    }

    lib.close();
    return true;
}


bool IDF3_BOARD::ReadFile( const wxString& aFullFileName )
{
    // 1. Check that the file extension is 'emn'
    // 2. Check if a file with extension 'emp' exists and read it
    // 3. Open the specified filename and read it

    std::string fname = TO_UTF8( aFullFileName );

    wxFileName brdname( aFullFileName );
    wxFileName libname( aFullFileName );

    brdname.SetExt( wxT( "emn" ) );
    libname.SetExt( wxT( "emp" ) );

    std::string bfname = TO_UTF8( aFullFileName );

    if( !brdname.IsOk() )
    {
        ERROR_IDF;
        cerr << "invalid file name: '" << bfname << "'\n";
        return false;
    }

    if( !brdname.IsOk() )
    {
        ERROR_IDF;
        cerr << "invalid file name: '" << bfname << "'\n";
        return false;
    }

    if( !brdname.FileExists() )
    {
        ERROR_IDF;
        cerr << "no such file: '" << bfname  << "'\n";
        return false;
    }

    if( !brdname.IsFileReadable() )
    {
        ERROR_IDF;
        cerr << "cannot read file: '" << bfname << "'\n";
        return false;
    }

    bfname = TO_UTF8( brdname.GetFullPath() );
    std::string lfname = TO_UTF8( libname.GetFullPath() );

    if( !libname.FileExists() )
    {
        ERROR_IDF;
        cerr << "no associated library file (*.emp)\n";
    }
    else if( !libname.IsFileReadable() )
    {
        ERROR_IDF;
        cerr << "cannot read library file: '" << lfname << "'\n";
    }
    else
    {
        // read the library file before proceeding
        if( !readLibFile( lfname ) )
        {
            ERROR_IDF;
            cerr << "problems reading library file: '" << lfname << "'\n";
            return false;
        }
    }

    // read the board file
    if( !readBoardFile( bfname ) )
    {
        Clear();
        ERROR_IDF;
        cerr << "problems reading board file: '" << lfname << "'\n";
        return false;
    }

    return true;
}

// write the library file data
bool IDF3_BOARD::writeLibFile( const std::string& aFileName )
{
    std::ofstream lib;

    lib.open( aFileName.c_str(), std::ios_base::out );

    if( !lib.is_open() )
    {
        ERROR_IDF;
        cerr << "could not open library file: '" << aFileName << "'\n";
        return false;
    }

    wxDateTime tdate( time( NULL ) );

    if( idfSource.empty() )
        idfSource = "KiCad-IDF Framework";

    ostringstream fileDate;
    fileDate << setfill( '0' ) << setw(4) << tdate.GetYear();
    fileDate << "/" << setw(2) << tdate.GetMonth() << "/" << tdate.GetDay();
    fileDate << "." << tdate.GetHour() << ":" << tdate.GetMinute() << ":" << tdate.GetSecond();
    libDate = fileDate.str();

    lib << ".HEADER\n";
    lib << "LIBRARY_FILE 3.0 \"Created by " << idfSource;
    lib << "\" " << libDate << " " << (++libFileVersion) << "\n";
    lib << ".END_HEADER\n\n";

    std::map< std::string, IDF3_COMP_OUTLINE*>::iterator its = compOutlines.begin();
    std::map< std::string, IDF3_COMP_OUTLINE*>::iterator ite = compOutlines.end();

    while( its != ite )
    {
        its->second->WriteData( lib );
        ++its;
    }

    bool ok = !lib.fail();

    lib.close();

    return ok;
}

// write the board file data
bool IDF3_BOARD::writeBoardFile( const std::string& aFileName )
{
    std::ofstream brd;

    brd.open( aFileName.c_str(), std::ios_base::out );

    if( !brd.is_open() )
    {
        ERROR_IDF;
        cerr << "could not open board file: '" << aFileName << "'\n";
        return false;
    }

    wxDateTime tdate( time( NULL ) );

    if( idfSource.empty() )
        idfSource = "KiCad-IDF Framework";

    ostringstream fileDate;
    fileDate << setfill( '0' ) << setw(4) << tdate.GetYear();
    fileDate << "/" << setw(2) << tdate.GetMonth() << "/" << tdate.GetDay();
    fileDate << "." << tdate.GetHour() << ":" << tdate.GetMinute() << ":" << tdate.GetSecond();
    brdDate = fileDate.str();

    brd << ".HEADER\n";
    brd << "BOARD_FILE 3.0 \"Created by " << idfSource;
    brd << "\" " << brdDate << " " << (++brdFileVersion) << "\n";

    if( boardName.empty() )
        brd << "\"BOARD WITH NO NAME\" ";
    else
        brd << "\"" << boardName << "\" ";

    brd << setw(1) << setfill( ' ' );

    if( unit == IDF3::UNIT_MM )
        brd << "MM\n";
    else
        brd << "THOU\n";

    brd << ".END_HEADER\n\n";

    // write the BOARD_OUTLINE
    if( !olnBoard.WriteData( brd ) )
    {
        ERROR_IDF << "problems writing BOARD OUTLINE\n";
        brd.close();
        return false;
    }

    // OTHER outlines
    do
    {
        std::map<std::string, OTHER_OUTLINE*>::iterator its = olnOther.begin();
        std::map<std::string, OTHER_OUTLINE*>::iterator ite = olnOther.end();

        while( (its != ite) && its->second->WriteData( brd ) ) ++its;

    } while( 0 );

    // ROUTE outlines
    do
    {
        std::list<ROUTE_OUTLINE*>::iterator its = olnRoute.begin();
        std::list<ROUTE_OUTLINE*>::iterator ite = olnRoute.end();

        while( (its != ite) && (*its)->WriteData( brd ) ) ++its;

    } while( 0 );

    // PLACEMENT outlines
    do
    {
        std::list<PLACE_OUTLINE*>::iterator its = olnPlace.begin();
        std::list<PLACE_OUTLINE*>::iterator ite = olnPlace.end();

        while( (its != ite) && (*its)->WriteData( brd ) ) ++its;

    } while( 0 );

    // ROUTE KEEPOUT outlines
    do
    {
        std::list<ROUTE_KO_OUTLINE*>::iterator its = olnRouteKeepout.begin();
        std::list<ROUTE_KO_OUTLINE*>::iterator ite = olnRouteKeepout.end();

        while( (its != ite) && (*its)->WriteData( brd ) ) ++its;

    } while( 0 );

    // VIA KEEPOUT outlines
    do
    {
        std::list<VIA_KO_OUTLINE*>::iterator its = olnViaKeepout.begin();
        std::list<VIA_KO_OUTLINE*>::iterator ite = olnViaKeepout.end();

        while( (its != ite) && (*its)->WriteData( brd ) ) ++its;

    } while( 0 );

    // PLACE KEEPOUT outlines
    do
    {
        std::list<PLACE_KO_OUTLINE*>::iterator its = olnPlaceKeepout.begin();
        std::list<PLACE_KO_OUTLINE*>::iterator ite = olnPlaceKeepout.end();

        while( (its != ite) && (*its)->WriteData( brd ) ) ++its;

    } while( 0 );

    // PLACEMENT GROUP outlines
    do
    {
        std::map<std::string, GROUP_OUTLINE*>::iterator its = olnGroup.begin();
        std::map<std::string, GROUP_OUTLINE*>::iterator ite = olnGroup.end();

        while( (its != ite) && its->second->WriteData( brd ) ) ++its;

    } while( 0 );

    // Drilled holes
    do
    {
        std::list<std::string>::iterator itds = drillComments.begin();
        std::list<std::string>::iterator itde = drillComments.end();

        while( itds != itde )
        {
            brd << "# " << *itds << "\n";
            ++itds;
        }

        brd << ".DRILLED_HOLES\n";

        std::list<IDF_DRILL_DATA*>::iterator itbs = board_drills.begin();
        std::list<IDF_DRILL_DATA*>::iterator itbe = board_drills.end();

        while( itbs != itbe )
        {
            (*itbs)->Write( brd, unit );
            ++itbs;
        }

        std::map< std::string, IDF3_COMPONENT*>::iterator itcs = components.begin();
        std::map< std::string, IDF3_COMPONENT*>::iterator itce = components.end();

        while( itcs != itce )
        {
            itcs->second->WriteDrillData( brd );
            ++itcs;
        }

        brd << ".END_DRILLED_HOLES\n\n";
    } while( 0 );

    // Notes
    if( !notes.empty() )
    {
        std::list<std::string>::iterator itncs = noteComments.begin();
        std::list<std::string>::iterator itnce = noteComments.end();

        while( itncs != itnce )
        {
            brd << "# " << *itncs << "\n";
            ++itncs;
        }

        brd << ".NOTES\n";

        std::list<IDF_NOTE*>::iterator itns = notes.begin();
        std::list<IDF_NOTE*>::iterator itne = notes.end();

        while( itns != itne )
        {
            (*itns)->WriteNote( brd, unit );
            ++itns;
        }

        brd << ".END_NOTES\n\n";

    }

    // Placement
    if( !components.empty() )
    {
        std::list<std::string>::iterator itpcs = placeComments.begin();
        std::list<std::string>::iterator itpce = placeComments.end();

        while( itpcs != itpce )
        {
            brd << "# " << *itpcs << "\n";
            ++itpcs;
        }

        std::map< std::string, IDF3_COMPONENT*>::iterator itcs = components.begin();
        std::map< std::string, IDF3_COMPONENT*>::iterator itce = components.end();

        brd << ".PLACEMENT\n";

        while( itcs != itce )
        {
            itcs->second->WritePlaceData( brd );
            ++itcs;
        }

        brd << ".END_PLACEMENT\n";
    }

    bool ok = !brd.fail();
    brd.close();

    return ok;
}

bool IDF3_BOARD::WriteFile( const wxString& aFullFileName, bool aUnitMM, bool aForceUnitFlag )
{
    if( aUnitMM == IDF3::UNIT_MM )
        setUnit( IDF3::UNIT_MM, aForceUnitFlag );
    else
        setUnit( IDF3::UNIT_THOU, aForceUnitFlag );

    // 1. Check that the file extension is 'emn'
    // 2. Write the *.emn file according to the IDFv3 spec
    // 3. Write the *.emp file according to the IDFv3 spec

    std::string fname = TO_UTF8( aFullFileName );

    wxFileName brdname( aFullFileName );
    wxFileName libname( aFullFileName );

    brdname.SetExt( wxT( "emn" ) );
    libname.SetExt( wxT( "emp" ) );

    std::string bfname = TO_UTF8( aFullFileName );

    if( !brdname.IsOk() )
    {
        ERROR_IDF;
        cerr << "invalid file name: '" << bfname << "'\n";
        errno = EINVAL;
        return false;
    }

    if( brdname.FileExists() && !brdname.IsFileWritable() )
    {
        ERROR_IDF;
        cerr << "cannot overwrite existing board file\n";
        cerr << "* Filename: '" << bfname << "'\n";
        errno = EACCES;
        return false;
    }

    bfname = TO_UTF8( brdname.GetFullPath() );
    std::string lfname = TO_UTF8( libname.GetFullPath() );

    if( libname.FileExists() && !libname.IsFileWritable() )
    {
        ERROR_IDF;
        cerr << "cannot overwrite existing library file\n";
        cerr << "* Filename: '" << lfname << "'\n";
        errno = EACCES;
        return false;
    }

    if( !writeLibFile( lfname ) )
    {
        ERROR_IDF;
        cerr << "problems writing library file: '" << lfname << "'\n";
        return false;
    }

    if( !writeBoardFile( bfname ) )
    {
        ERROR_IDF;
        cerr << "problems writing board file: '" << bfname << "'\n";
        return false;
    }

    return true;
}


const std::string& IDF3_BOARD::GetIDFSource( void )
{
    return idfSource;
}


void  IDF3_BOARD::SetIDFSource( const std::string& aIDFSource )
{
    idfSource = aIDFSource;
    return;
}

const std::string& IDF3_BOARD::GetBoardSource( void )
{
    return brdSource;
}

const std::string& IDF3_BOARD::GetLibrarySource( void )
{
    return libSource;
}

const std::string& IDF3_BOARD::GetBoardDate( void )
{
    return brdDate;
}

const std::string& IDF3_BOARD::GetLibraryDate( void )
{
    return libDate;
}

int   IDF3_BOARD::GetBoardVersion( void )
{
    return brdFileVersion;
}

bool  IDF3_BOARD::SetBoardVersion( int aVersion )
{
    if( aVersion < 0 )
        return false;

    brdFileVersion = aVersion;

    return true;
}


int   IDF3_BOARD::GetLibraryVersion( void )
{
    return libFileVersion;
}


bool  IDF3_BOARD::SetLibraryVersion( int aVersion )
{
    if( aVersion < 0 )
        return false;

    libFileVersion = aVersion;

    return true;
}


double IDF3_BOARD::GetUserScale( void )
{
    return userScale;
}


bool IDF3_BOARD::SetUserScale( double aScaleFactor )
{
    if( aScaleFactor <= 0.0 )
    {
        ERROR_IDF << "user scale factor must be > 0\n";
        return false;
    }

    userScale = aScaleFactor;
    return true;
}

int IDF3_BOARD::GetUserPrecision( void )
{
    return userPrec;
}

bool IDF3_BOARD::SetUserPrecision( int aPrecision )
{
    if( aPrecision < 0 || aPrecision > 8 )
        return false;

    userPrec = aPrecision;
    return true;
}


void IDF3_BOARD::GetUserOffset( double& aXoff, double& aYoff )
{
    aXoff = userXoff;
    aYoff = userYoff;
    return;
}


void IDF3_BOARD::SetUserOffset( double aXoff, double aYoff )
{
    userXoff = aXoff;
    userYoff = aYoff;
    return;
}


bool IDF3_BOARD::AddBoardOutline( IDF_OUTLINE* aOutline )
{
    return olnBoard.AddOutline( aOutline );
}


bool IDF3_BOARD::DelBoardOutline( IDF_OUTLINE* aOutline )
{
    return olnBoard.DelOutline( aOutline );
}


bool IDF3_BOARD::DelBoardOutline( size_t aIndex )
{
    return olnBoard.DelOutline( aIndex );
}


size_t IDF3_BOARD::GetBoardOutlinesSize( void )
{
    return olnBoard.OutlinesSize();
}


BOARD_OUTLINE* IDF3_BOARD::GetBoardOutline( void )
{
    return &olnBoard;
}


const std::list< IDF_OUTLINE* >*const IDF3_BOARD::GetBoardOutlines( void )
{
    return olnBoard.GetOutlines();
}


IDF_DRILL_DATA* IDF3_BOARD::AddBoardDrill( double aDia, double aXpos, double aYpos,
                                   IDF3::KEY_PLATING aPlating,
                                   const std::string aHoleType,
                                   IDF3::KEY_OWNER aOwner )
{
    IDF_DRILL_DATA* drill = new IDF_DRILL_DATA( aDia, aXpos, aYpos, aPlating,
                                                "BOARD", aHoleType, aOwner );

    if( drill != NULL )
        board_drills.push_back( drill );

    return drill;
}

IDF_DRILL_DATA* IDF3_BOARD::AddDrill( IDF_DRILL_DATA* aDrilledHole )
{
    if( !aDrilledHole )
        return NULL;

    if( CompareToken( "PANEL", aDrilledHole->GetDrillRefDes() ) )
    {
        ERROR_IDF;
        cerr << "\n*BUG: PANEL drilled holes are not supported\n";
        return NULL;
    }

    if( CompareToken( "BOARD", aDrilledHole->GetDrillRefDes() ) )
    {
        board_drills.push_back( aDrilledHole );
        return aDrilledHole;
    }

    return addCompDrill( aDrilledHole );
}


bool IDF3_BOARD::DelBoardDrill( double aDia, double aXpos, double aYpos )
{
    std::list<IDF_DRILL_DATA*>::iterator sp = board_drills.begin();
    std::list<IDF_DRILL_DATA*>::iterator ep = board_drills.end();
    bool rval = false;

    while( sp != ep )
    {
        if( (*sp)->Matches( aDia, aXpos, aYpos ) )
        {
            rval = true;
            delete *sp;
            sp = board_drills.erase( sp );
            continue;
        }
        ++sp;
    }

    return rval;
}


// a slot is a deficient representation of a kicad slotted hole;
// it is usually associated with a component but IDFv3 does not
// provide for such an association.
bool IDF3_BOARD::AddSlot( double aWidth, double aLength, double aOrientation, double aX, double aY )
{
    if( aWidth < IDF_MIN_DIA_MM )
        return true;

    IDF_POINT c[2];     // centers
    IDF_POINT pt[4];

    // make sure the user isn't giving us dud information
    if( aLength < aWidth )
        std::swap( aLength, aWidth );

    if( aLength == aWidth )
    {
        ERROR_IDF;
        cerr << "length == width (" << aWidth << ")\n";
        return false;
    }

    double a1 = aOrientation / 180.0 * M_PI;
    double a2 = a1 + M_PI2;
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

    IDF_OUTLINE* outline = new IDF_OUTLINE;

    if( outline == NULL )
    {
        ERROR_IDF;
        cerr << "could not create an outline object\n";
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

    return AddBoardOutline( outline );
}


IDF_DRILL_DATA* IDF3_BOARD::addCompDrill( double aDia, double aXpos, double aYpos,
                                  IDF3::KEY_PLATING aPlating,
                                  const std::string aHoleType,
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
        ERROR_IDF;
        cerr << "PANEL data not supported\n";
        return NULL;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator ref = components.find( refdes );

    if( ref == components.end() )
    {
        // create the item
        IDF3_COMPONENT* comp = new IDF3_COMPONENT( this );

        if( comp == NULL )
        {
            ERROR_IDF;
            cerr << "could not create new component object\n";
            return NULL;
        }

        comp->SetParent( this );
        comp->SetRefDes( refdes );
        ref = components.insert( std::pair< std::string, IDF3_COMPONENT*> ( comp->GetRefDes(), comp ) ).first;
    }

    // add the drill
    return ref->second->AddDrill( aDia, aXpos, aYpos, aPlating, aHoleType, aOwner );
}


IDF_DRILL_DATA* IDF3_BOARD::addCompDrill( IDF_DRILL_DATA* aDrilledHole )
{
    if( !aDrilledHole )
        return NULL;

    if( CompareToken( "PANEL", aDrilledHole->GetDrillRefDes() ) )
    {
        ERROR_IDF;
        cerr << "PANEL data not supported\n";
        return NULL;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator ref = components.find( aDrilledHole->GetDrillRefDes() );

    if( ref == components.end() )
    {
        // create the item
        IDF3_COMPONENT* comp = new IDF3_COMPONENT( this );

        if( comp == NULL )
        {
            ERROR_IDF;
            cerr << "could not create new component object\n";
            return NULL;
        }

        comp->SetParent( this );
        comp->SetRefDes( aDrilledHole->GetDrillRefDes() );
        ref = components.insert( std::pair< std::string, IDF3_COMPONENT*> ( comp->GetRefDes(), comp ) ).first;
    }

    // add the drill
    return ref->second->AddDrill( aDrilledHole );
}


bool IDF3_BOARD::delCompDrill( double aDia, double aXpos, double aYpos, std::string aRefDes )
{
    std::map<std::string, IDF3_COMPONENT*>::iterator ref = components.find( aRefDes );

    if( ref == components.end() )
        return false;

    return ref->second->DelDrill( aDia, aXpos, aYpos );
}


bool IDF3_BOARD::AddComponent( IDF3_COMPONENT* aComponent )
{
    if( !aComponent )
    {
        ERROR_IDF << "Invalid component pointer (NULL)\n";
        return false;
    }

    if( components.insert( std::pair<std::string, IDF3_COMPONENT*>
        ( aComponent->GetRefDes(), aComponent ) ).second == false )
    {
        ERROR_IDF << "Duplicate RefDes ('" << aComponent->GetRefDes() << "')\n";
        return false;
    }

    return true;
}


bool IDF3_BOARD::DelComponent( IDF3_COMPONENT* aComponent )
{
    if( !aComponent )
    {
        ERROR_IDF << "Invalid component pointer (NULL)\n";
        return false;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator it =
        components.find( aComponent->GetRefDes() );

    if( it == components.end() )
        return false;

    delete it->second;
    components.erase( it );

    return true;
}


bool IDF3_BOARD::DelComponent( size_t aIndex )
{
    if( aIndex >= components.size() )
    {
        ERROR_IDF << "index (" << aIndex << ") >= components size ("
            << components.size() << ")\n";
            return false;
    }

    std::map<std::string, IDF3_COMPONENT*>::iterator it = components.begin();

    while( aIndex-- > 0 ) ++it;

    delete it->second;
    components.erase( it );

    return false;
}


size_t IDF3_BOARD::GetComponentsSize( void )
{
    return components.size();
}


std::map< std::string, IDF3_COMPONENT* >*const IDF3_BOARD::GetComponents( void )
{
    return &components;
}


IDF3_COMPONENT* IDF3_BOARD::FindComponent( std::string aRefDes )
{
    std::map<std::string, IDF3_COMPONENT*>::iterator it = components.find( aRefDes );

    if( it == components.end() )
        return NULL;

    return it->second;
}


// returns a pointer to a component outline object or NULL
// if the object doesn't exist
IDF3_COMP_OUTLINE* IDF3_BOARD::GetComponentOutline( const std::string aGeomName,
                                                    const std::string aPartName,
                                                    wxString aFullFileName )
{
    std::ostringstream ostr;
    ostr << aGeomName << "_" << aPartName;

    IDF3_COMP_OUTLINE* cp = GetComponentOutline( ostr.str() );

    if( cp != NULL )
        return cp;

    std::string fname = TO_UTF8( aFullFileName );

    cp = new IDF3_COMP_OUTLINE;

    if( cp == NULL )
    {
        ERROR_IDF;
        cerr << "failed to create outline with UID '" << aGeomName << "_";
        cerr << aPartName << "'\n";
        cerr << "* filename: '" << fname << "'\n";
        return NULL;
    }

    wxFileName idflib( aFullFileName );

    if( !idflib.IsOk() )
    {
        ERROR_IDF;
        cerr << "invalid file name: '" << fname << "'\n";
        delete cp;
        return NULL;
    }

    if( !idflib.FileExists() )
    {
        ERROR_IDF;
        cerr << "no such file: '" << fname  << "'\n";
        delete cp;
        return NULL;
    }

    if( !idflib.IsFileReadable() )
    {
        ERROR_IDF;
        cerr << "cannot read file: '" << fname << "'\n";
        delete cp;
        return NULL;
    }

    std::ifstream model;

    model.open( fname.c_str(), std::ios_base::in );

    if( !model.is_open() )
    {
        ERROR_IDF;
        cerr << "could not open file: '" << fname << "'\n";
        delete cp;
        return NULL;
    }

    std::string iline;      // the input line
    bool isComment;         // true if a line just read in is a comment line
    std::streampos pos;

    while( true )
    {
        while( !FetchIDFLine( model, iline, isComment, pos ) && model.good() );

        if( !model.good() )
        {
            ERROR_IDF;
            cerr << "problems reading file: '" << fname << "'\n";
            delete cp;
            model.close();
            return NULL;
        }

        // accept comment lines, .ELECTRICAL, or .MECHANICAL only
        if( isComment )
        {
            cp->AddComment( iline );
            continue;
        }

        if( CompareToken( ".ELECTRICAL", iline ) || CompareToken( ".MECHANICAL", iline ) )
        {
            if( !cp->ReadData( model, iline ) )
            {
                ERROR_IDF;
                cerr << "problems reading file: '" << fname << "'\n";
                delete cp;
                model.close();
                return NULL;
            }
            else
            {
                break;
            }
        }
        else
        {
            ERROR_IDF << "faulty IDF component definition\n";
            cerr << "* Expecting .ELECTRICAL or .MECHANICAL, got '" << iline << "'\n";
            cerr << "* File: '" << fname << "'\n";
            delete cp;
            model.close();
            return NULL;
        }
    }   // while( true )

    model.close();

    return cp;
}


// returns a pointer to the component outline object with the
// unique ID aComponentID
IDF3_COMP_OUTLINE* IDF3_BOARD::GetComponentOutline( std::string aComponentID )
{
    std::map< std::string, IDF3_COMP_OUTLINE*>::iterator its = compOutlines.find( aComponentID );

    if( its != compOutlines.end() )
        return its->second;

    return NULL;
}


// returns a pointer to the outline which is substituted
// whenever a true outline cannot be found or is defective
IDF3_COMP_OUTLINE* IDF3_BOARD::GetInvalidOutline( const std::string& aGeomName, const std::string& aPartName )
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

    if( cp != NULL )
        return cp;

    cp = new IDF3_COMP_OUTLINE;

    if( cp == NULL )
    {
        ERROR_IDF << "could not create new outline\n";
        return NULL;
    }

    if( empty )
        cp->CreateDefaultOutline( "", "" );
    else
        cp->CreateDefaultOutline( aGeomName, aPartName );

    compOutlines.insert( pair<const std::string, IDF3_COMP_OUTLINE*>(cp->GetUID(), cp) );

    return cp;
}


// clears all data
void IDF3_BOARD::Clear( void )
{
    // preserve the board thickness
    double thickness = olnBoard.GetThickness();

    idfSource.clear();
    brdSource.clear();
    libSource.clear();
    brdDate.clear();
    libDate.clear();
    brdFileVersion = 0;
    libFileVersion = 0;

    // delete comment lists
    noteComments.clear();
    drillComments.clear();
    placeComments.clear();

    // delete notes
    while( !notes.empty() )
    {
        delete notes.front();
        notes.pop_front();
    }

    // delete drill list
    do
    {
        std::list<IDF_DRILL_DATA*>::iterator ds = board_drills.begin();
        std::list<IDF_DRILL_DATA*>::iterator de = board_drills.end();

        while( ds != de )
        {
            delete *ds;
            ++ds;
        }

        board_drills.clear();
    } while(0);


    // delete components
    do
    {
        std::map<std::string, IDF3_COMPONENT*>::iterator cs = components.begin();
        std::map<std::string, IDF3_COMPONENT*>::iterator ce = components.end();

        while( cs != ce )
        {
            delete cs->second;
            ++cs;
        }

        components.clear();
    } while(0);


    // delete component outlines
    do
    {
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator cs = compOutlines.begin();
        std::map<std::string, IDF3_COMP_OUTLINE*>::iterator ce = compOutlines.end();

        while( cs != ce )
        {
            delete cs->second;
            ++cs;
        }

        compOutlines.clear();
    } while(0);


    // delete OTHER outlines
    do
    {
        std::map<std::string, OTHER_OUTLINE*>::iterator os = olnOther.begin();
        std::map<std::string, OTHER_OUTLINE*>::iterator oe = olnOther.end();

        while( os != oe )
        {
            delete os->second;
            ++os;
        }

        olnOther.clear();
    } while(0);


    // delete ROUTE outlines
    do
    {
        std::list<ROUTE_OUTLINE*>::iterator os = olnRoute.begin();
        std::list<ROUTE_OUTLINE*>::iterator oe = olnRoute.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        olnRoute.clear();
    } while(0);


    // delete PLACE outlines
    do
    {
        std::list<PLACE_OUTLINE*>::iterator os = olnPlace.begin();
        std::list<PLACE_OUTLINE*>::iterator oe = olnPlace.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        olnPlace.clear();
    } while(0);


    // delete ROUTE KEEPOUT outlines
    do
    {
        std::list<ROUTE_KO_OUTLINE*>::iterator os = olnRouteKeepout.begin();
        std::list<ROUTE_KO_OUTLINE*>::iterator oe = olnRouteKeepout.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        olnRouteKeepout.clear();
    } while(0);


    // delete VIA KEEPOUT outlines
    do
    {
        std::list<VIA_KO_OUTLINE*>::iterator os = olnViaKeepout.begin();
        std::list<VIA_KO_OUTLINE*>::iterator oe = olnViaKeepout.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        olnViaKeepout.clear();
    } while(0);


    // delete PLACEMENT KEEPOUT outlines
    do
    {
        std::list<PLACE_KO_OUTLINE*>::iterator os = olnPlaceKeepout.begin();
        std::list<PLACE_KO_OUTLINE*>::iterator oe = olnPlaceKeepout.end();

        while( os != oe )
        {
            delete *os;
            ++os;
        }

        olnPlaceKeepout.clear();
    } while(0);


    // delete PLACEMENT GROUP outlines
    do
    {
        std::map<std::string, GROUP_OUTLINE*>::iterator os = olnGroup.begin();
        std::map<std::string, GROUP_OUTLINE*>::iterator oe = olnGroup.end();

        while( os != oe )
        {
            delete os->second;
            ++os;
        }

        olnGroup.clear();
    } while(0);

    boardName.clear();
    olnBoard.SetThickness( thickness );

    state     = FILE_START;
    unit      = UNIT_MM;
    userScale = 1.0;
    userXoff  = 0.0;
    userYoff  = 0.0;

    return;
}
