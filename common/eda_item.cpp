/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>

#include <bitmaps.h>
#include <eda_item.h>
#include <eda_rect.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <i18n_utility.h>


static const unsigned char dummy_png[] = {
 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0xf3, 0xff,
 0x61, 0x00, 0x00, 0x00, 0x5f, 0x49, 0x44, 0x41, 0x54, 0x38, 0xcb, 0x63, 0xf8, 0xff, 0xff, 0x3f,
 0x03, 0x25, 0x98, 0x61, 0x68, 0x1a, 0x00, 0x04, 0x46, 0x40, 0xfc, 0x02, 0x88, 0x45, 0x41, 0x1c,
 0x76, 0x20, 0xfe, 0x01, 0xc4, 0xbe, 0x24, 0x18, 0x60, 0x01, 0xc4, 0x20, 0x86, 0x04, 0x88, 0xc3,
 0x01, 0xe5, 0x04, 0x0c, 0xb8, 0x01, 0x37, 0x81, 0xf8, 0x04, 0x91, 0xf8, 0x0a, 0x54, 0x8f, 0x06,
 0xb2, 0x01, 0x9b, 0x81, 0x78, 0x02, 0x91, 0x78, 0x05, 0x54, 0x8f, 0xca, 0xe0, 0x08, 0x03, 0x36,
 0xa8, 0xbf, 0xec, 0xc8, 0x32, 0x80, 0xcc, 0x84, 0x04, 0x0a, 0xbc, 0x1d, 0x40, 0x2c, 0xc8, 0x30,
 0xf4, 0x33, 0x13, 0x00, 0x6b, 0x1a, 0x46, 0x7b, 0x68, 0xe7, 0x0f, 0x0b, 0x00, 0x00, 0x00, 0x00,
 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static const BITMAP_OPAQUE dummy_xpm[1] = {{ dummy_png, sizeof( dummy_png ), "dummy_xpm" }};


EDA_ITEM::EDA_ITEM( EDA_ITEM* parent, KICAD_T idType ) :
        m_status( 0 ),
        m_parent( parent ),
        m_forceVisible( false ),
        m_flags( 0 ),
        m_structType( idType )
{ }


EDA_ITEM::EDA_ITEM( KICAD_T idType ) :
        m_status( 0 ),
        m_parent( nullptr ),
        m_forceVisible( false ),
        m_flags( 0 ),
        m_structType( idType )
{ }


EDA_ITEM::EDA_ITEM( const EDA_ITEM& base ) :
        m_Uuid( base.m_Uuid ),
        m_status( base.m_status ),
        m_parent( base.m_parent ),
        m_forceVisible( base.m_forceVisible ),
        m_flags( base.m_flags ),
        m_structType( base.m_structType )
{ }


void EDA_ITEM::SetModified()
{
    SetFlags( IS_CHANGED );

    // If this a child object, then the parent modification state also needs to be set.
    if( m_parent )
        m_parent->SetModified();
}


const EDA_RECT EDA_ITEM::GetBoundingBox() const
{
    // return a zero-sized box per default. derived classes should override
    // this
    return EDA_RECT( wxPoint( 0, 0 ), wxSize( 0, 0 ) );
}


EDA_ITEM* EDA_ITEM::Clone() const
{
    wxCHECK_MSG( false, NULL, wxT( "Clone not implemented in derived class " ) + GetClass() +
                 wxT( ".  Bad programmer!" ) );
}


// see base_struct.h
// many classes inherit this method, be careful:
//TODO (snh): Fix this to use std::set instead of C-style vector
SEARCH_RESULT EDA_ITEM::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    if( IsType( scanTypes ) )
    {
        if( SEARCH_RESULT::QUIT == inspector( this, testData ) )
            return SEARCH_RESULT::QUIT;
    }

    return SEARCH_RESULT::CONTINUE;
}


wxString EDA_ITEM::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    wxFAIL_MSG( wxT( "GetSelectMenuText() was not overridden for schematic item type " ) +
                GetClass() );

    return wxString( wxT( "Undefined menu text for " ) + GetClass() );
}


bool EDA_ITEM::Matches( const wxString& aText, const wxFindReplaceData& aSearchData ) const
{
    wxString text = aText;
    wxString searchText = aSearchData.GetFindString();

    // Don't match if searching for replaceable item and the item doesn't support text replace.
    if( (aSearchData.GetFlags() & FR_SEARCH_REPLACE) && !IsReplaceable() )
        return false;

    if( aSearchData.GetFlags() & wxFR_WHOLEWORD )
        return aText.IsSameAs( searchText, aSearchData.GetFlags() & wxFR_MATCHCASE );

    if( aSearchData.GetFlags() & FR_MATCH_WILDCARD )
    {
        if( aSearchData.GetFlags() & wxFR_MATCHCASE )
            return text.Matches( searchText );

        return text.MakeUpper().Matches( searchText.MakeUpper() );
    }

    if( aSearchData.GetFlags() & wxFR_MATCHCASE )
        return aText.Find( searchText ) != wxNOT_FOUND;

    return text.MakeUpper().Find( searchText.MakeUpper() ) != wxNOT_FOUND;
}


bool EDA_ITEM::Replace( const wxFindReplaceData& aSearchData, wxString& aText )
{
    wxString searchString = (aSearchData.GetFlags() & wxFR_MATCHCASE) ? aText : aText.Upper();

    int result = searchString.Find( (aSearchData.GetFlags() & wxFR_MATCHCASE) ?
                                    aSearchData.GetFindString() :
                                    aSearchData.GetFindString().Upper() );

    if( result == wxNOT_FOUND )
        return false;

    wxString prefix = aText.Left( result );
    wxString suffix;

    if( aSearchData.GetFindString().length() + result < aText.length() )
        suffix = aText.Right( aText.length() - ( aSearchData.GetFindString().length() + result ) );

    wxLogTrace( traceFindReplace, wxT( "Replacing '%s', prefix '%s', replace '%s', suffix '%s'." ),
                aText, prefix, aSearchData.GetReplaceString(), suffix );

    aText = prefix + aSearchData.GetReplaceString() + suffix;

    return true;
}


bool EDA_ITEM::operator<( const EDA_ITEM& aItem ) const
{
    wxFAIL_MSG( wxString::Format( wxT( "Less than operator not defined for item type %s." ),
                                 GetClass() ) );

    return false;
}

EDA_ITEM& EDA_ITEM::operator=( const EDA_ITEM& aItem )
{
    // do not call initVars()

    m_structType = aItem.m_structType;
    m_flags      = aItem.m_flags;
    m_status     = aItem.m_status;
    m_parent     = aItem.m_parent;
    m_forceVisible = aItem.m_forceVisible;

    return *this;
}

const BOX2I EDA_ITEM::ViewBBox() const
{
    // Basic fallback
    EDA_RECT bbox = GetBoundingBox();

    return BOX2I( bbox.GetOrigin(), bbox.GetSize() );
}


void EDA_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount      = 1;
    aLayers[0]  = 0;
}

BITMAP_DEF EDA_ITEM::GetMenuImage() const
{
    return dummy_xpm;
}

#if defined(DEBUG)

void EDA_ITEM::ShowDummy( std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    os << '<' << s.Lower().mb_str() << ">"
       << " Need ::Show() override for this class "
       << "</" << s.Lower().mb_str() << ">\n";
}


std::ostream& EDA_ITEM::NestedSpace( int nestLevel, std::ostream& os )
{
    for( int i = 0; i<nestLevel; ++i )
        os << "  ";

    // number of spaces here controls indent per nest level

    return os;
}

#endif


static struct EDA_ITEM_DESC
{
    EDA_ITEM_DESC()
    {
        ENUM_MAP<KICAD_T>::Instance()
            .Undefined( TYPE_NOT_INIT )
            .Map( NOT_USED,             wxT( "<not used>" ) )
            .Map( SCREEN_T,             _HKI( "Screen" ) )

            .Map( PCB_FOOTPRINT_T,      _HKI( "Footprint" ) )
            .Map( PCB_PAD_T,            _HKI( "Pad" ) )
            .Map( PCB_SHAPE_T,          _HKI( "Graphic Shape" ) )
            .Map( PCB_TEXT_T,           _HKI( "Board Text" ) )
            .Map( PCB_FP_TEXT_T,        _HKI( "Footprint Text" ) )
            .Map( PCB_FP_SHAPE_T,       _HKI( "Graphic Shape" ) )
            .Map( PCB_FP_ZONE_T,        _HKI( "Zone" ) )
            .Map( PCB_TRACE_T,          _HKI( "Track" ) )
            .Map( PCB_VIA_T,            _HKI( "Via" ) )
            .Map( PCB_MARKER_T,         _HKI( "Board Marker" ) )
            .Map( PCB_DIM_ALIGNED_T,    _HKI( "Aligned Dimension" ) )
            .Map( PCB_DIM_ORTHOGONAL_T, _HKI( "Orthogonal Dimension" ) )
            .Map( PCB_DIM_CENTER_T,     _HKI( "Center Dimension" ) )
            .Map( PCB_DIM_LEADER_T,     _HKI( "Leader" ) )
            .Map( PCB_TARGET_T,         _HKI( "Target" ) )
            .Map( PCB_ZONE_T,           _HKI( "Zone" ) )
            .Map( PCB_ITEM_LIST_T,      _HKI( "Item List" ) )
            .Map( PCB_NETINFO_T,        _HKI( "Net Info" ) )
            .Map( PCB_GROUP_T,          _HKI( "Group" ) )

            .Map( SCH_MARKER_T,         _HKI( "Schematic Marker" ) )
            .Map( SCH_JUNCTION_T,       _HKI( "Junction" ) )
            .Map( SCH_NO_CONNECT_T,     _HKI( "No-Connect Flag" ) )
            .Map( SCH_BUS_WIRE_ENTRY_T, _HKI( "Wire Entry" ) )
            .Map( SCH_BUS_BUS_ENTRY_T,  _HKI( "Bus Entry" ) )
            .Map( SCH_LINE_T,           _HKI( "Graphic Line" ) )
            .Map( SCH_BITMAP_T,         _HKI( "Bitmap" ) )
            .Map( SCH_TEXT_T,           _HKI( "Schematic Text" ) )
            .Map( SCH_LABEL_T,          _HKI( "Net Label" ) )
            .Map( SCH_GLOBAL_LABEL_T,   _HKI( "Global Label" ) )
            .Map( SCH_HIER_LABEL_T,     _HKI( "Hierarchical Label" ) )
            .Map( SCH_FIELD_T,          _HKI( "Schematic Field" ) )
            .Map( SCH_COMPONENT_T,      _HKI( "Component" ) )
            .Map( SCH_SHEET_PIN_T,      _HKI( "Sheet Pin" ) )
            .Map( SCH_SHEET_T,          _HKI( "Sheet" ) )

            // Synthetic search tokens don't need to be included...
            //.Map( SCH_FIELD_LOCATE_REFERENCE_T, _HKI( "Field Locate Reference" ) )
            //.Map( SCH_FIELD_LOCATE_VALUE_T,     _HKI( "Field Locate Value" ) )
            //.Map( SCH_FIELD_LOCATE_FOOTPRINT_T, _HKI( "Field Locate Footprint" ) )

            .Map( SCH_SCREEN_T,         _HKI( "SCH Screen" ) )

            .Map( LIB_PART_T,           _HKI( "Symbol" ) )
            .Map( LIB_ALIAS_T,          _HKI( "Alias" ) )
            .Map( LIB_ARC_T,            _HKI( "Arc" ) )
            .Map( LIB_CIRCLE_T,         _HKI( "Circle" ) )
            .Map( LIB_TEXT_T,           _HKI( "Symbol Text" ) )
            .Map( LIB_RECTANGLE_T,      _HKI( "Rectangle" ) )
            .Map( LIB_POLYLINE_T,       _HKI( "Polyline" ) )
            .Map( LIB_BEZIER_T,         _HKI( "Bezier" ) )
            .Map( LIB_PIN_T,            _HKI( "Pin" ) )
            .Map( LIB_FIELD_T,          _HKI( "Symbol Field" ) )

            .Map( GERBER_LAYOUT_T,      _HKI( "Gerber Layout" ) )
            .Map( GERBER_DRAW_ITEM_T,   _HKI( "Draw Item" ) )
            .Map( GERBER_IMAGE_T,       _HKI( "Image" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( EDA_ITEM );
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_ITEM, KICAD_T>( "Type",
                    NO_SETTER( EDA_ITEM, KICAD_T ), &EDA_ITEM::Type ) );
    }
} _EDA_ITEM_DESC;

ENUM_TO_WXANY( KICAD_T );
