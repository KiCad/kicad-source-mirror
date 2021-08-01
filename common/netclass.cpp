/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2009-2020 KiCad Developers, see change_log.txt for contributors.
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

#include <netclass.h>
#include <macros.h>
#include <base_units.h>

// This will get mapped to "kicad_default" in the specctra_export.
const char NETCLASS::Default[] = "Default";

// Initial values for netclass initialization
const int DEFAULT_CLEARANCE        = PcbMm2iu( 0.2 ); // track to track and track to pads clearance
const int DEFAULT_VIA_DIAMETER     = PcbMm2iu( 0.8 );
const int DEFAULT_VIA_DRILL        = PcbMm2iu( 0.4 );
const int DEFAULT_UVIA_DIAMETER    = PcbMm2iu( 0.3 );
const int DEFAULT_UVIA_DRILL       = PcbMm2iu( 0.1 );
const int DEFAULT_TRACK_WIDTH      = PcbMm2iu( 0.25 );
const int DEFAULT_DIFF_PAIR_WIDTH  = PcbMm2iu( 0.2 );
const int DEFAULT_DIFF_PAIR_GAP    = PcbMm2iu( 0.25 );
const int DEFAULT_DIFF_PAIR_VIAGAP = PcbMm2iu( 0.25 );

const int DEFAULT_WIRE_WIDTH       = SchMils2iu( 6 );
const int DEFAULT_BUS_WIDTH        = SchMils2iu( 12 );

const int DEFAULT_LINE_STYLE = 0; // solid


NETCLASS::NETCLASS( const wxString& aName ) :
    m_Name( aName ),
    m_PcbColor( KIGFX::COLOR4D::UNSPECIFIED )
{
    // Default settings
    SetClearance( DEFAULT_CLEARANCE );
    SetViaDrill( DEFAULT_VIA_DRILL );
    SetuViaDrill( DEFAULT_UVIA_DRILL );
    // These defaults will be overwritten by SetParams,
    // from the board design parameters, later
    SetTrackWidth( DEFAULT_TRACK_WIDTH );
    SetViaDiameter( DEFAULT_VIA_DIAMETER );
    SetuViaDiameter( DEFAULT_UVIA_DIAMETER );
    SetDiffPairWidth( DEFAULT_DIFF_PAIR_WIDTH );
    SetDiffPairGap( DEFAULT_DIFF_PAIR_GAP );
    SetDiffPairViaGap( DEFAULT_DIFF_PAIR_VIAGAP );

    SetWireWidth( DEFAULT_WIRE_WIDTH );
    SetBusWidth( DEFAULT_BUS_WIDTH );
    SetSchematicColor( COLOR4D::UNSPECIFIED );
    SetLineStyle( DEFAULT_LINE_STYLE );
}


NETCLASS::~NETCLASS()
{
}


NETCLASSES::NETCLASSES()
{
    m_default = std::make_shared<NETCLASS>( NETCLASS::Default );
}


NETCLASSES::~NETCLASSES()
{
}


bool NETCLASSES::Add( const NETCLASSPTR& aNetClass )
{
    const wxString& name = aNetClass->GetName();

    if( name == NETCLASS::Default )
    {
        m_default = aNetClass;
        return true;
    }

    // Test for an existing netclass:
    if( !Find( name ) )
    {
        // name not found, take ownership
        m_NetClasses[name] = aNetClass;

        return true;
    }
    else
    {
        // name already exists
        // do not "take ownership" and return false telling caller such.
        return false;
    }
}


NETCLASSPTR NETCLASSES::Remove( const wxString& aNetName )
{
    NETCLASS_MAP::iterator found = m_NetClasses.find( aNetName );

    if( found != m_NetClasses.end() )
    {
        std::shared_ptr<NETCLASS> netclass = found->second;
        m_NetClasses.erase( found );
        return netclass;
    }

    return NETCLASSPTR();
}


NETCLASSPTR NETCLASSES::Find( const wxString& aName ) const
{
    if( aName == NETCLASS::Default )
        return GetDefault();

    NETCLASS_MAP::const_iterator found = m_NetClasses.find( aName );

    if( found == m_NetClasses.end() )
        return NETCLASSPTR();
    else
        return found->second;
}


#if defined(DEBUG)

void NETCLASS::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    //NestedSpace( nestLevel, os )

    os << '<' << GetClass().Lower().mb_str() << ">\n";

    for( const_iterator i = begin();  i!=end();  ++i )
    {
        // NestedSpace( nestLevel+1, os ) << *i;
        os << TO_UTF8( *i );
    }

    // NestedSpace( nestLevel, os )
    os << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
