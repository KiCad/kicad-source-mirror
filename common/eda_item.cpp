/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <trace_helpers.h>
#include <trigo.h>
#include <i18n_utility.h>
#include <wx/log.h>

#include <wx/fdrepdlg.h>
#include <eda_pattern_match.h>

EDA_ITEM::EDA_ITEM( EDA_ITEM* parent, KICAD_T idType ) :
        m_parent( parent ),
        m_forceVisible( false ),
        m_flags( 0 ),
        m_structType( idType )
{ }


EDA_ITEM::EDA_ITEM( KICAD_T idType ) :
        m_parent( nullptr ),
        m_forceVisible( false ),
        m_flags( 0 ),
        m_structType( idType )
{ }


EDA_ITEM::EDA_ITEM( const EDA_ITEM& base ) :
        m_Uuid( base.m_Uuid ),
        m_parent( base.m_parent ),
        m_forceVisible( base.m_forceVisible ),
        m_flags( base.m_flags ),
        m_structType( base.m_structType )
{
    SetForcedTransparency( base.GetForcedTransparency() );
}


void EDA_ITEM::SetModified()
{
    SetFlags( IS_CHANGED );

    // If this a child object, then the parent modification state also needs to be set.
    if( m_parent )
        m_parent->SetModified();
}


const BOX2I EDA_ITEM::GetBoundingBox() const
{
    // return a zero-sized box per default. derived classes should override
    // this
    return BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 0, 0 ) );
}


EDA_ITEM* EDA_ITEM::Clone() const
{
    wxCHECK_MSG( false, nullptr, wxT( "Clone not implemented in derived class " ) + GetClass() +
                 wxT( ".  Bad programmer!" ) );
}


// see base_struct.h
// many classes inherit this method, be careful:
INSPECT_RESULT EDA_ITEM::Visit( INSPECTOR inspector, void* testData,
                                const std::vector<KICAD_T>& aScanTypes )
{
#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    if( IsType( aScanTypes ) )
    {
        if( INSPECT_RESULT::QUIT == inspector( this, testData ) )
            return INSPECT_RESULT::QUIT;
    }

    return INSPECT_RESULT::CONTINUE;
}


wxString EDA_ITEM::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    wxFAIL_MSG( wxT( "GetItemDescription() was not overridden for schematic item type " ) +
                GetClass() );

    return wxString( wxT( "Undefined item description for " ) + GetClass() );
}


bool EDA_ITEM::Matches( const wxString& aText, const EDA_SEARCH_DATA& aSearchData ) const
{
    wxString text = aText;
    wxString searchText = aSearchData.findString;

    // Don't match if searching for replaceable item and the item doesn't support text replace.
    if( aSearchData.searchAndReplace && !IsReplaceable() )
        return false;

    if( !aSearchData.matchCase )
    {
        text.MakeUpper();
        searchText.MakeUpper();
    }

    if( aSearchData.matchMode == EDA_SEARCH_MATCH_MODE::PERMISSIVE )
    {
        EDA_COMBINED_MATCHER matcher( searchText, CTX_SEARCH );

        return matcher.Find( text );
    }
    else if( aSearchData.matchMode == EDA_SEARCH_MATCH_MODE::WHOLEWORD )
    {
        int ii = 0;

        while( ii < (int) text.length() )
        {
            int next = text.find( searchText, ii );

            if( next == wxNOT_FOUND )
                return false;

            ii = next;
            next += searchText.length();

            bool startOK = ( ii == 0 || !wxIsalnum( text.GetChar( ii - 1 ) ) );
            bool endOK = ( next == (int) text.length() || !wxIsalnum( text.GetChar( next ) ) );

            if( startOK && endOK )
                return true;
            else
                ii++;
        }

        return false;
    }
    else if( aSearchData.matchMode == EDA_SEARCH_MATCH_MODE::WILDCARD )
    {
        return text.Matches( searchText );
    }
    else
    {
        return text.Find( searchText ) != wxNOT_FOUND;
    }
}


bool EDA_ITEM::Replace( const EDA_SEARCH_DATA& aSearchData, wxString& aText )
{
    wxString text = aText;
    wxString searchText = aSearchData.findString;
    wxString result;
    bool     replaced = false;

    if( !aSearchData.matchCase )
    {
        text = text.Upper();
        searchText = searchText.Upper();
    }

    int ii = 0;

    while( ii < (int) text.length() )
    {
        int next = text.find( searchText, ii );

        if( next == wxNOT_FOUND )
        {
            result += aText.Mid( ii, wxString::npos );
            break;
        }

        if( next > ii )
            result += aText.Mid( ii, next - ii );

        ii = next;
        next += searchText.length();

        bool startOK;
        bool endOK;

        if( aSearchData.matchMode == EDA_SEARCH_MATCH_MODE::WHOLEWORD )
        {
            startOK = ( ii == 0 || !wxIsalnum( text.GetChar( ii - 1 ) ) );
            endOK = ( next == (int) text.length() || !wxIsalnum( text.GetChar( next ) ) );
        }
        else
        {
            startOK = true;
            endOK = true;
        }

        if( startOK && endOK )
        {
            result += aSearchData.replaceString;
            replaced = true;
            ii = next;
        }
        else
        {
            result += aText.GetChar( ii );
            ii++;
        }
    }

    aText = result;
    return replaced;
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

    m_structType   = aItem.m_structType;
    m_flags        = aItem.m_flags;
    m_parent       = aItem.m_parent;
    m_forceVisible = aItem.m_forceVisible;

    SetForcedTransparency( aItem.GetForcedTransparency() );

    return *this;
}


const BOX2I EDA_ITEM::ViewBBox() const
{
    // Basic fallback
    return GetBoundingBox();
}


void EDA_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount      = 1;
    aLayers[0]  = 0;
}


BITMAPS EDA_ITEM::GetMenuImage() const
{
    return BITMAPS::dummy_item;
}


#if defined( DEBUG )

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


wxString EDA_ITEM::GetTypeDesc() const
{
    //@see EDA_ITEM_DESC for definition of ENUM_MAP<KICAD_T>
    wxString typeDescr = ENUM_MAP<KICAD_T>::Instance().ToString( Type() );

    return wxGetTranslation( typeDescr );
}


wxString EDA_ITEM::GetFriendlyName() const
{
    return GetTypeDesc();
}


static struct EDA_ITEM_DESC
{
    EDA_ITEM_DESC()
    {
        ENUM_MAP<KICAD_T>::Instance()
            .Undefined( TYPE_NOT_INIT )
            .Map( NOT_USED,                wxT( "<not used>" ) )
            .Map( SCREEN_T,                _HKI( "Screen" ) )

            .Map( PCB_FOOTPRINT_T,         _HKI( "Footprint" ) )
            .Map( PCB_PAD_T,               _HKI( "Pad" ) )
            .Map( PCB_SHAPE_T,             _HKI( "Graphic" ) )
            .Map( PCB_BITMAP_T,            _HKI( "Bitmap" ) )
            .Map( PCB_FIELD_T,             _HKI( "Field" ) )
            .Map( PCB_TEXT_T,              _HKI( "Text" ) )
            .Map( PCB_TEXTBOX_T,           _HKI( "Text Box" ) )
            .Map( PCB_TRACE_T,             _HKI( "Track" ) )
            .Map( PCB_ARC_T,               _HKI( "Track" ) )
            .Map( PCB_VIA_T,               _HKI( "Via" ) )
            .Map( PCB_MARKER_T,            _HKI( "Marker" ) )
            .Map( PCB_DIM_ALIGNED_T,       _HKI( "Dimension" ) )
            .Map( PCB_DIM_ORTHOGONAL_T,    _HKI( "Dimension" ) )
            .Map( PCB_DIM_CENTER_T,        _HKI( "Dimension" ) )
            .Map( PCB_DIM_RADIAL_T,        _HKI( "Dimension" ) )
            .Map( PCB_DIM_LEADER_T,        _HKI( "Leader" ) )
            .Map( PCB_TARGET_T,            _HKI( "Target" ) )
            .Map( PCB_ZONE_T,              _HKI( "Zone" ) )
            .Map( PCB_ITEM_LIST_T,         _HKI( "ItemList" ) )
            .Map( PCB_NETINFO_T,           _HKI( "NetInfo" ) )
            .Map( PCB_GROUP_T,             _HKI( "Group" ) )

            .Map( SCH_MARKER_T,            _HKI( "Marker" ) )
            .Map( SCH_JUNCTION_T,          _HKI( "Junction" ) )
            .Map( SCH_NO_CONNECT_T,        _HKI( "No-Connect Flag" ) )
            .Map( SCH_BUS_WIRE_ENTRY_T,    _HKI( "Wire Entry" ) )
            .Map( SCH_BUS_BUS_ENTRY_T,     _HKI( "Bus Entry" ) )
            .Map( SCH_LINE_T,              _HKI( "Line" ) )
            .Map( SCH_BITMAP_T,            _HKI( "Bitmap" ) )
            .Map( SCH_SHAPE_T,             _HKI( "Graphic" ) )
            .Map( SCH_TEXT_T,              _HKI( "Text" ) )
            .Map( SCH_TEXTBOX_T,           _HKI( "Text Box" ) )
            .Map( SCH_LABEL_T,             _HKI( "Net Label" ) )
            .Map( SCH_DIRECTIVE_LABEL_T,   _HKI( "Directive Label" ) )
            .Map( SCH_GLOBAL_LABEL_T,      _HKI( "Global Label" ) )
            .Map( SCH_HIER_LABEL_T,        _HKI( "Hierarchical Label" ) )
            .Map( SCH_FIELD_T,             _HKI( "Field" ) )
            .Map( SCH_SYMBOL_T,            _HKI( "Symbol" ) )
            .Map( SCH_PIN_T,               _HKI( "Pin" ) )
            .Map( SCH_SHEET_PIN_T,         _HKI( "Sheet Pin" ) )
            .Map( SCH_SHEET_T,             _HKI( "Sheet" ) )

            // Synthetic search tokens don't need to be included...
            //.Map( SCH_FIELD_LOCATE_REFERENCE_T, _HKI( "Field Locate Reference" ) )
            //.Map( SCH_FIELD_LOCATE_VALUE_T,     _HKI( "Field Locate Value" ) )
            //.Map( SCH_FIELD_LOCATE_FOOTPRINT_T, _HKI( "Field Locate Footprint" ) )

            .Map( SCH_SCREEN_T,            _HKI( "SCH Screen" ) )

            .Map( LIB_SYMBOL_T,            _HKI( "Symbol" ) )
            .Map( LIB_SHAPE_T,             _HKI( "Graphic" ) )
            .Map( LIB_TEXT_T,              _HKI( "Text" ) )
            .Map( LIB_TEXTBOX_T,           _HKI( "Text Box" ) )
            .Map( LIB_PIN_T,               _HKI( "Pin" ) )
            .Map( LIB_FIELD_T,             _HKI( "Symbol Field" ) )

            .Map( GERBER_LAYOUT_T,         _HKI( "Gerber Layout" ) )
            .Map( GERBER_DRAW_ITEM_T,      _HKI( "Draw Item" ) )
            .Map( GERBER_IMAGE_T,          _HKI( "Image" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( EDA_ITEM );

        propMgr.AddProperty( new PROPERTY_ENUM<EDA_ITEM, KICAD_T>( wxS( "Type" ),
                             NO_SETTER( EDA_ITEM, KICAD_T ), &EDA_ITEM::Type ) )
                .SetIsHiddenFromPropertiesManager();
    }
} _EDA_ITEM_DESC;

ENUM_TO_WXANY( KICAD_T );
