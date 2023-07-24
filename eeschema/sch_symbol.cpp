/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <sch_edit_frame.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <lib_shape.h>
#include <pgm_base.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <refdes_utils.h>
#include <wx/log.h>
#include <settings/settings_manager.h>
#include <string_utils.h>

#include <utility>
#include "plotters/plotter.h"
#include "sim/sim_model.h"


std::unordered_map<TRANSFORM, int> SCH_SYMBOL::s_transformToOrientationCache;


/**
 * Convert a wxString to UTF8 and replace any control characters with a ~,
 * where a control character is one of the first ASCII values up to ' ' 32d.
 */
std::string toUTFTildaText( const wxString& txt )
{
    std::string ret = TO_UTF8( txt );

    for( std::string::iterator it = ret.begin();  it!=ret.end();  ++it )
    {
        if( (unsigned char) *it <= ' ' )
            *it = '~';
    }

    return ret;
}


/**
 * Used to draw a dummy shape when a LIB_SYMBOL is not found in library
 *
 * This symbol is a 400 mils square with the text "??"
 */
static LIB_SYMBOL* dummy()
{
    static LIB_SYMBOL* symbol;

    if( !symbol )
    {
        symbol = new LIB_SYMBOL( wxEmptyString );

        LIB_SHAPE* square = new LIB_SHAPE( symbol, SHAPE_T::RECTANGLE );

        square->MoveTo( VECTOR2I( schIUScale.MilsToIU( -200 ), schIUScale.MilsToIU( 200 ) ) );
        square->SetEnd( VECTOR2I( schIUScale.MilsToIU( 200 ), schIUScale.MilsToIU( -200 ) ) );

        LIB_TEXT* text = new LIB_TEXT( symbol );

        text->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 150 ), schIUScale.MilsToIU( 150 ) ) );
        text->SetText( wxString( wxT( "??" ) ) );

        symbol->AddDrawItem( square );
        symbol->AddDrawItem( text );
    }

    return symbol;
}


SCH_SYMBOL::SCH_SYMBOL() :
    SCH_ITEM( nullptr, SCH_SYMBOL_T )
{
    m_DNP = false;
    Init( VECTOR2I( 0, 0 ) );
}


SCH_SYMBOL::SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const LIB_ID& aLibId,
                        const SCH_SHEET_PATH* aSheet, int aUnit, int aConvert,
                        const VECTOR2I& aPosition, EDA_ITEM* aParent ) :
    SCH_ITEM( aParent, SCH_SYMBOL_T )
{
    Init( aPosition );

    m_unit      = aUnit;
    m_convert   = aConvert;
    m_lib_id    = aLibId;

    std::unique_ptr< LIB_SYMBOL > part;

    part = aSymbol.Flatten();
    part->SetParent();
    SetLibSymbol( part.release() );

    // Copy fields from the library symbol
    UpdateFields( aSheet,
                  true,   /* update style */
                  false,  /* update ref */
                  false,  /* update other fields */
                  true,   /* reset ref */
                  true    /* reset other fields */ );

    m_prefix = UTIL::GetRefDesPrefix( m_part->GetReferenceField().GetText() );

    if( aSheet )
        SetRef( aSheet, UTIL::GetRefDesUnannotated( m_prefix ) );

    // Inherit the include in bill of materials and board netlist settings from flattened
    // library symbol.
    m_excludedFromBOM = m_part->GetExcludedFromBOM();
    m_excludedFromBoard = m_part->GetExcludedFromBoard();
    m_DNP = false;

}


SCH_SYMBOL::SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const SCH_SHEET_PATH* aSheet,
                        const PICKED_SYMBOL& aSel, const VECTOR2I& aPosition,
                        EDA_ITEM* aParent ) :
    SCH_SYMBOL( aSymbol, aSel.LibId, aSheet, aSel.Unit, aSel.Convert, aPosition, aParent )
{
    // Set any fields that were modified as part of the symbol selection
    for( const std::pair<int, wxString>& i : aSel.Fields )
    {
        if( i.first == REFERENCE_FIELD )
            SetRef( aSheet, i.second );
        else if( SCH_FIELD* field = GetFieldById( i.first ) )
            field->SetText( i.second );
    }
}


SCH_SYMBOL::SCH_SYMBOL( const SCH_SYMBOL& aSymbol ) :
    SCH_ITEM( aSymbol )
{
    m_parent      = aSymbol.m_parent;
    m_pos         = aSymbol.m_pos;
    m_unit        = aSymbol.m_unit;
    m_convert     = aSymbol.m_convert;
    m_lib_id      = aSymbol.m_lib_id;
    m_isInNetlist = aSymbol.m_isInNetlist;
    m_excludedFromBOM = aSymbol.m_excludedFromBOM;
    m_excludedFromBoard = aSymbol.m_excludedFromBoard;
    m_DNP         = aSymbol.m_DNP;

    if( aSymbol.m_part )
        SetLibSymbol( new LIB_SYMBOL( *aSymbol.m_part.get() ) );

    const_cast<KIID&>( m_Uuid ) = aSymbol.m_Uuid;

    m_transform = aSymbol.m_transform;
    m_prefix = aSymbol.m_prefix;
    m_instanceReferences = aSymbol.m_instanceReferences;
    m_fields = aSymbol.m_fields;

    // Re-parent the fields, which before this had aSymbol as parent
    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

    m_fieldsAutoplaced = aSymbol.m_fieldsAutoplaced;
    m_schLibSymbolName = aSymbol.m_schLibSymbolName;
}


void SCH_SYMBOL::Init( const VECTOR2I& pos )
{
    m_layer   = LAYER_DEVICE;
    m_pos     = pos;
    m_unit    = 1;  // In multi unit chip - which unit to draw.
    m_convert = LIB_ITEM::LIB_CONVERT::BASE;  // De Morgan Handling

    // The rotation/mirror transformation matrix. pos normal
    m_transform = TRANSFORM();

    // construct only the mandatory fields, which are the first 4 only.
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        m_fields.emplace_back( pos, i, this, TEMPLATE_FIELDNAME::GetDefaultFieldName( i ) );

        if( i == REFERENCE_FIELD )
            m_fields.back().SetLayer( LAYER_REFERENCEPART );
        else if( i == VALUE_FIELD )
            m_fields.back().SetLayer( LAYER_VALUEPART );
        else
            m_fields.back().SetLayer( LAYER_FIELDS );
    }

    m_prefix = wxString( wxT( "U" ) );
    m_isInNetlist = true;
    m_excludedFromBOM = false;
    m_excludedFromBoard = false;
}


EDA_ITEM* SCH_SYMBOL::Clone() const
{
    return new SCH_SYMBOL( *this );
}


bool SCH_SYMBOL::IsMissingLibSymbol() const
{
    if( !m_part )
        return true;

    return false;
}


void SCH_SYMBOL::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 8;
    aLayers[0] = LAYER_DANGLING;       // Pins are drawn by their parent symbol, so the parent
                                       // symbol needs to draw to LAYER_DANGLING
    aLayers[1] = LAYER_OP_CURRENTS;    // Same for pin operating points
    aLayers[2] = LAYER_DEVICE;
    aLayers[3] = LAYER_REFERENCEPART;
    aLayers[4] = LAYER_VALUEPART;
    aLayers[5] = LAYER_FIELDS;
    aLayers[6] = LAYER_DEVICE_BACKGROUND;
    aLayers[7] = LAYER_SELECTION_SHADOWS;
}


bool SCH_SYMBOL::IsMovableFromAnchorPoint() const
{
    // If a symbol's anchor is not grid-aligned to its pins then moving from the anchor is
    // going to end up moving the symbol's pins off-grid.

    // The minimal grid size allowed to place a pin is 25 mils
    const int min_grid_size = schIUScale.MilsToIU( 25 );

    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        if( ( ( pin->GetPosition().x - m_pos.x ) % min_grid_size ) != 0 )
            return false;

        if( ( ( pin->GetPosition().y - m_pos.y ) % min_grid_size ) != 0 )
            return false;
    }

    return true;
}


void SCH_SYMBOL::SetLibId( const LIB_ID& aLibId )
{
    m_lib_id = aLibId;
}


wxString SCH_SYMBOL::GetSchSymbolLibraryName() const
{
    if( !m_schLibSymbolName.IsEmpty() )
        return m_schLibSymbolName;
    else
        return m_lib_id.Format();
}


void SCH_SYMBOL::SetLibSymbol( LIB_SYMBOL* aLibSymbol )
{
    wxCHECK2( ( aLibSymbol == nullptr ) || ( aLibSymbol->IsRoot() ), aLibSymbol = nullptr );

    m_part.reset( aLibSymbol );
    UpdatePins();
}


wxString SCH_SYMBOL::GetDescription() const
{
    if( m_part )
        return m_part->GetDescription();

    return wxEmptyString;
}


wxString SCH_SYMBOL::GetKeyWords() const
{
    if( m_part )
        return m_part->GetKeyWords();

    return wxEmptyString;
}


wxString SCH_SYMBOL::GetDatasheet() const
{
    if( m_part )
        return m_part->GetDatasheetField().GetText();

    return wxEmptyString;
}


void SCH_SYMBOL::UpdatePins()
{
    std::map<wxString, wxString> altPinMap;
    std::map<wxString, KIID>     pinUuidMap;

    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        pinUuidMap[ pin->GetNumber() ] = pin->m_Uuid;

        if( !pin->GetAlt().IsEmpty() )
            altPinMap[ pin->GetNumber() ] = pin->GetAlt();
    }

    m_pins.clear();
    m_pinMap.clear();

    if( !m_part )
        return;

    unsigned i = 0;

    std::vector<LIB_PIN*> pins = m_part->GetAllLibPins();

    for( LIB_PIN* libPin : pins )
    {
        // NW: Don't filter by unit: this data-structure is used for all instances,
        // some if which might have different units.
        if( libPin->GetConvert() && m_convert && m_convert != libPin->GetConvert() )
            continue;

        m_pins.push_back( std::make_unique<SCH_PIN>( libPin, this ) );

        auto ii = pinUuidMap.find( libPin->GetNumber() );

        if( ii != pinUuidMap.end() )
            const_cast<KIID&>( m_pins.back()->m_Uuid ) = ii->second;

        auto iii = altPinMap.find( libPin->GetNumber() );

        if( iii != altPinMap.end() )
            m_pins.back()->SetAlt( iii->second );

        m_pinMap[ libPin ] = i;

        ++i;
    }
}


void SCH_SYMBOL::SetUnit( int aUnit )
{
    UpdateUnit( aUnit );
}


void SCH_SYMBOL::UpdateUnit( int aUnit )
{
    m_unit = aUnit;
}


void SCH_SYMBOL::SetConvert( int aConvert )
{
    if( m_convert != aConvert )
    {
        m_convert = aConvert;

        // The convert may have a different pin layout so the update the pin map.
        UpdatePins();
    }
}


void SCH_SYMBOL::SetTransform( const TRANSFORM& aTransform )
{
    if( m_transform != aTransform )
        m_transform = aTransform;
}


int SCH_SYMBOL::GetUnitCount() const
{
    if( m_part )
        return m_part->GetUnitCount();

    return 0;
}


wxString SCH_SYMBOL::GetUnitDisplayName( int aUnit )
{
    wxCHECK( m_part, ( wxString::Format( _( "Unit %s" ), LIB_SYMBOL::SubReference( aUnit ) ) ) );

    return m_part->GetUnitDisplayName( aUnit );
}


bool SCH_SYMBOL::HasUnitDisplayName( int aUnit )
{
    wxCHECK( m_part, false );

    return m_part->HasUnitDisplayName( aUnit );
}


void SCH_SYMBOL::PrintBackground( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    LIB_SYMBOL_OPTIONS opts;
    opts.transform = m_transform;
    opts.draw_visible_fields = false;
    opts.draw_hidden_fields = false;

    if( m_part )
        m_part->PrintBackground( aSettings, m_pos + aOffset, m_unit, m_convert, opts, GetDNP() );
}


void SCH_SYMBOL::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    LIB_SYMBOL_OPTIONS opts;
    opts.transform = m_transform;
    opts.draw_visible_fields = false;
    opts.draw_hidden_fields = false;

    if( m_part )
    {
        LIB_PINS  libPins;
        m_part->GetPins( libPins, m_unit, m_convert );

        LIB_SYMBOL tempSymbol( *m_part );
        LIB_PINS tempPins;
        tempSymbol.GetPins( tempPins, m_unit, m_convert );

        // Copy the pin info from the symbol to the temp pins
        for( unsigned i = 0; i < tempPins.size(); ++ i )
        {
            SCH_PIN* symbolPin = GetPin( libPins[ i ] );
            LIB_PIN* tempPin = tempPins[ i ];

            tempPin->SetName( symbolPin->GetShownName() );
            tempPin->SetType( symbolPin->GetType() );
            tempPin->SetShape( symbolPin->GetShape() );
        }

        tempSymbol.Print( aSettings, m_pos + aOffset, m_unit, m_convert, opts, GetDNP() );
    }
    else    // Use dummy() part if the actual cannot be found.
    {
        dummy()->Print( aSettings, m_pos + aOffset, 0, 0, opts, GetDNP() );
    }

    for( SCH_FIELD& field : m_fields )
        field.Print( aSettings, aOffset );

    if( m_DNP )
    {
        BOX2I bbox = GetBodyAndPinsBoundingBox();
        wxDC* DC = aSettings->GetPrintDC();
        COLOR4D dnp_color = aSettings->GetLayerColor( LAYER_DNP_MARKER );

        GRFilledSegment( DC, bbox.GetOrigin(), bbox.GetEnd(),
                             3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ),
                             dnp_color );

        GRFilledSegment( DC, bbox.GetOrigin() + VECTOR2I( bbox.GetWidth(), 0 ),
                             bbox.GetOrigin() + VECTOR2I( 0, bbox.GetHeight() ),
                             3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ),
                             dnp_color );
    }
}


void SCH_SYMBOL::SetExcludeFromSim( bool aExclude )
{
    SCH_FIELD* enable = FindField( SIM_ENABLE_FIELD );

    if( aExclude )
    {
        if( !enable )
            enable = AddField( SCH_FIELD( VECTOR2I( 0, 0 ), -1, this, SIM_ENABLE_FIELD ) );

        enable->SetText( wxS( "0" ) );
    }
    else
    {
        RemoveField( SIM_ENABLE_FIELD );
    }
}


bool SCH_SYMBOL::GetExcludeFromSim() const
{
    return GetFieldText( SIM_ENABLE_FIELD ) == wxS( "0" );
}


bool SCH_SYMBOL::GetInstance( SCH_SYMBOL_INSTANCE& aInstance,
                              const KIID_PATH& aSheetPath, bool aTestFromEnd ) const
{
    for( const SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        if( !aTestFromEnd )
        {
            if( instance.m_Path == aSheetPath )
            {
                aInstance = instance;
                return true;
            }
        }
        else if( instance.m_Path.EndsWith( aSheetPath ) )
        {
            aInstance = instance;
            return true;
        }
    }

    return false;
}


void SCH_SYMBOL::RemoveInstance( const SCH_SHEET_PATH& aInstancePath )
{
    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii < m_instanceReferences.size(); ii++ )
    {
        if( m_instanceReferences[ii].m_Path == aInstancePath.Path() )
        {
            wxLogTrace( traceSchSheetPaths, "Removing symbol instance:\n"
                                            "  sheet path %s\n"
                                            "  reference %s, unit %d from symbol %s.",
                        aInstancePath.Path().AsString(),
                        m_instanceReferences[ii].m_Reference,
                        m_instanceReferences[ii].m_Unit,
                        m_Uuid.AsString() );

            m_instanceReferences.erase( m_instanceReferences.begin() + ii );
            ii--;
        }
    }
}


void SCH_SYMBOL::SortInstances( bool (*aSortFunction)( const SCH_SYMBOL_INSTANCE& aLhs,
                                                       const SCH_SYMBOL_INSTANCE& aRhs ) )
{
    std::sort( m_instanceReferences.begin(), m_instanceReferences.end(), aSortFunction );
}


void SCH_SYMBOL::AddHierarchicalReference( const KIID_PATH& aPath, const wxString& aRef, int aUnit )
{
    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii < m_instanceReferences.size(); ii++ )
    {
        if( m_instanceReferences[ii].m_Path == aPath )
        {
            wxLogTrace( traceSchSheetPaths, "Removing symbol instance:\n"
                                            "  sheet path %s\n"
                                            "  reference %s, unit %d from symbol %s.",
                                            aPath.AsString(),
                                            m_instanceReferences[ii].m_Reference,
                                            m_instanceReferences[ii].m_Unit,
                                            m_Uuid.AsString() );

            m_instanceReferences.erase( m_instanceReferences.begin() + ii );
            ii--;
        }
    }

    SCH_SYMBOL_INSTANCE instance;
    instance.m_Path = aPath;
    instance.m_Reference = aRef;
    instance.m_Unit = aUnit;

    wxLogTrace( traceSchSheetPaths,
                "Adding symbol '%s' instance:\n"
                "    sheet path '%s'\n"
                "    reference '%s'\n"
                "    unit %d\n",
                m_Uuid.AsString(),
                aPath.AsString(),
                aRef,
                aUnit );

    m_instanceReferences.push_back( instance );

    // This should set the default instance to the first saved instance data for each symbol
    // when importing sheets.
    if( m_instanceReferences.size() == 1 )
    {
        m_fields[ REFERENCE_FIELD ].SetText( aRef );
        m_unit = aUnit;
    }
}


void SCH_SYMBOL::AddHierarchicalReference( const SCH_SYMBOL_INSTANCE& aInstance )
{
    KIID_PATH searchPath( aInstance.m_Path );

    std::vector<SCH_SYMBOL_INSTANCE>::iterator resultIt;

    do
    {
        resultIt = std::find_if( m_instanceReferences.begin(), m_instanceReferences.end(),
                                 [searchPath]( const auto& it )
                                 {
                                     return it.m_Path == searchPath;
                                 } );

        if( resultIt != m_instanceReferences.end() )
        {
            wxLogTrace( traceSchSheetPaths, "Removing symbol instance:\n"
                        "  sheet path %s\n"
                        "  reference %s, unit %d from symbol %s.",
                        aInstance.m_Path.AsString(),
                        resultIt->m_Reference,
                        resultIt->m_Unit,
                        m_Uuid.AsString() );

            // Instance data should be unique by path.  Double check just in case there was
            // some buggy code in the past.
            resultIt = m_instanceReferences.erase( resultIt );
        }
    }
    while( resultIt != m_instanceReferences.end() );

    SCH_SYMBOL_INSTANCE instance = aInstance;

    wxLogTrace( traceSchSheetPaths,
                "Adding symbol '%s' instance:\n"
                "    sheet path '%s'\n"
                "    reference '%s'\n"
                "    unit %d\n",
                m_Uuid.AsString(),
                instance.m_Path.AsString(),
                instance.m_Reference,
                instance.m_Unit );

    m_instanceReferences.push_back( instance );

    // This should set the default instance to the first saved instance data for each symbol
    // when importing sheets.
    if( m_instanceReferences.size() == 1 )
    {
        m_fields[ REFERENCE_FIELD ].SetText( instance.m_Reference );
        m_unit = instance.m_Unit;
    }
}


const wxString SCH_SYMBOL::GetRef( const SCH_SHEET_PATH* sheet, bool aIncludeUnit ) const
{
    KIID_PATH path = sheet->Path();
    wxString  ref;
    wxString  subRef;

    for( const SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            ref = instance.m_Reference;
            subRef = LIB_SYMBOL::SubReference( instance.m_Unit );
            break;
        }
    }

    // If it was not found in m_Paths array, then see if it is in m_Field[REFERENCE] -- if so,
    // use this as a default for this path.  This will happen if we load a version 1 schematic
    // file.  It will also mean that multiple instances of the same sheet by default all have
    // the same symbol references, but perhaps this is best.
    if( ref.IsEmpty() && !GetField( REFERENCE_FIELD )->GetText().IsEmpty() )
    {
        const_cast<SCH_SYMBOL*>( this )->SetRef( sheet, GetField( REFERENCE_FIELD )->GetText() );
        ref = GetField( REFERENCE_FIELD )->GetText();
    }

    if( ref.IsEmpty() )
        ref = UTIL::GetRefDesUnannotated( m_prefix );

    if( aIncludeUnit && GetUnitCount() > 1 )
        ref += subRef;

    return ref;
}


bool SCH_SYMBOL::IsReferenceStringValid( const wxString& aReferenceString )
{
    return !UTIL::GetRefDesPrefix( aReferenceString ).IsEmpty();
}


void SCH_SYMBOL::SetRef( const SCH_SHEET_PATH* sheet, const wxString& ref )
{
    KIID_PATH path = sheet->Path();
    bool      found = false;

    // check to see if it is already there before inserting it
    for( SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            found = true;
            instance.m_Reference = ref;
            break;
        }
    }

    if( !found )
        AddHierarchicalReference( path, ref, m_unit );

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
        pin->ClearDefaultNetName( sheet );

    if( Schematic() && *sheet == Schematic()->CurrentSheet() )
        m_fields[ REFERENCE_FIELD ].SetText( ref );

    // Reinit the m_prefix member if needed
    m_prefix = UTIL::GetRefDesPrefix( ref );

    if( m_prefix.IsEmpty() )
        m_prefix = wxT( "U" );

    // Power symbols have references starting with # and are not included in netlists
    m_isInNetlist = ! ref.StartsWith( wxT( "#" ) );
}


bool SCH_SYMBOL::IsAnnotated( const SCH_SHEET_PATH* aSheet )
{
    KIID_PATH path = aSheet->Path();

    for( const SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
            return instance.m_Reference.Last() != '?';
    }

    return false;
}


void SCH_SYMBOL::UpdatePrefix()
{
    wxString refDesignator = GetField( REFERENCE_FIELD )->GetText();

    refDesignator.Replace( "~", " " );

    wxString prefix = refDesignator;

    while( prefix.Length() )
    {
        wxUniCharRef last = prefix.Last();

        if( ( last >= '0' && last <= '9' ) || last == '?' || last == '*' )
            prefix.RemoveLast();
        else
            break;
    }

    // Avoid a prefix containing trailing/leading spaces
    prefix.Trim( true );
    prefix.Trim( false );

    if( !prefix.IsEmpty() )
        SetPrefix( prefix );
}


int SCH_SYMBOL::GetUnitSelection( const SCH_SHEET_PATH* aSheet ) const
{
    KIID_PATH path = aSheet->Path();

    for( const SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
            return instance.m_Unit;
    }

    // If it was not found in m_Paths array, then use m_unit.  This will happen if we load a
    // version 1 schematic file.
    return m_unit;
}


void SCH_SYMBOL::SetUnitSelection( const SCH_SHEET_PATH* aSheet, int aUnitSelection )
{
    KIID_PATH path = aSheet->Path();

    // check to see if it is already there before inserting it
    for( SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            instance.m_Unit = aUnitSelection;
            return;
        }
    }

    // didn't find it; better add it
    AddHierarchicalReference( path, UTIL::GetRefDesUnannotated( m_prefix ), aUnitSelection );
}


void SCH_SYMBOL::SetUnitSelection( int aUnitSelection )
{
    for( SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
        instance.m_Unit = aUnitSelection;
}


const wxString SCH_SYMBOL::GetValueFieldText( bool aResolve, const SCH_SHEET_PATH* aPath,
                                              bool aAllowExtraText ) const
{
    if( aResolve )
        return GetField( VALUE_FIELD )->GetShownText( aPath, aAllowExtraText );

    return GetField( VALUE_FIELD )->GetText();
}


void SCH_SYMBOL::SetValueFieldText( const wxString& aValue )
{
    m_fields[ VALUE_FIELD ].SetText( aValue );
}


const wxString SCH_SYMBOL::GetFootprintFieldText( bool aResolve, const SCH_SHEET_PATH* aPath,
                                                  bool aAllowExtraText ) const
{
    if( aResolve )
        return GetField( FOOTPRINT_FIELD )->GetShownText( aPath, aAllowExtraText );

    return GetField( FOOTPRINT_FIELD )->GetText();
}


void SCH_SYMBOL::SetFootprintFieldText( const wxString& aFootprint )
{
    m_fields[ FOOTPRINT_FIELD ].SetText( aFootprint );
}


SCH_FIELD* SCH_SYMBOL::GetField( MANDATORY_FIELD_T aFieldType )
{
    return &m_fields[aFieldType];
}


const SCH_FIELD* SCH_SYMBOL::GetField( MANDATORY_FIELD_T aFieldType ) const
{
    return &m_fields[aFieldType];
}


SCH_FIELD* SCH_SYMBOL::GetFieldById( int aFieldId )
{
    for( size_t ii = 0; ii < m_fields.size(); ++ii )
    {
        if( m_fields[ii].GetId() == aFieldId )
            return &m_fields[ii];
    }

    return nullptr;
}


SCH_FIELD* SCH_SYMBOL::GetFieldByName( const wxString& aFieldName )
{
    for( size_t ii = 0; ii < m_fields.size(); ++ii )
    {
        if( m_fields[ii].GetName() == aFieldName )
            return &m_fields[ii];
    }

    return nullptr;
}


wxString SCH_SYMBOL::GetFieldText( const wxString& aFieldName ) const
{
    for( const SCH_FIELD& field : m_fields )
    {
        if( aFieldName == field.GetName() || aFieldName == field.GetCanonicalName() )
            return field.GetText();
    }

    return wxEmptyString;
}


void SCH_SYMBOL::GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly )
{
    for( SCH_FIELD& field : m_fields )
    {
        if( aVisibleOnly )
        {
            if( !field.IsVisible() || field.GetShownText( nullptr, true ).IsEmpty() )
                continue;
        }

        aVector.push_back( &field );
    }
}


SCH_FIELD* SCH_SYMBOL::AddField( const SCH_FIELD& aField )
{
    int newNdx = m_fields.size();

    m_fields.push_back( aField );
    return &m_fields[newNdx];
}


void SCH_SYMBOL::RemoveField( const wxString& aFieldName )
{
    for( unsigned i = MANDATORY_FIELDS; i < m_fields.size(); ++i )
    {
        if( aFieldName == m_fields[i].GetName( false ) )
        {
            m_fields.erase( m_fields.begin() + i );
            return;
        }
    }
}


SCH_FIELD* SCH_SYMBOL::FindField( const wxString& aFieldName, bool aIncludeDefaultFields,
                                  bool aCaseInsensitive )
{
    unsigned start = aIncludeDefaultFields ? 0 : MANDATORY_FIELDS;

    for( unsigned i = start; i < m_fields.size(); ++i )
    {
        if( aCaseInsensitive )
        {
            if( aFieldName.Upper() == m_fields[i].GetName( false ).Upper() )
                return &m_fields[i];
        }
        else
        {
            if( aFieldName == m_fields[i].GetName( false ) )
                return &m_fields[i];
        }
    }

    return nullptr;
}


void SCH_SYMBOL::UpdateFields( const SCH_SHEET_PATH* aPath, bool aUpdateStyle, bool aUpdateRef,
                               bool aUpdateOtherFields, bool aResetRef, bool aResetOtherFields )
{
    if( m_part )
    {
        std::vector<LIB_FIELD*> fields;

        m_part->GetFields( fields );

        for( const LIB_FIELD* libField : fields )
        {
            int id = libField->GetId();
            SCH_FIELD* schField;

            if( id >= 0 && id < MANDATORY_FIELDS )
            {
                schField = GetFieldById( id );
            }
            else
            {
                schField = FindField( libField->GetCanonicalName() );

                if( !schField )
                {
                    wxString  fieldName = libField->GetCanonicalName();
                    SCH_FIELD newField( VECTOR2I( 0, 0 ), GetFieldCount(), this, fieldName );
                    schField = AddField( newField );
                }
            }

            if( aUpdateStyle )
            {
                schField->ImportValues( *libField );
                schField->SetTextPos( m_pos + libField->GetTextPos() );
            }

            if( id == REFERENCE_FIELD && aPath )
            {
                if( aResetRef )
                    SetRef( aPath, m_part->GetReferenceField().GetText() );
                else if( aUpdateRef )
                    SetRef( aPath, libField->GetText() );
            }
            else if( id == VALUE_FIELD )
            {
                SetValueFieldText( UnescapeString( libField->GetText() ) );
            }
            else if( id == FOOTPRINT_FIELD )
            {
                if( aResetOtherFields || aUpdateOtherFields )
                    SetFootprintFieldText( libField->GetText() );
            }
            else if( id == DATASHEET_FIELD )
            {
                if( aResetOtherFields )
                    schField->SetText( GetDatasheet() ); // alias-specific value
                else if( aUpdateOtherFields )
                    schField->SetText( libField->GetText() );
            }
            else
            {
                if( aResetOtherFields || aUpdateOtherFields )
                    schField->SetText( libField->GetText() );
            }
        }
    }
}


void SCH_SYMBOL::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
        aFunction( pin.get() );

    for( SCH_FIELD& field : m_fields )
        aFunction( &field );
}


SCH_PIN* SCH_SYMBOL::GetPin( const wxString& aNumber ) const
{
    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        if( pin->GetNumber() == aNumber )
            return pin.get();
    }

    return nullptr;
}


void SCH_SYMBOL::GetLibPins( std::vector<LIB_PIN*>& aPinsList ) const
{
    if( m_part )
        m_part->GetPins( aPinsList, m_unit, m_convert );
}


std::vector<LIB_PIN*> SCH_SYMBOL::GetAllLibPins() const
{
    std::vector<LIB_PIN*> pinList;

    if( m_part )
        m_part->GetPins( pinList, 0, 0 );

    return pinList;
}


SCH_PIN* SCH_SYMBOL::GetPin( LIB_PIN* aLibPin ) const
{
    wxASSERT( m_pinMap.count( aLibPin ) );
    return m_pins[ m_pinMap.at( aLibPin ) ].get();
}


std::vector<SCH_PIN*> SCH_SYMBOL::GetPins( const SCH_SHEET_PATH* aSheet ) const
{
    std::vector<SCH_PIN*> pins;

    if( aSheet == nullptr )
    {
        wxCHECK_MSG( Schematic(), pins, "Can't call GetPins on a symbol with no schematic" );

        aSheet = &Schematic()->CurrentSheet();
    }

    int unit = GetUnitSelection( aSheet );

    for( const std::unique_ptr<SCH_PIN>& p : m_pins )
    {
        if( unit && p->GetLibPin()->GetUnit() && ( p->GetLibPin()->GetUnit() != unit ) )
            continue;

        pins.push_back( p.get() );
    }

    return pins;
}


void SCH_SYMBOL::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    wxCHECK_RET( aItem != nullptr && aItem->Type() == SCH_SYMBOL_T,
                 wxT( "Cannot swap data with invalid symbol." ) );

    SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem;

    std::swap( m_lib_id, symbol->m_lib_id );

    m_pins.swap( symbol->m_pins );      // std::vector's swap()

    for( std::unique_ptr<SCH_PIN>& pin : symbol->m_pins )
        pin->SetParent( symbol );

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
        pin->SetParent( this );

    LIB_SYMBOL* libSymbol = symbol->m_part.release();
    symbol->m_part = std::move( m_part );
    symbol->UpdatePins();
    m_part.reset( libSymbol );
    UpdatePins();

    std::swap( m_pos, symbol->m_pos );
    std::swap( m_unit, symbol->m_unit );
    std::swap( m_convert, symbol->m_convert );

    m_fields.swap( symbol->m_fields );    // std::vector's swap()

    for( SCH_FIELD& field : symbol->m_fields )
        field.SetParent( symbol );

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

    TRANSFORM tmp = m_transform;

    m_transform = symbol->m_transform;
    symbol->m_transform = tmp;

    std::swap( m_excludedFromBOM, symbol->m_excludedFromBOM );
    std::swap( m_DNP, symbol->m_DNP );
    std::swap( m_excludedFromBoard, symbol->m_excludedFromBoard );

    std::swap( m_instanceReferences, symbol->m_instanceReferences );
    std::swap( m_schLibSymbolName, symbol->m_schLibSymbolName );
}


void SCH_SYMBOL::GetContextualTextVars( wxArrayString* aVars ) const
{
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
        aVars->push_back( m_fields[i].GetCanonicalName().Upper() );

    for( size_t i = MANDATORY_FIELDS; i < m_fields.size(); ++i )
        aVars->push_back( m_fields[i].GetName() );

    aVars->push_back( wxT( "OP" ) );
    aVars->push_back( wxT( "FOOTPRINT_LIBRARY" ) );
    aVars->push_back( wxT( "FOOTPRINT_NAME" ) );
    aVars->push_back( wxT( "UNIT" ) );
    aVars->push_back( wxT( "SYMBOL_LIBRARY" ) );
    aVars->push_back( wxT( "SYMBOL_NAME" ) );
    aVars->push_back( wxT( "SYMBOL_DESCRIPTION" ) );
    aVars->push_back( wxT( "SYMBOL_KEYWORDS" ) );
    aVars->push_back( wxT( "EXCLUDE_FROM_BOM" ) );
    aVars->push_back( wxT( "EXCLUDE_FROM_BOARD" ) );
    aVars->push_back( wxT( "DNP" ) );
}


bool SCH_SYMBOL::ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth ) const
{
    static wxRegEx operatingPoint( wxT( "^"
                                        "OP"
                                        "(:[a-zA-Z]*)?"            // port
                                        "(.([0-9])?([a-zA-Z]*))?"  // format
                                        "$" ) );

    SCHEMATIC* schematic = Schematic();

    // SCH_SYMBOL object has no context outside a schematic and the instance on a path.
    if( !schematic || !aPath )
        return false;

    if( operatingPoint.Matches( *token ) )
    {
        wxString port( operatingPoint.GetMatch( *token, 1 ) );
        wxString precisionStr( operatingPoint.GetMatch( *token, 3 ) );
        wxString range( operatingPoint.GetMatch( *token, 4 ) );

        wxString signal = GetRef( aPath ) + port;
        int      precision = 3;

        if( !precisionStr.IsEmpty() )
            precision = precisionStr[0] - '0';

        if( range.IsEmpty() )
        {
            if( port == wxS( ":power" ) )
                range = wxS( "~W" );
            else
                range = wxS( "~A" );
        }

        *token = schematic->GetOperatingPoint( signal.Lower(), precision, range );

        return true;
    }

    if( token->Contains( ':' ) )
    {
        if( schematic->ResolveCrossReference( token, aDepth + 1 ) )
            return true;
    }

    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        if( token->IsSameAs( m_fields[ i ].GetCanonicalName().Upper() ) )
        {
            if( i == REFERENCE_FIELD )
                *token = GetRef( aPath, true );
            else
                *token = m_fields[ i ].GetShownText( aPath, false, aDepth + 1 );

            return true;
        }
    }

    for( size_t i = MANDATORY_FIELDS; i < m_fields.size(); ++i )
    {
        if( token->IsSameAs( m_fields[ i ].GetName() )
            || token->IsSameAs( m_fields[ i ].GetName().Upper() ) )
        {
            *token = m_fields[ i ].GetShownText( aPath, false, aDepth + 1 );
            return true;
        }
    }

    for( const TEMPLATE_FIELDNAME& templateFieldname :
            schematic->Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( token->IsSameAs( templateFieldname.m_Name )
            || token->IsSameAs( templateFieldname.m_Name.Upper() ) )
        {
            // If we didn't find it in the fields list then it isn't set on this symbol.
            // Just return an empty string.
            *token = wxEmptyString;
            return true;
        }
    }

    if( token->IsSameAs( wxT( "FOOTPRINT_LIBRARY" ) ) )
    {
        wxString footprint = GetFootprintFieldText( true, aPath, false );

        wxArrayString parts = wxSplit( footprint, ':' );

        *token = parts[ 0 ];
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        wxString footprint = GetFootprintFieldText( true, aPath, false );

        wxArrayString parts = wxSplit( footprint, ':' );

        *token = parts[ std::min( 1, (int) parts.size() - 1 ) ];
        return true;
    }
    else if( token->IsSameAs( wxT( "UNIT" ) ) )
    {
        int unit = GetUnitSelection( aPath );

        *token = LIB_SYMBOL::SubReference( unit );
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_LIBRARY" ) ) )
    {
        *token = m_lib_id.GetLibNickname();
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_NAME" ) ) )
    {
        *token = m_lib_id.GetLibItemName();
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_DESCRIPTION" ) ) )
    {
        *token = GetDescription();
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_KEYWORDS" ) ) )
    {
        *token = GetKeyWords();
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_BOM" ) ) )
    {
        *token = this->GetExcludedFromBOM() ? _( "Excluded from BOM" )
                                            : wxString( wxT( "" ) );
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_BOARD" ) ) )
    {
        *token = this->GetExcludedFromBoard() ? _( "Excluded from board" )
                                              : wxString( wxT( "" ) );
        return true;
    }
    else if( token->IsSameAs( wxT( "DNP" ) ) )
    {
        *token = this->GetDNP() ? _( "DNP" ) : wxString( wxT( "" ) );
        return true;
    }

    // See if parent can resolve it (this will recurse to ancestors)
    if( aPath->Last() && aPath->Last()->ResolveTextVar( aPath, token, aDepth + 1 ) )
        return true;

    return false;
}


void SCH_SYMBOL::ClearAnnotation( const SCH_SHEET_PATH* aSheetPath, bool aResetPrefix )
{
    if( aSheetPath )
    {
        KIID_PATH path = aSheetPath->Path();

        for( SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
        {
            if( instance.m_Path == path )
            {
                if( instance.m_Reference.IsEmpty() || aResetPrefix )
                    instance.m_Reference = UTIL::GetRefDesUnannotated( m_prefix );
                else
                    instance.m_Reference = UTIL::GetRefDesUnannotated( instance.m_Reference );
            }
        }
    }
    else
    {
        for( SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
        {
            if( instance.m_Reference.IsEmpty() || aResetPrefix)
                instance.m_Reference = UTIL::GetRefDesUnannotated( m_prefix );
            else
                instance.m_Reference = UTIL::GetRefDesUnannotated( instance.m_Reference );
        }
    }

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
        pin->ClearDefaultNetName( aSheetPath );

    // These 2 changes do not work in complex hierarchy.
    // When a clear annotation is made, the calling function must call a
    // UpdateAllScreenReferences for the active sheet.
    // But this call cannot made here.
    wxString currentReference = m_fields[REFERENCE_FIELD].GetText();

    if( currentReference.IsEmpty() || aResetPrefix )
        m_fields[REFERENCE_FIELD].SetText( UTIL::GetRefDesUnannotated( m_prefix ) );
    else
        m_fields[REFERENCE_FIELD].SetText( UTIL::GetRefDesUnannotated( currentReference ) );
}


bool SCH_SYMBOL::AddSheetPathReferenceEntryIfMissing( const KIID_PATH& aSheetPath )
{
    // An empty sheet path is illegal, at a minimum the root sheet UUID must be present.
    wxCHECK( aSheetPath.size() > 0, false );

    for( const SCH_SYMBOL_INSTANCE& instance : m_instanceReferences )
    {
        // if aSheetPath is found, nothing to do:
        if( instance.m_Path == aSheetPath )
            return false;
    }

    // This entry does not exist: add it, with its last-used reference
    AddHierarchicalReference( aSheetPath, m_fields[REFERENCE_FIELD].GetText(), m_unit );
    return true;
}


bool SCH_SYMBOL::ReplaceInstanceSheetPath( const KIID_PATH& aOldSheetPath,
                                           const KIID_PATH& aNewSheetPath )
{
    auto it = std::find_if( m_instanceReferences.begin(), m_instanceReferences.end(),
                [ aOldSheetPath ]( SCH_SYMBOL_INSTANCE& r )->bool
                {
                    return aOldSheetPath == r.m_Path;
                }
            );

    if( it != m_instanceReferences.end() )
    {
        wxLogTrace( traceSchSheetPaths,
                    "Replacing sheet path %s\n  with sheet path %s\n  for symbol %s.",
                    aOldSheetPath.AsString(), aNewSheetPath.AsString(), m_Uuid.AsString() );

        it->m_Path = aNewSheetPath;
        return true;
    }

    wxLogTrace( traceSchSheetPaths,
                "Could not find sheet path %s\n  to replace with sheet path %s\n  for symbol %s.",
                aOldSheetPath.AsString(), aNewSheetPath.AsString(), m_Uuid.AsString() );

    return false;
}


void SCH_SYMBOL::SetOrientation( int aOrientation )
{
    TRANSFORM temp = TRANSFORM();
    bool transform = false;

    switch( aOrientation )
    {
    case SYM_ORIENT_0:
    case SYM_NORMAL:                    // default transform matrix
        m_transform.x1 = 1;
        m_transform.y2 = -1;
        m_transform.x2 = m_transform.y1 = 0;
        break;

    case SYM_ROTATE_COUNTERCLOCKWISE:  // Rotate + (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = 1;
        temp.x2   = -1;
        transform = true;
        break;

    case SYM_ROTATE_CLOCKWISE:          // Rotate - (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = -1;
        temp.x2   = 1;
        transform = true;
        break;

    case SYM_MIRROR_Y:                  // Mirror Y (incremental rotation)
        temp.x1   = -1;
        temp.y2   = 1;
        temp.y1   = temp.x2 = 0;
        transform = true;
        break;

    case SYM_MIRROR_X:                  // Mirror X (incremental rotation)
        temp.x1   = 1;
        temp.y2   = -1;
        temp.y1   = temp.x2 = 0;
        transform = true;
        break;

    case SYM_ORIENT_90:
        SetOrientation( SYM_ORIENT_0 );
        SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
        break;

    case SYM_ORIENT_180:
        SetOrientation( SYM_ORIENT_0 );
        SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
        SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
        break;

    case SYM_ORIENT_270:
        SetOrientation( SYM_ORIENT_0 );
        SetOrientation( SYM_ROTATE_CLOCKWISE );
        break;

    case ( SYM_ORIENT_0 + SYM_MIRROR_X ):
        SetOrientation( SYM_ORIENT_0 );
        SetOrientation( SYM_MIRROR_X );
        break;

    case ( SYM_ORIENT_0 + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_0 );
        SetOrientation( SYM_MIRROR_Y );
        break;

    case ( SYM_ORIENT_90 + SYM_MIRROR_X ):
        SetOrientation( SYM_ORIENT_90 );
        SetOrientation( SYM_MIRROR_X );
        break;

    case ( SYM_ORIENT_90 + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_90 );
        SetOrientation( SYM_MIRROR_Y );
        break;

    case ( SYM_ORIENT_180 + SYM_MIRROR_X ):
        SetOrientation( SYM_ORIENT_180 );
        SetOrientation( SYM_MIRROR_X );
        break;

    case ( SYM_ORIENT_180 + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_180 );
        SetOrientation( SYM_MIRROR_Y );
        break;

    case ( SYM_ORIENT_270 + SYM_MIRROR_X ):
        SetOrientation( SYM_ORIENT_270 );
        SetOrientation( SYM_MIRROR_X );
        break;

    case ( SYM_ORIENT_270 + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_270 );
        SetOrientation( SYM_MIRROR_Y );
        break;

    default:
        transform = false;
        wxFAIL_MSG( "Invalid schematic symbol orientation type." );
        break;
    }

    if( transform )
    {
        /* The new matrix transform is the old matrix transform modified by the
         *  requested transformation, which is the temp transform (rot,
         *  mirror ..) in order to have (in term of matrix transform):
         *     transform coord = new_m_transform * coord
         *  where transform coord is the coord modified by new_m_transform from
         *  the initial value coord.
         *  new_m_transform is computed (from old_m_transform and temp) to
         *  have:
         *     transform coord = old_m_transform * temp
         */
        TRANSFORM newTransform;

        newTransform.x1 = m_transform.x1 * temp.x1 + m_transform.x2 * temp.y1;
        newTransform.y1 = m_transform.y1 * temp.x1 + m_transform.y2 * temp.y1;
        newTransform.x2 = m_transform.x1 * temp.x2 + m_transform.x2 * temp.y2;
        newTransform.y2 = m_transform.y1 * temp.x2 + m_transform.y2 * temp.y2;
        m_transform = newTransform;
    }
}


int SCH_SYMBOL::GetOrientation() const
{
    /*
     * This is slow, but also a bizarre algorithm.  I don't feel like unteasing the algorithm right
     * now, so let's just cache it for the moment.
     */
    if( s_transformToOrientationCache.count( m_transform ) )
        return s_transformToOrientationCache.at( m_transform );

    int rotate_values[] =
    {
        SYM_ORIENT_0,
        SYM_ORIENT_90,
        SYM_ORIENT_180,
        SYM_ORIENT_270,
        SYM_MIRROR_X + SYM_ORIENT_0,
        SYM_MIRROR_X + SYM_ORIENT_90,
        SYM_MIRROR_X + SYM_ORIENT_270,
        SYM_MIRROR_Y,
        SYM_MIRROR_Y + SYM_ORIENT_0,
        SYM_MIRROR_Y + SYM_ORIENT_90,
        SYM_MIRROR_Y + SYM_ORIENT_180,
        SYM_MIRROR_Y + SYM_ORIENT_270
    };

    // Try to find the current transform option:
    TRANSFORM transform = m_transform;
    SCH_SYMBOL temp( *this );

    for( int type_rotate : rotate_values )
    {
        temp.SetOrientation( type_rotate );

        if( transform == temp.GetTransform() )
        {
            s_transformToOrientationCache[m_transform] = type_rotate;
            return type_rotate;
        }
    }

    // Error: orientation not found in list (should not happen)
    wxFAIL_MSG( "Schematic symbol orientation matrix internal error." );

    return SYM_NORMAL;
}


#if defined(DEBUG)

void SCH_SYMBOL::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " ref=\"" << TO_UTF8( GetField( REFERENCE_FIELD )->GetName() )
                                 << '"' << " chipName=\""
                                 << GetLibId().Format() << '"' << m_pos
                                 << " layer=\"" << m_layer
                                 << '"' << ">\n";

    // skip the reference, it's been output already.
    for( int i = 1; i < GetFieldCount();  ++i )
    {
        const wxString& value = GetFields()[i].GetText();

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" << " name=\""
                                             << TO_UTF8( GetFields()[i].GetName() )
                                             << '"' << " value=\""
                                             << TO_UTF8( value ) << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << TO_UTF8( GetClass().Lower() ) << ">\n";
}

#endif


BOX2I SCH_SYMBOL::doGetBoundingBox( bool aIncludePins, bool aIncludeFields ) const
{
    BOX2I    bBox;

    if( m_part )
        bBox = m_part->GetBodyBoundingBox( m_unit, m_convert, aIncludePins, false );
    else
        bBox = dummy()->GetBodyBoundingBox( m_unit, m_convert, aIncludePins, false );

    int x0 = bBox.GetX();
    int xm = bBox.GetRight();

    // We must reverse Y values, because matrix orientation
    // suppose Y axis normal for the library items coordinates,
    // m_transform reverse Y values, but bBox is already reversed!
    int y0 = -bBox.GetY();
    int ym = -bBox.GetBottom();

    // Compute the real Boundary box (rotated, mirrored ...)
    int x1 = m_transform.x1 * x0 + m_transform.y1 * y0;
    int y1 = m_transform.x2 * x0 + m_transform.y2 * y0;
    int x2 = m_transform.x1 * xm + m_transform.y1 * ym;
    int y2 = m_transform.x2 * xm + m_transform.y2 * ym;

    bBox.SetX( x1 );
    bBox.SetY( y1 );
    bBox.SetWidth( x2 - x1 );
    bBox.SetHeight( y2 - y1 );
    bBox.Normalize();

    bBox.Offset( m_pos );

    if( aIncludeFields )
    {
        for( const SCH_FIELD& field : m_fields )
        {
            if( field.IsVisible() )
                bBox.Merge( field.GetBoundingBox() );
        }
    }

    return bBox;
}


BOX2I SCH_SYMBOL::GetBodyBoundingBox() const
{
    return doGetBoundingBox( false, false );
}


BOX2I SCH_SYMBOL::GetBodyAndPinsBoundingBox() const
{
    return doGetBoundingBox( true, false );
}


const BOX2I SCH_SYMBOL::GetBoundingBox() const
{
    return doGetBoundingBox( true, true );
}


void SCH_SYMBOL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );
    SCH_SHEET_PATH* currentSheet = schframe ? &schframe->GetCurrentSheet() : nullptr;

    auto addExcludes =
            [&]()
    {
        wxArrayString msgs;

        if( GetExcludeFromSim() )
            msgs.Add( _( "Simulation" ) );

        if( GetExcludedFromBOM() )
            msgs.Add( _( "BOM" ) );

        if( GetExcludedFromBoard() )
            msgs.Add( _( "Board" ) );

        if( GetDNP() )
            msgs.Add( _( "DNP" ) );

        msg = wxJoin( msgs, '|' );
        msg.Replace( '|', wxS( ", " ) );

        if( !msg.empty() )
            aList.emplace_back( _( "Exclude from" ), msg );
    };

    // part and alias can differ if alias is not the root
    if( m_part )
    {
        if( m_part.get() != dummy() )
        {
            if( m_part->IsPower() )
            {
                // Don't use GetShownText(); we want to see the variable references here
                aList.emplace_back( _( "Power symbol" ),
                                    KIUI::EllipsizeStatusText( aFrame, GetField( VALUE_FIELD )->GetText() ) );
            }
            else
            {
                aList.emplace_back( _( "Reference" ),
                                    UnescapeString( GetRef( currentSheet ) ) );

                // Don't use GetShownText(); we want to see the variable references here
                aList.emplace_back( _( "Value" ),
                                    KIUI::EllipsizeStatusText( aFrame, GetField( VALUE_FIELD )->GetText() ) );
                addExcludes();
                aList.emplace_back( _( "Name" ),
                                    KIUI::EllipsizeStatusText( aFrame, GetLibId().GetLibItemName() ) );
            }

#if 0       // Display symbol flags, for debug only
            aList.emplace_back( _( "flags" ), wxString::Format( "%X", GetEditFlags() ) );
#endif

            if( !m_part->IsRoot() )
            {
                msg = _( "Missing parent" );

                std::shared_ptr< LIB_SYMBOL > parent = m_part->GetParent().lock();

                if( parent )
                    msg = parent->GetName();

                aList.emplace_back( _( "Derived from" ), UnescapeString( msg ) );
            }
            else if( !m_lib_id.GetLibNickname().empty() )
            {
                aList.emplace_back( _( "Library" ), m_lib_id.GetLibNickname() );
            }
            else
            {
                aList.emplace_back( _( "Library" ), _( "Undefined!!!" ) );
            }

            // Display the current associated footprint, if exists.
            // Don't use GetShownText(); we want to see the variable references here
            msg = KIUI::EllipsizeStatusText( aFrame, GetField( FOOTPRINT_FIELD )->GetText() );

            if( msg.IsEmpty() )
                msg = _( "<Unknown>" );

            aList.emplace_back( _( "Footprint" ), msg );

            // Display description of the symbol, and keywords found in lib
            aList.emplace_back( _( "Description" ) + wxT( ": " ) + m_part->GetDescription(),
                                _( "Keywords" ) + wxT( ": " ) + m_part->GetKeyWords() );
        }
    }
    else
    {
        aList.emplace_back( _( "Reference" ), GetRef( currentSheet ) );
        // Don't use GetShownText(); we want to see the variable references here
        aList.emplace_back( _( "Value" ),
                            KIUI::EllipsizeStatusText( aFrame, GetField( VALUE_FIELD )->GetText() ) );
        addExcludes();
        aList.emplace_back( _( "Name" ),
                            KIUI::EllipsizeStatusText( aFrame, GetLibId().GetLibItemName() ) );

        wxString libNickname = GetLibId().GetLibNickname();

        if( libNickname.empty() )
            msg = _( "No library defined!" );
        else
            msg.Printf( _( "Symbol not found in %s!" ), libNickname );

        aList.emplace_back( _( "Library" ), msg );
    }
}


BITMAPS SCH_SYMBOL::GetMenuImage() const
{
    return BITMAPS::add_component;
}


void SCH_SYMBOL::MirrorHorizontally( int aCenter )
{
    int dx = m_pos.x;

    SetOrientation( SYM_MIRROR_Y );
    MIRROR( m_pos.x, aCenter );
    dx -= m_pos.x;     // dx,0 is the move vector for this transform

    for( SCH_FIELD& field : m_fields )
    {
        // Move the fields to the new position because the symbol itself has moved.
        VECTOR2I pos = field.GetTextPos();
        pos.x -= dx;
        field.SetTextPos( pos );
    }
}


void SCH_SYMBOL::MirrorVertically( int aCenter )
{
    int dy = m_pos.y;

    SetOrientation( SYM_MIRROR_X );
    MIRROR( m_pos.y, aCenter );
    dy -= m_pos.y;     // 0,dy is the move vector for this transform

    for( SCH_FIELD& field : m_fields )
    {
        // Move the fields to the new position because the symbol itself has moved.
        VECTOR2I pos = field.GetTextPos();
        pos.y -= dy;
        field.SetTextPos( pos );
    }
}


void SCH_SYMBOL::Rotate( const VECTOR2I& aCenter )
{
    VECTOR2I prev = m_pos;

    RotatePoint( m_pos, aCenter, ANGLE_90 );

    SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );

    for( SCH_FIELD& field : m_fields )
    {
        // Move the fields to the new position because the symbol itself has moved.
        VECTOR2I pos = field.GetTextPos();
        pos.x -= prev.x - m_pos.x;
        pos.y -= prev.y - m_pos.y;
        field.SetTextPos( pos );
    }
}


bool SCH_SYMBOL::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    // Symbols are searchable via the child field and pin item text.
    return false;
}


void SCH_SYMBOL::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    for( auto& pin : m_pins )
    {
        LIB_PIN* lib_pin = pin->GetLibPin();

        if( lib_pin->GetUnit() && m_unit && ( m_unit != lib_pin->GetUnit() ) )
            continue;

        DANGLING_END_ITEM item( PIN_END, lib_pin, GetPinPhysicalPosition( lib_pin ), this );
        aItemList.push_back( item );
    }
}


bool SCH_SYMBOL::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                      const SCH_SHEET_PATH* aPath )
{
    bool changed = false;

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        bool previousState = pin->IsDangling();
        pin->SetIsDangling( true );

        VECTOR2I pos = m_transform.TransformCoordinate( pin->GetLocalPosition() ) + m_pos;

        for( DANGLING_END_ITEM& each_item : aItemList )
        {
            // Some people like to stack pins on top of each other in a symbol to indicate
            // internal connection. While technically connected, it is not particularly useful
            // to display them that way, so skip any pins that are in the same symbol as this
            // one.
            if( each_item.GetParent() == this )
                continue;

            switch( each_item.GetType() )
            {
            case PIN_END:
            case LABEL_END:
            case SHEET_LABEL_END:
            case WIRE_END:
            case NO_CONNECT_END:
            case JUNCTION_END:

                if( pos == each_item.GetPosition() )
                    pin->SetIsDangling( false );

                break;

            default:
                break;
            }

            if( !pin->IsDangling() )
                break;
        }

        changed = ( changed || ( previousState != pin->IsDangling() ) );
    }

    return changed;
}


VECTOR2I SCH_SYMBOL::GetPinPhysicalPosition( const LIB_PIN* Pin ) const
{
    wxCHECK_MSG( Pin != nullptr && Pin->Type() == LIB_PIN_T, VECTOR2I( 0, 0 ),
                 wxT( "Cannot get physical position of pin." ) );

    return m_transform.TransformCoordinate( Pin->GetPosition() ) + m_pos;
}


std::vector<VECTOR2I> SCH_SYMBOL::GetConnectionPoints() const
{
    std::vector<VECTOR2I> retval;

    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        // Collect only pins attached to the current unit and convert.
        // others are not associated to this symbol instance
        int pin_unit = pin->GetLibPin()->GetUnit();
        int pin_convert = pin->GetLibPin()->GetConvert();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_convert > 0 && pin_convert != GetConvert() )
            continue;

        retval.push_back( m_transform.TransformCoordinate( pin->GetLocalPosition() ) + m_pos );
    }

    return retval;
}


LIB_ITEM* SCH_SYMBOL::GetDrawItem( const VECTOR2I& aPosition, KICAD_T aType )
{
    if( m_part )
    {
        // Calculate the position relative to the symbol.
        VECTOR2I libPosition = aPosition - m_pos;

        return m_part->LocateDrawItem( m_unit, m_convert, aType, libPosition, m_transform );
    }

    return nullptr;
}


wxString SCH_SYMBOL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Symbol %s [%s]" ),
                             KIUI::EllipsizeMenuText( GetField( REFERENCE_FIELD )->GetText() ),
                             KIUI::EllipsizeMenuText( GetLibId().GetLibItemName() ) );
}


INSPECT_RESULT SCH_SYMBOL::Visit( INSPECTOR aInspector, void* aTestData,
                                  const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == SCH_LOCATE_ANY_T
            || ( scanType == SCH_SYMBOL_T )
            || ( scanType == SCH_SYMBOL_LOCATE_POWER_T && m_part && m_part->IsPower() ) )
        {
            if( INSPECT_RESULT::QUIT == aInspector( this, aTestData ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_FIELD_T )
        {
            for( SCH_FIELD& field : m_fields )
            {
                if( INSPECT_RESULT::QUIT == aInspector( &field, (void*) this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }

        if( scanType == SCH_FIELD_LOCATE_REFERENCE_T )
        {
            if( INSPECT_RESULT::QUIT == aInspector( GetField( REFERENCE_FIELD ), (void*) this ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_FIELD_LOCATE_VALUE_T
            || ( scanType == SCH_SYMBOL_LOCATE_POWER_T && m_part && m_part->IsPower() ) )
        {
            if( INSPECT_RESULT::QUIT == aInspector( GetField( VALUE_FIELD ), (void*) this ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_FIELD_LOCATE_FOOTPRINT_T )
        {
            if( INSPECT_RESULT::QUIT == aInspector( GetField( FOOTPRINT_FIELD ), (void*) this ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_FIELD_LOCATE_DATASHEET_T )
        {
            if( INSPECT_RESULT::QUIT == aInspector( GetField( DATASHEET_FIELD ), (void*) this ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_PIN_T )
        {
            for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
            {
                // Collect only pins attached to the current unit and convert.
                // others are not associated to this symbol instance
                int pin_unit = pin->GetLibPin()->GetUnit();
                int pin_convert = pin->GetLibPin()->GetConvert();

                if( pin_unit > 0 && pin_unit != GetUnit() )
                    continue;

                if( pin_convert > 0 && pin_convert != GetConvert() )
                    continue;

                if( INSPECT_RESULT::QUIT == aInspector( pin.get(), (void*) this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


bool SCH_SYMBOL::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto symbol = static_cast<const SCH_SYMBOL*>( &aItem );

    BOX2I rect = GetBodyAndPinsBoundingBox();

    if( rect.GetArea() != symbol->GetBodyAndPinsBoundingBox().GetArea() )
        return rect.GetArea() < symbol->GetBodyAndPinsBoundingBox().GetArea();

    if( m_pos.x != symbol->m_pos.x )
        return m_pos.x < symbol->m_pos.x;

    if( m_pos.y != symbol->m_pos.y )
        return m_pos.y < symbol->m_pos.y;

    return m_Uuid < aItem.m_Uuid;       // Ensure deterministic sort
}


bool SCH_SYMBOL::operator==( const SCH_SYMBOL& aSymbol ) const
{
    if( GetFieldCount() !=  aSymbol.GetFieldCount() )
        return false;

    for( int i = VALUE_FIELD; i < GetFieldCount(); i++ )
    {
        if( GetFields()[i].GetText().Cmp( aSymbol.GetFields()[i].GetText() ) != 0 )
            return false;
    }

    return true;
}


bool SCH_SYMBOL::operator!=( const SCH_SYMBOL& aSymbol ) const
{
    return !( *this == aSymbol );
}


SCH_SYMBOL& SCH_SYMBOL::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_SYMBOL* c = (SCH_SYMBOL*) &aItem;

        m_lib_id    = c->m_lib_id;

        LIB_SYMBOL* libSymbol = c->m_part ? new LIB_SYMBOL( *c->m_part.get() ) : nullptr;

        m_part.reset( libSymbol );
        m_pos       = c->m_pos;
        m_unit      = c->m_unit;
        m_convert   = c->m_convert;
        m_transform = c->m_transform;

        m_instanceReferences = c->m_instanceReferences;

        m_fields    = c->m_fields;    // std::vector's assignment operator

        // Reparent fields after assignment to new symbol.
        for( SCH_FIELD& field : m_fields )
            field.SetParent( this );

        UpdatePins();
    }

    return *this;
}


bool SCH_SYMBOL::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I bBox = GetBodyBoundingBox();
    bBox.Inflate( aAccuracy / 2 );

    if( bBox.Contains( aPosition ) )
        return true;

    return false;
}


bool SCH_SYMBOL::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & STRUCT_DELETED || m_flags & SKIP_STRUCT )
        return false;

    BOX2I rect = aRect;

    rect.Inflate( aAccuracy / 2 );

    if( aContained )
        return rect.Contains( GetBodyBoundingBox() );

    return rect.Intersects( GetBodyBoundingBox() );
}


bool SCH_SYMBOL::doIsConnected( const VECTOR2I& aPosition ) const
{
    VECTOR2I new_pos = m_transform.InverseTransform().TransformCoordinate( aPosition - m_pos );

    for( const auto& pin : m_pins )
    {
        if( pin->GetType() == ELECTRICAL_PINTYPE::PT_NC )
            continue;

        // Collect only pins attached to the current unit and convert.
        // others are not associated to this symbol instance
        int pin_unit = pin->GetLibPin()->GetUnit();
        int pin_convert = pin->GetLibPin()->GetConvert();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_convert > 0 && pin_convert != GetConvert() )
            continue;

        if( pin->GetLocalPosition() == new_pos )
            return true;
    }

    return false;
}


bool SCH_SYMBOL::IsInNetlist() const
{
    return m_isInNetlist;
}


void SCH_SYMBOL::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( aBackground )
        return;

    if( m_part )
    {
        LIB_PINS  libPins;
        m_part->GetPins( libPins, GetUnit(), GetConvert() );

        // Copy the source so we can re-orient and translate it.
        LIB_SYMBOL tempSymbol( *m_part );
        LIB_PINS tempPins;
        tempSymbol.GetPins( tempPins, GetUnit(), GetConvert() );

        // Copy the pin info from the symbol to the temp pins
        for( unsigned i = 0; i < tempPins.size(); ++ i )
        {
            SCH_PIN* symbolPin = GetPin( libPins[ i ] );
            LIB_PIN* tempPin = tempPins[ i ];

            tempPin->SetName( symbolPin->GetShownName() );
            tempPin->SetType( symbolPin->GetType() );
            tempPin->SetShape( symbolPin->GetShape() );

            if( symbolPin->IsDangling() )
                tempPin->SetFlags( IS_DANGLING );
        }

        TRANSFORM temp = GetTransform();
        aPlotter->StartBlock( nullptr );

        for( bool local_background : { true, false } )
        {
            tempSymbol.Plot( aPlotter, GetUnit(), GetConvert(), local_background, m_pos, temp,
                             GetDNP() );

            for( const SCH_FIELD& field : m_fields )
                field.Plot( aPlotter, local_background );
        }

        if( m_DNP )
            PlotDNP( aPlotter );

        // Plot attributes to a hypertext menu
        std::vector<wxString> properties;
        SCH_SHEET_PATH*       sheet = &Schematic()->CurrentSheet();

        for( const SCH_FIELD& field : GetFields() )
        {
            wxString text_field = field.GetShownText( sheet, false);

            if( text_field.IsEmpty() )
                continue;

            properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                       field.GetName(), text_field ) );
        }

        if( !m_part->GetKeyWords().IsEmpty() )
        {
            properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                       _( "Keywords" ),
                                                       m_part->GetKeyWords() ) );
        }

        aPlotter->HyperlinkMenu( GetBoundingBox(), properties );



        aPlotter->EndBlock( nullptr );

        if( !m_part->IsPower() )
            aPlotter->Bookmark( GetBoundingBox(), GetRef( sheet ), _( "Symbols" ) );
    }
}


void SCH_SYMBOL::PlotDNP( PLOTTER* aPlotter ) const
{
    BOX2I bbox = GetBodyAndPinsBoundingBox();

    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    aPlotter->SetColor( colors->GetColor( LAYER_DNP_MARKER ) );

    aPlotter->ThickSegment( bbox.GetOrigin(), bbox.GetEnd(),
                            3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ),
                            FILLED, nullptr );

    aPlotter->ThickSegment( bbox.GetOrigin() + VECTOR2I( bbox.GetWidth(), 0 ),
                            bbox.GetOrigin() + VECTOR2I( 0, bbox.GetHeight() ),
                            3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ),
                            FILLED, nullptr );
}


void SCH_SYMBOL::PlotPins( PLOTTER* aPlotter ) const
{
    if( m_part )
    {
        LIB_PINS  libPins;
        m_part->GetPins( libPins, GetUnit(), GetConvert() );

        // Copy the source to stay const
        LIB_SYMBOL tempSymbol( *m_part );
        LIB_PINS tempPins;
        tempSymbol.GetPins( tempPins, GetUnit(), GetConvert() );

        TRANSFORM transform = GetTransform();

        // Copy the pin info from the symbol to the temp pins
        for( unsigned i = 0; i < tempPins.size(); ++ i )
        {
            SCH_PIN* symbolPin = GetPin( libPins[ i ] );
            LIB_PIN* tempPin = tempPins[ i ];

            tempPin->SetName( symbolPin->GetShownName() );
            tempPin->SetType( symbolPin->GetType() );
            tempPin->SetShape( symbolPin->GetShape() );
            tempPin->Plot( aPlotter, false, m_pos, transform, GetDNP() );
        }
    }
}


bool SCH_SYMBOL::HasBrightenedPins()
{
    for( const auto& pin : m_pins )
    {
        if( pin->IsBrightened() )
            return true;
    }

    return false;
}


void SCH_SYMBOL::ClearBrightenedPins()
{
    for( auto& pin : m_pins )
        pin->ClearBrightened();
}


bool SCH_SYMBOL::IsPointClickableAnchor( const VECTOR2I& aPos ) const
{
    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        int pin_unit = pin->GetLibPin()->GetUnit();
        int pin_convert = pin->GetLibPin()->GetConvert();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_convert > 0 && pin_convert != GetConvert() )
            continue;

        if( pin->IsPointClickableAnchor( aPos ) )
            return true;
    }

    return false;
}


bool SCH_SYMBOL::IsSymbolLikePowerGlobalLabel() const
{
    // return true if the symbol is equivalent to a global label:
    // It is a Power symbol
    // It has only one pin type Power input

    if( !GetLibSymbolRef() || !GetLibSymbolRef()->IsPower() )
        return false;

    std::vector<LIB_PIN*> pin_list = GetAllLibPins();

    if( pin_list.size() != 1 )
        return false;

    return pin_list[0]->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN;
}


bool SCH_SYMBOL::IsPower() const
{
    wxCHECK( m_part, false );

    return m_part->IsPower();
}


static struct SCH_SYMBOL_DESC
{
    SCH_SYMBOL_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_SYMBOL );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SYMBOL ), TYPE_HASH( SCH_ITEM ) );

        const wxString groupAttributes = _HKI( "Attributes" );

        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, bool>( _HKI( "Exclude from board" ),
                &SCH_SYMBOL::SetExcludedFromBoard, &SCH_SYMBOL::GetExcludedFromBoard ),
                groupAttributes );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, bool>( _HKI( "Exclude from simulation" ),
                &SCH_SYMBOL::SetExcludeFromSim, &SCH_SYMBOL::GetExcludeFromSim ),
                groupAttributes );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, bool>( _HKI( "Exclude from bill of materials" ),
                &SCH_SYMBOL::SetExcludedFromBOM, &SCH_SYMBOL::GetExcludedFromBOM ),
                groupAttributes );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, bool>( _HKI( "Do not populate" ),
                &SCH_SYMBOL::SetDNP, &SCH_SYMBOL::GetDNP ),
                groupAttributes );
    }
} _SCH_SYMBOL_DESC;
