/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_shape.h>
#include <pgm_base.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <sim/sim_model.h>
#include <sim/spice_generator.h>
#include <sim/sim_lib_mgr.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <refdes_utils.h>
#include <wx/log.h>
#include <settings/settings_manager.h>
#include <sch_plotter.h>
#include <string_utils.h>

#include <utility>


std::unordered_map<TRANSFORM, int> SCH_SYMBOL::s_transformToOrientationCache;


/**
 * Convert a wxString to UTF8 and replace any control characters with a ~, where a control
 * character is one of the first ASCII values up to ' ' 32d.
 */
std::string toUTFTildaText( const wxString& txt )
{
    std::string ret = TO_UTF8( txt );

    for( char& c : ret )
    {
        if( (unsigned char) c <= ' ' )
            c = '~';
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

        SCH_SHAPE* square = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

        square->SetPosition( VECTOR2I( schIUScale.MilsToIU( -200 ), schIUScale.MilsToIU( 200 ) ) );
        square->SetEnd( VECTOR2I( schIUScale.MilsToIU( 200 ), schIUScale.MilsToIU( -200 ) ) );
        symbol->AddDrawItem( square );

        SCH_TEXT* text = new SCH_TEXT( { 0, 0 }, wxT( "??"), LAYER_DEVICE );

        text->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 150 ), schIUScale.MilsToIU( 150 ) ) );
        symbol->AddDrawItem( text );
    }

    return symbol;
}


SCH_SYMBOL::SCH_SYMBOL() :
    SYMBOL( nullptr, SCH_SYMBOL_T )
{
    Init( VECTOR2I( 0, 0 ) );
}


SCH_SYMBOL::SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const LIB_ID& aLibId,
                        const SCH_SHEET_PATH* aSheet, int aUnit, int aBodyStyle,
                        const VECTOR2I& aPosition, EDA_ITEM* aParent ) :
        SYMBOL( aParent, SCH_SYMBOL_T )
{
    Init( aPosition );

    m_unit      = aUnit;
    m_bodyStyle = aBodyStyle;
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
    m_excludedFromSim = m_part->GetExcludedFromSim();
    m_excludedFromBOM = m_part->GetExcludedFromBOM();
    m_excludedFromBoard = m_part->GetExcludedFromBoard();
}


SCH_SYMBOL::SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const SCH_SHEET_PATH* aSheet,
                        const PICKED_SYMBOL& aSel, const VECTOR2I& aPosition, EDA_ITEM* aParent ) :
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
    SYMBOL( aSymbol )
{
    m_parent      = aSymbol.m_parent;
    m_pos         = aSymbol.m_pos;
    m_unit        = aSymbol.m_unit;
    m_bodyStyle   = aSymbol.m_bodyStyle;
    m_lib_id      = aSymbol.m_lib_id;
    m_isInNetlist = aSymbol.m_isInNetlist;
    m_DNP         = aSymbol.m_DNP;

    const_cast<KIID&>( m_Uuid ) = aSymbol.m_Uuid;

    m_transform = aSymbol.m_transform;
    m_prefix = aSymbol.m_prefix;
    m_instanceReferences = aSymbol.m_instanceReferences;
    m_fields = aSymbol.m_fields;

    // Re-parent the fields, which before this had aSymbol as parent
    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

    m_pins.clear();

    // Copy (and re-parent) the pins
    for( const std::unique_ptr<SCH_PIN>& pin : aSymbol.m_pins )
    {
        m_pins.emplace_back( std::make_unique<SCH_PIN>( *pin ) );
        m_pins.back()->SetParent( this );
    }

    if( aSymbol.m_part )
        SetLibSymbol( new LIB_SYMBOL( *aSymbol.m_part ) );

    m_fieldsAutoplaced = aSymbol.m_fieldsAutoplaced;
    m_schLibSymbolName = aSymbol.m_schLibSymbolName;
}


void SCH_SYMBOL::Init( const VECTOR2I& pos )
{
    m_layer     = LAYER_DEVICE;
    m_pos       = pos;
    m_unit      = 1;  // In multi unit chip - which unit to draw.
    m_bodyStyle = BODY_STYLE::BASE;  // De Morgan Handling

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
}


EDA_ITEM* SCH_SYMBOL::Clone() const
{
    return new SCH_SYMBOL( *this );
}


bool SCH_SYMBOL::IsMissingLibSymbol() const
{
    return m_part == nullptr;
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
    wxCHECK2( !aLibSymbol || aLibSymbol->IsRoot(), aLibSymbol = nullptr );

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
    std::map<wxString, wxString>            altPinMap;
    std::map<wxString, std::set<SCH_PIN*>>  pinUuidMap;
    std::set<SCH_PIN*>                      unassignedSchPins;
    std::set<LIB_PIN*>                      unassignedLibPins;

    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        pinUuidMap[ pin->GetNumber() ].insert( pin.get() );

        unassignedSchPins.insert( pin.get() );

        if( !pin->GetAlt().IsEmpty() )
            altPinMap[ pin->GetNumber() ] = pin->GetAlt();

        pin->SetLibPin( nullptr );
    }

    m_pinMap.clear();

    if( !m_part )
        return;

    std::vector<LIB_PIN*> pins = m_part->GetAllLibPins();

    for( LIB_PIN* libPin : pins )
    {
        // NW: Don't filter by unit: this data-structure is used for all instances,
        // some of which might have different units.
        if( libPin->GetBodyStyle() && m_bodyStyle && m_bodyStyle != libPin->GetBodyStyle() )
            continue;

        SCH_PIN* pin = nullptr;

        auto ii = pinUuidMap.find( libPin->GetNumber() );

        if( ii == pinUuidMap.end() || ii->second.empty() )
        {
            unassignedLibPins.insert( libPin );
            continue;
        }

        auto it = ii->second.begin();
        pin = *it;
        ii->second.erase( it );
        pin->SetLibPin( libPin );
        pin->SetPosition( libPin->GetPosition() );

        unassignedSchPins.erase( pin );

        auto iii = altPinMap.find( libPin->GetNumber() );

        if( iii != altPinMap.end() )
            pin->SetAlt( iii->second );

        m_pinMap[ libPin ] = pin;
    }

    // Add any pins that were not found in the symbol
    for( LIB_PIN* libPin : unassignedLibPins )
    {
        SCH_PIN* pin = nullptr;

        // First try to re-use an existing pin
        if( !unassignedSchPins.empty() )
        {
            auto it = unassignedSchPins.begin();
            pin = *it;
            unassignedSchPins.erase( it );
        }
        else
        {
            // This is a pin that was not found in the symbol, so create a new one.
            pin = m_pins.emplace_back( std::make_unique<SCH_PIN>( SCH_PIN( libPin, this ) ) ).get();
        }

        m_pinMap[ libPin ] = pin;
        pin->SetLibPin( libPin );
        pin->SetPosition( libPin->GetPosition() );
        pin->SetNumber( libPin->GetNumber() );

        auto iii = altPinMap.find( libPin->GetNumber() );

        if( iii != altPinMap.end() )
            pin->SetAlt( iii->second );
    }

    // If we have any pins left in the symbol that were not found in the library, remove them.
    for( auto it1 = m_pins.begin(); it1 != m_pins.end() && !unassignedSchPins.empty(); )
    {
        auto it2 = unassignedSchPins.find( it1->get() );

        if( it2 != unassignedSchPins.end() )
        {
            it1 = m_pins.erase( it1 );
            unassignedSchPins.erase( it2 );
        }
        else
        {
            ++it1;
        }
    }

    // If the symbol is selected, then its pins are selected.
    if( IsSelected() )
    {
        for( std::unique_ptr<SCH_PIN>& pin : m_pins )
            pin->SetSelected();
    }

}


void SCH_SYMBOL::SetBodyStyle( int aBodyStyle )
{
    if( m_bodyStyle != aBodyStyle )
    {
        m_bodyStyle = aBodyStyle;

        // The convert may have a different pin layout so the update the pin map.
        UpdatePins();
    }
}


bool SCH_SYMBOL::HasAlternateBodyStyle() const
{
    if( m_part )
        return m_part->HasAlternateBodyStyle();

    return false;
}


void SCH_SYMBOL::SetTransform( const TRANSFORM& aTransform )
{
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
    wxCHECK( m_part, ( wxString::Format( _( "Unit %s" ), SubReference( aUnit ) ) ) );

    return m_part->GetUnitDisplayName( aUnit );
}


bool SCH_SYMBOL::HasUnitDisplayName( int aUnit )
{
    wxCHECK( m_part, false );

    return m_part->HasUnitDisplayName( aUnit );
}


void SCH_SYMBOL::PrintBackground( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                                  const VECTOR2I& aOffset, bool aDimmed )
{
    wxCHECK( m_part, /* void */ );

    SCH_RENDER_SETTINGS localRenderSettings( *aSettings );
    localRenderSettings.m_Transform = m_transform;
    localRenderSettings.m_ShowVisibleFields = false;
    localRenderSettings.m_ShowHiddenFields = false;

    if( GetDNP() )
        aDimmed = true;

    m_part->PrintBackground( &localRenderSettings, m_unit, m_bodyStyle, m_pos + aOffset, aDimmed );
}


void SCH_SYMBOL::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                        const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    SCH_RENDER_SETTINGS localRenderSettings( *aSettings );
    localRenderSettings.m_Transform = m_transform;
    localRenderSettings.m_ShowVisibleFields = false;
    localRenderSettings.m_ShowHiddenFields = false;

    if( m_DNP )
        aDimmed = true;

    if( m_part )
    {
        std::vector<LIB_PIN*>  libPins;
        m_part->GetPins( libPins, m_unit, m_bodyStyle );

        LIB_SYMBOL tempSymbol( *m_part );
        std::vector<LIB_PIN*> tempPins;
        tempSymbol.GetPins( tempPins, m_unit, m_bodyStyle );

        // Copy the pin info from the symbol to the temp pins
        for( unsigned i = 0; i < tempPins.size(); ++ i )
        {
            SCH_PIN* symbolPin = GetPin( libPins[ i ] );
            LIB_PIN* tempPin = tempPins[ i ];

            tempPin->SetName( symbolPin->GetShownName() );
            tempPin->SetType( symbolPin->GetType() );
            tempPin->SetShape( symbolPin->GetShape() );
        }

        for( SCH_ITEM& item : tempSymbol.GetDrawItems() )
        {
            if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( &item ) )
            {
                // Use SCH_FIELD's text resolver
                SCH_FIELD dummy( this, -1 );
                dummy.SetText( text->GetText() );
                text->SetText( dummy.GetShownText( false ) );
            }
        }

        tempSymbol.Print( &localRenderSettings, m_unit, m_bodyStyle, m_pos + aOffset, false,
                          aDimmed );
    }
    else    // Use dummy() part if the actual cannot be found.
    {
        dummy()->Print( &localRenderSettings, 0, 0, m_pos + aOffset, aForceNoFill, aDimmed );
    }

    for( SCH_FIELD& field : m_fields )
        field.Print( &localRenderSettings, m_unit, m_bodyStyle, aOffset, aForceNoFill, aDimmed );

    if( m_DNP )
    {
        BOX2I bbox = GetBodyAndPinsBoundingBox();
        wxDC* DC = localRenderSettings.GetPrintDC();
        COLOR4D dnp_color = localRenderSettings.GetLayerColor( LAYER_DNP_MARKER );

        GRFilledSegment( DC, bbox.GetOrigin(), bbox.GetEnd(),
                             3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ),
                             dnp_color );

        GRFilledSegment( DC, bbox.GetOrigin() + VECTOR2I( bbox.GetWidth(), 0 ),
                             bbox.GetOrigin() + VECTOR2I( 0, bbox.GetHeight() ),
                             3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ),
                             dnp_color );
    }
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
    RemoveInstance( aInstancePath.Path() );
}


void SCH_SYMBOL::RemoveInstance( const KIID_PATH& aInstancePath )
{
    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii < m_instanceReferences.size(); ii++ )
    {
        if( m_instanceReferences[ii].m_Path == aInstancePath )
        {
            wxLogTrace( traceSchSheetPaths, wxS( "Removing symbol instance:\n"
                                                 "  sheet path %s\n"
                                                 "  reference %s, unit %d from symbol %s." ),
                        aInstancePath.AsString(),
                        m_instanceReferences[ii].m_Reference,
                        m_instanceReferences[ii].m_Unit,
                        m_Uuid.AsString() );

            m_instanceReferences.erase( m_instanceReferences.begin() + ii );
            ii--;
        }
    }
}


void SCH_SYMBOL::AddHierarchicalReference( const KIID_PATH& aPath, const wxString& aRef, int aUnit )
{
    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii < m_instanceReferences.size(); ii++ )
    {
        if( m_instanceReferences[ii].m_Path == aPath )
        {
            wxLogTrace( traceSchSheetPaths, wxS( "Removing symbol instance:\n"
                                                 "  sheet path %s\n"
                                                 "  reference %s, unit %d from symbol %s." ),
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

    wxLogTrace( traceSchSheetPaths, wxS( "Adding symbol '%s' instance:\n"
                                         "    sheet path '%s'\n"
                                         "    reference '%s'\n"
                                         "    unit %d\n" ),
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
            wxLogTrace( traceSchSheetPaths, wxS( "Removing symbol instance:\n"
                                                 "  sheet path %s\n"
                                                 "  reference %s, unit %d from symbol %s." ),
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

    wxLogTrace( traceSchSheetPaths, wxS( "Adding symbol '%s' instance:\n"
                                         "    sheet path '%s'\n"
                                         "    reference '%s'\n"
                                         "    unit %d\n" ),
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
            subRef = SubReference( instance.m_Unit );
            break;
        }
    }

    // If it was not found in m_Paths array, then see if it is in m_Field[REFERENCE] -- if so,
    // use this as a default for this path.  This will happen if we load a version 1 schematic
    // file.  It will also mean that multiple instances of the same sheet by default all have
    // the same symbol references, but perhaps this is best.
    if( ref.IsEmpty() && !GetField( REFERENCE_FIELD )->GetText().IsEmpty() )
        ref = GetField( REFERENCE_FIELD )->GetText();

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


bool SCH_SYMBOL::IsAnnotated( const SCH_SHEET_PATH* aSheet ) const
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


wxString SCH_SYMBOL::SubReference( int aUnit, bool aAddSeparator ) const
{
    if( SCHEMATIC* schematic = Schematic() )
        return schematic->Settings().SubReference( aUnit, aAddSeparator );

    return LIB_SYMBOL::LetterSubReference( aUnit, 'A' );
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


const wxString SCH_SYMBOL::GetValue( bool aResolve, const SCH_SHEET_PATH* aPath,
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
    for( SCH_FIELD& field : m_fields )
    {
        if( field.GetId() == aFieldId )
            return &field;
    }

    return nullptr;
}


SCH_FIELD* SCH_SYMBOL::GetFieldByName( const wxString& aFieldName )
{
    for( SCH_FIELD& field : m_fields )
    {
        if( field.GetName() == aFieldName )
            return &field;
    }

    return nullptr;
}


const SCH_FIELD* SCH_SYMBOL::GetFieldByName( const wxString& aFieldName ) const
{
    for( const SCH_FIELD& field : m_fields )
    {
        if( field.GetName() == aFieldName )
            return &field;
    }

    return nullptr;
}


void SCH_SYMBOL::GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly )
{
    for( SCH_FIELD& field : m_fields )
    {
        if( aVisibleOnly )
        {
            if( !field.IsVisible() || field.GetText().IsEmpty() )
                continue;
        }

        aVector.push_back( &field );
    }
}


SCH_FIELD* SCH_SYMBOL::AddField( const SCH_FIELD& aField )
{
    m_fields.push_back( aField );
    return &m_fields.back();
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
        std::vector<SCH_FIELD*> fields;
        m_part->GetFields( fields );

        for( const SCH_FIELD* libField : fields )
        {
            int        id = libField->GetId();
            SCH_FIELD* schField;

            if( libField->IsMandatory() )
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
        m_part->GetPins( aPinsList, m_unit, m_bodyStyle );
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
    auto it = m_pinMap.find( aLibPin );

    if( it != m_pinMap.end() )
        return it->second;

    wxFAIL_MSG_AT( "Pin not found", __FILE__, __LINE__, __FUNCTION__ );
    return nullptr;
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
        if( unit && p->GetLibPin() && p->GetLibPin()->GetUnit()
            && ( p->GetLibPin()->GetUnit() != unit ) )
        {
            continue;
        }

        pins.push_back( p.get() );
    }

    return pins;
}


void SCH_SYMBOL::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    wxCHECK_RET( aItem != nullptr && aItem->Type() == SCH_SYMBOL_T,
                 wxT( "Cannot swap data with invalid symbol." ) );

    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( aItem );

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
    std::swap( m_bodyStyle, symbol->m_bodyStyle );

    m_fields.swap( symbol->m_fields );    // std::vector's swap()

    for( SCH_FIELD& field : symbol->m_fields )
        field.SetParent( symbol );

    for( SCH_FIELD& field : m_fields )
        field.SetParent( this );

    TRANSFORM tmp = m_transform;

    m_transform = symbol->m_transform;
    symbol->m_transform = tmp;

    std::swap( m_excludedFromSim, symbol->m_excludedFromSim );
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
    aVars->push_back( wxT( "EXCLUDE_FROM_SIM" ) );
    aVars->push_back( wxT( "DNP" ) );
    aVars->push_back( wxT( "SHORT_NET_NAME(<pin_number>)" ) );
    aVars->push_back( wxT( "NET_NAME(<pin_number>)" ) );
    aVars->push_back( wxT( "NET_CLASS(<pin_number>)" ) );
    aVars->push_back( wxT( "PIN_NAME(<pin_number>)" ) );
}


bool SCH_SYMBOL::ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth ) const
{
    static wxRegEx operatingPoint( wxT( "^"
                                        "OP"
                                        "(:[^.]*)?"                // pin
                                        "(.([0-9])?([a-zA-Z]*))?"  // format
                                        "$" ) );

    wxCHECK( aPath, false );

    SCHEMATIC* schematic = Schematic();

    if( !schematic )
        return false;

    if( operatingPoint.Matches( *token ) )
    {
        wxString pin( operatingPoint.GetMatch( *token, 1 ).Lower() );
        wxString precisionStr( operatingPoint.GetMatch( *token, 3 ) );
        wxString range( operatingPoint.GetMatch( *token, 4 ) );
        int      precision = 3;

        if( !precisionStr.IsEmpty() )
            precision = precisionStr[0] - '0';

        if( range.IsEmpty() )
            range = wxS( "~A" );

        SIM_LIB_MGR   simLibMgr( &schematic->Prj() );
        NULL_REPORTER devnull;
        SIM_MODEL&    model = simLibMgr.CreateModel( aPath, const_cast<SCH_SYMBOL&>( *this ),
                                                     devnull ).model;
        SPICE_ITEM spiceItem;
        spiceItem.refName = GetRef( aPath );

        wxString spiceRef = model.SpiceGenerator().ItemName( spiceItem );
        spiceRef = spiceRef.Lower();

        if( pin.IsEmpty() )
        {
            *token = schematic->GetOperatingPoint( spiceRef, precision, range );
            return true;
        }
        else if( pin == wxS( ":power" ) )
        {
            if( range.IsEmpty() )
                range = wxS( "~W" );

            *token = schematic->GetOperatingPoint( spiceRef + wxS( ":power" ), precision, range );
            return true;
        }
        else
        {
            pin = pin.SubString( 1, -1 );   // Strip ':' from front

            for( const std::reference_wrapper<const SIM_MODEL::PIN>& modelPin : model.GetPins() )
            {
                SCH_PIN* symbolPin = GetPin( modelPin.get().symbolPinNumber );

                if( pin == symbolPin->GetName().Lower() || pin == symbolPin->GetNumber().Lower() )
                {
                    if( model.GetPins().size() == 2 )
                    {
                        *token = schematic->GetOperatingPoint( spiceRef, precision, range );
                    }
                    else
                    {
                        wxString signalName = spiceRef + wxS( ":" ) + modelPin.get().name;
                        *token = schematic->GetOperatingPoint( signalName, precision, range );
                    }

                    return true;
                }
            }
        }

        *token = wxS( "?" );
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

    // Consider missing simulation fields as empty, not un-resolved
    if( token->IsSameAs( wxT( "SIM.DEVICE" ) )
            || token->IsSameAs( wxT( "SIM.TYPE" ) )
            || token->IsSameAs( wxT( "SIM.PINS" ) )
            || token->IsSameAs( wxT( "SIM.PARAMS" ) )
            || token->IsSameAs( wxT( "SIM.LIBRARY" ) )
            || token->IsSameAs( wxT( "SIM.NAME" ) ) )
    {
        *token = wxEmptyString;
        return true;
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

        if( parts.Count() > 0 )
            *token = parts[ 0 ];
        else
            *token = wxEmptyString;

        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        wxString footprint = GetFootprintFieldText( true, aPath, false );

        wxArrayString parts = wxSplit( footprint, ':' );

        if( parts.Count() > 1 )
            *token = parts[ std::min( 1, (int) parts.size() - 1 ) ];
        else
            *token = wxEmptyString;

        return true;
    }
    else if( token->IsSameAs( wxT( "UNIT" ) ) )
    {
        *token = SubReference( GetUnitSelection( aPath ) );
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_LIBRARY" ) ) )
    {
        *token = m_lib_id.GetUniStringLibNickname();
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_NAME" ) ) )
    {
        *token = m_lib_id.GetUniStringLibItemName();
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
        *token = this->GetExcludedFromBOM() ? _( "Excluded from BOM" ) : wxString( "" );
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_BOARD" ) ) )
    {
        *token = this->GetExcludedFromBoard() ? _( "Excluded from board" ) : wxString( "" );
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_SIM" ) ) )
    {
        *token = this->GetExcludedFromSim() ? _( "Excluded from simulation" ) : wxString( "" );
        return true;
    }
    else if( token->IsSameAs( wxT( "DNP" ) ) )
    {
        *token = this->GetDNP() ? _( "DNP" ) : wxString( "" );
        return true;
    }
    else if( token->StartsWith( wxT( "SHORT_NET_NAME(" ) )
                 || token->StartsWith( wxT( "NET_NAME(" ) )
                 || token->StartsWith( wxT( "NET_CLASS(" ) )
                 || token->StartsWith( wxT( "PIN_NAME(" ) ) )
    {
        wxString pinNumber = token->AfterFirst( '(' );
        pinNumber = pinNumber.BeforeLast( ')' );

        for( SCH_PIN* pin : GetPins( aPath ) )
        {
            if( pin->GetNumber() == pinNumber )
            {
                if( token->StartsWith( wxT( "PIN_NAME" ) ) )
                {
                    *token = pin->GetAlt().IsEmpty() ? pin->GetName() : pin->GetAlt();
                    return true;
                }

                SCH_CONNECTION* conn = pin->Connection( aPath );

                if( !conn )
                    *token = wxEmptyString;
                else if( token->StartsWith( wxT( "SHORT_NET_NAME" ) ) )
                    *token = conn->LocalName();
                else if( token->StartsWith( wxT( "NET_NAME" ) ) )
                    *token = conn->Name();
                else if( token->StartsWith( wxT( "NET_CLASS" ) ) )
                    *token = pin->GetEffectiveNetClass( aPath )->GetName();

                return true;
            }
        }
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

    case ( SYM_ORIENT_0 + SYM_MIRROR_X + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_0 );
        SetOrientation( SYM_MIRROR_X );
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

    case ( SYM_ORIENT_90 + SYM_MIRROR_X + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_90 );
        SetOrientation( SYM_MIRROR_X );
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

    case ( SYM_ORIENT_180 + SYM_MIRROR_X + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_180 );
        SetOrientation( SYM_MIRROR_X );
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

    case ( SYM_ORIENT_270 + SYM_MIRROR_X + SYM_MIRROR_Y ):
        SetOrientation( SYM_ORIENT_270 );
        SetOrientation( SYM_MIRROR_X );
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
                                 << GetLibId().Format().wx_str() << '"' << m_pos
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
        bBox = m_part->GetBodyBoundingBox( m_unit, m_bodyStyle, aIncludePins, false );
    else
        bBox = dummy()->GetBodyBoundingBox( m_unit, m_bodyStyle, aIncludePins, false );

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
    try
    {
        return doGetBoundingBox( false, false );
    }
    catch( const boost::bad_pointer& exc )
    {
        // This may be overkill and could be an assertion but we are more likely to
        // find any boost pointer container errors this way.
        wxLogError( wxT( "Boost bad pointer exception '%s' occurred." ), exc.what() );
        return BOX2I();
    }
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

                if( GetExcludedFromSim() )
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
            aList.emplace_back( _( "Description" ) + wxT( ": " )
                                        + GetField( DESCRIPTION_FIELD )->GetText(),
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


void SCH_SYMBOL::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    VECTOR2I prev = m_pos;

    RotatePoint( m_pos, aCenter, aRotateCCW ? ANGLE_270 : ANGLE_90 );

    SetOrientation( aRotateCCW ? SYM_ROTATE_CLOCKWISE : SYM_ROTATE_COUNTERCLOCKWISE );

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
    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        LIB_PIN* lib_pin = pin->GetLibPin();

        if( lib_pin && lib_pin->GetUnit() && m_unit && ( m_unit != lib_pin->GetUnit() ) )
            continue;

        DANGLING_END_ITEM item( PIN_END, lib_pin, GetPinPhysicalPosition( lib_pin ), this );
        aItemList.push_back( item );
    }
}


bool SCH_SYMBOL::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                      std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                      const SCH_SHEET_PATH*           aPath )
{
    bool changed = false;

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        bool previousState = pin->IsDangling();
        pin->SetIsDangling( true );

        VECTOR2I pos = m_transform.TransformCoordinate( pin->GetLocalPosition() ) + m_pos;

        auto lower = DANGLING_END_ITEM_HELPER::get_lower_pos( aItemListByPos, pos );
        bool do_break = false;

        for( auto it = lower; it < aItemListByPos.end() && it->GetPosition() == pos; it++ )
        {
            DANGLING_END_ITEM& each_item = *it;
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
                pin->SetIsDangling( false );
                do_break = true;
                break;

            default:
                break;
            }

            if( do_break )
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


bool SCH_SYMBOL::HasConnectivityChanges( const SCH_ITEM* aItem,
                                         const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourself.
    if( aItem == this )
        return false;

    const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( symbol, false );

    if( GetPosition() != symbol->GetPosition() )
        return true;

    if( GetLibId() != symbol->GetLibId() )
        return true;

    if( GetUnitSelection( aInstance ) != symbol->GetUnitSelection( aInstance ) )
        return true;

    if( GetRef( aInstance ) != symbol->GetRef( aInstance ) )
        return true;

    // Power symbol value field changes are connectivity changes.
    if( IsPower()
      && ( GetValue( true, aInstance, false ) != symbol->GetValue( true, aInstance, false ) ) )
        return true;

    if( m_pins.size() != symbol->m_pins.size() )
        return true;

    for( size_t i = 0; i < m_pins.size(); i++ )
    {
        if( m_pins[i]->HasConnectivityChanges( symbol->m_pins[i].get() ) )
            return true;
    }

    return false;
}


std::vector<VECTOR2I> SCH_SYMBOL::GetConnectionPoints() const
{
    std::vector<VECTOR2I> retval;

    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        // Collect only pins attached to the current unit and convert.
        // others are not associated to this symbol instance
        int pin_unit      = pin->GetLibPin() ? pin->GetLibPin()->GetUnit()
                                             : GetUnit();
        int pin_bodyStyle = pin->GetLibPin() ? pin->GetLibPin()->GetBodyStyle()
                                             : GetBodyStyle();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_bodyStyle > 0 && pin_bodyStyle != GetBodyStyle() )
            continue;

        retval.push_back( m_transform.TransformCoordinate( pin->GetLocalPosition() ) + m_pos );
    }

    return retval;
}


SCH_ITEM* SCH_SYMBOL::GetDrawItem( const VECTOR2I& aPosition, KICAD_T aType )
{
    if( m_part )
    {
        // Calculate the position relative to the symbol.
        VECTOR2I libPosition = aPosition - m_pos;

        return m_part->LocateDrawItem( m_unit, m_bodyStyle, aType, libPosition, m_transform );
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
                int pin_unit      = pin->GetLibPin() ? pin->GetLibPin()->GetUnit()
                                                     : GetUnit();
                int pin_bodyStyle = pin->GetLibPin() ? pin->GetLibPin()->GetBodyStyle()
                                                     : GetBodyStyle();

                if( pin_unit > 0 && pin_unit != GetUnit() )
                    continue;

                if( pin_bodyStyle > 0 && pin_bodyStyle != GetBodyStyle() )
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

    const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( &aItem );

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


SCH_SYMBOL& SCH_SYMBOL::operator=( const SCH_SYMBOL& aSymbol )
{
    wxCHECK_MSG( Type() == aSymbol.Type(), *this,
                 wxT( "Cannot assign object type " ) + aSymbol.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aSymbol != this )
    {
        SYMBOL::operator=( aSymbol );

        m_lib_id    = aSymbol.m_lib_id;
        m_part.reset( aSymbol.m_part ? new LIB_SYMBOL( *aSymbol.m_part ) : nullptr );
        m_pos       = aSymbol.m_pos;
        m_unit      = aSymbol.m_unit;
        m_bodyStyle = aSymbol.m_bodyStyle;
        m_transform = aSymbol.m_transform;

        m_instanceReferences = aSymbol.m_instanceReferences;

        m_fields    = aSymbol.m_fields;    // std::vector's assignment operator

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

    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        if( pin->GetType() == ELECTRICAL_PINTYPE::PT_NC )
            continue;

        // Collect only pins attached to the current unit and convert.
        // others are not associated to this symbol instance
        int pin_unit      = pin->GetLibPin() ? pin->GetLibPin()->GetUnit()
                                             : GetUnit();
        int pin_bodyStyle = pin->GetLibPin() ? pin->GetLibPin()->GetBodyStyle()
                                             : GetBodyStyle();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_bodyStyle > 0 && pin_bodyStyle != GetBodyStyle() )
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


void SCH_SYMBOL::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR_SETTINGS*      colors = Pgm().GetSettingsManager().GetColorSettings();

    if( m_part )
    {
        std::vector<LIB_PIN*>  libPins;
        m_part->GetPins( libPins, GetUnit(), GetBodyStyle() );

        // Copy the source so we can re-orient and translate it.
        LIB_SYMBOL tempSymbol( *m_part );
        std::vector<LIB_PIN*> tempPins;
        tempSymbol.GetPins( tempPins, GetUnit(), GetBodyStyle() );

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

        for( SCH_ITEM& item : tempSymbol.GetDrawItems() )
        {
            if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( &item ) )
            {
                // Use SCH_FIELD's text resolver
                SCH_FIELD dummy( this, -1 );
                dummy.SetText( text->GetText() );
                text->SetText( dummy.GetShownText( false ) );
            }
        }

        TRANSFORM savedTransform = renderSettings->m_Transform;
        renderSettings->m_Transform = GetTransform();
        aPlotter->StartBlock( nullptr );

        for( bool local_background : { true, false } )
        {
            tempSymbol.Plot( aPlotter, local_background, aPlotOpts, GetUnit(), GetBodyStyle(),
                             m_pos, GetDNP() );

            for( SCH_FIELD field : m_fields )
            {
                field.ClearRenderCache();
                field.Plot( aPlotter, local_background, aPlotOpts, GetUnit(), GetBodyStyle(),
                            m_pos, GetDNP() );
            }
        }

        if( m_DNP )
        {
            BOX2I bbox = GetBodyAndPinsBoundingBox();
            int   strokeWidth = 3.0 * schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );

            aPlotter->SetColor( colors->GetColor( LAYER_DNP_MARKER ) );

            aPlotter->ThickSegment( bbox.GetOrigin(), bbox.GetEnd(), strokeWidth, FILLED, nullptr );

            aPlotter->ThickSegment( bbox.GetOrigin() + VECTOR2I( bbox.GetWidth(), 0 ),
                                    bbox.GetOrigin() + VECTOR2I( 0, bbox.GetHeight() ),
                                    strokeWidth, FILLED, nullptr );
        }

        SCH_SHEET_PATH* sheet = &Schematic()->CurrentSheet();

        // Plot attributes to a hypertext menu
        if( aPlotOpts.m_PDFPropertyPopups )
        {
            std::vector<wxString> properties;

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
        }

        aPlotter->EndBlock( nullptr );
        renderSettings->m_Transform = savedTransform;

        if( !m_part->IsPower() )
            aPlotter->Bookmark( GetBoundingBox(), GetRef( sheet ), _( "Symbols" ) );
    }
}


void SCH_SYMBOL::PlotPins( PLOTTER* aPlotter ) const
{
    if( m_part )
    {
        SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
        TRANSFORM            savedTransform = renderSettings->m_Transform;
        renderSettings->m_Transform = GetTransform();

        std::vector<LIB_PIN*>  libPins;
        m_part->GetPins( libPins, GetUnit(), GetBodyStyle() );

        // Copy the source to stay const
        LIB_SYMBOL tempSymbol( *m_part );
        std::vector<LIB_PIN*> tempPins;
        tempSymbol.GetPins( tempPins, GetUnit(), GetBodyStyle() );

        SCH_PLOT_OPTS plotOpts;

        // Copy the pin info from the symbol to the temp pins
        for( unsigned i = 0; i < tempPins.size(); ++ i )
        {
            SCH_PIN* symbolPin = GetPin( libPins[ i ] );
            LIB_PIN* tempPin = tempPins[ i ];

            tempPin->SetName( symbolPin->GetShownName() );
            tempPin->SetType( symbolPin->GetType() );
            tempPin->SetShape( symbolPin->GetShape() );
            tempPin->Plot( aPlotter, false, plotOpts, GetUnit(), GetBodyStyle(), m_pos, GetDNP() );
        }

        renderSettings->m_Transform = savedTransform;
    }
}


bool SCH_SYMBOL::HasBrightenedPins()
{
    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        if( pin->IsBrightened() )
            return true;
    }

    return false;
}


void SCH_SYMBOL::ClearBrightenedPins()
{
    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
        pin->ClearBrightened();
}


bool SCH_SYMBOL::IsPointClickableAnchor( const VECTOR2I& aPos ) const
{
    for( const std::unique_ptr<SCH_PIN>& pin : m_pins )
    {
        int pin_unit      = pin->GetLibPin() ? pin->GetLibPin()->GetUnit()
                                             : GetUnit();
        int pin_bodyStyle = pin->GetLibPin() ? pin->GetLibPin()->GetBodyStyle()
                                             : GetBodyStyle();

        if( pin_unit > 0 && pin_unit != GetUnit() )
            continue;

        if( pin_bodyStyle > 0 && pin_bodyStyle != GetBodyStyle() )
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


bool SCH_SYMBOL::IsNormal() const
{
    wxCHECK( m_part, false );

    return m_part->IsNormal();
}


bool SCH_SYMBOL::operator==( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    const SCH_SYMBOL& symbol = static_cast<const SCH_SYMBOL&>( aOther );

    if( GetLibId() != symbol.GetLibId() )
        return false;

    if( GetPosition() != symbol.GetPosition() )
        return false;

    if( GetUnit() != symbol.GetUnit() )
        return false;

    if( GetBodyStyle() != symbol.GetBodyStyle() )
        return false;

    if( GetTransform() != symbol.GetTransform() )
        return false;

    if( GetFields() != symbol.GetFields() )
        return false;

    if( m_pins.size() != symbol.m_pins.size() )
        return false;

    for( unsigned i = 0; i < m_pins.size(); ++i )
    {
        if( !( *m_pins[i] == *symbol.m_pins[i] ) )
            return false;
    }

    return true;
}


double SCH_SYMBOL::Similarity( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return 0.0;

    const SCH_SYMBOL& symbol = static_cast<const SCH_SYMBOL&>( aOther );

    if( GetLibId() != symbol.GetLibId() )
        return 0.0;

    if( GetPosition() == symbol.GetPosition() )
        return 1.0;

    return 0.0;
}


static struct SCH_SYMBOL_DESC
{
    SCH_SYMBOL_DESC()
    {
        ENUM_MAP<SYMBOL_ORIENTATION_PROP>::Instance()
                .Map( SYMBOL_ANGLE_0,   wxS( "0" ) )
                .Map( SYMBOL_ANGLE_90,  wxS( "90" ) )
                .Map( SYMBOL_ANGLE_180, wxS( "180" ) )
                .Map( SYMBOL_ANGLE_270, wxS( "270" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_SYMBOL );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SYMBOL ), TYPE_HASH( SYMBOL ) );

        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, int>( _HKI( "Position X" ),
                    &SCH_SYMBOL::SetX, &SCH_SYMBOL::GetX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ) );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, int>( _HKI( "Position Y" ),
                    &SCH_SYMBOL::SetY, &SCH_SYMBOL::GetY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ) );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_SYMBOL, SYMBOL_ORIENTATION_PROP>(
                    _HKI( "Orientation" ),
                    &SCH_SYMBOL::SetOrientationProp, &SCH_SYMBOL::GetOrientationProp ) );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, bool>( _HKI( "Mirror X" ),
                    &SCH_SYMBOL::SetMirrorX, &SCH_SYMBOL::GetMirrorX ) );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, bool>( _HKI( "Mirror Y" ),
                    &SCH_SYMBOL::SetMirrorY, &SCH_SYMBOL::GetMirrorY ) );

        auto hasLibPart =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aItem ) )
                        return symbol->GetLibSymbolRef() != nullptr;

                    return false;
                };

        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Pin numbers" ),
                    &SYMBOL::SetShowPinNumbers, &SYMBOL::GetShowPinNumbers ) )
                .SetAvailableFunc( hasLibPart );

        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Pin names" ),
                    &SYMBOL::SetShowPinNames, &SYMBOL::GetShowPinNames ) )
                .SetAvailableFunc( hasLibPart );

        const wxString groupFields = _HKI( "Fields" );

        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, wxString>( _HKI( "Reference" ),
                    &SCH_SYMBOL::SetRefProp, &SCH_SYMBOL::GetRefProp ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, wxString>( _HKI( "Value" ),
                    &SCH_SYMBOL::SetValueProp, &SCH_SYMBOL::GetValueProp ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, wxString>( _HKI( "Library Link" ),
                    NO_SETTER( SCH_SYMBOL, wxString ), &SCH_SYMBOL::GetSymbolIDAsString ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, wxString>( _HKI( "Library Description" ),
                    NO_SETTER( SCH_SYMBOL, wxString ), &SCH_SYMBOL::GetDescription ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<SCH_SYMBOL, wxString>( _HKI( "Keywords" ),
                    NO_SETTER( SCH_SYMBOL, wxString ), &SCH_SYMBOL::GetKeyWords ),
                    groupFields );

        const wxString groupAttributes = _HKI( "Attributes" );

        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Exclude From Board" ),
                    &SYMBOL::SetExcludedFromBoard, &SYMBOL::GetExcludedFromBoard ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Exclude From Simulation" ),
                    &SYMBOL::SetExcludedFromSim, &SYMBOL::GetExcludedFromSim ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Exclude From Bill of Materials" ),
                    &SYMBOL::SetExcludedFromBOM, &SYMBOL::GetExcludedFromBOM ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Do not Populate" ),
                    &SYMBOL::SetDNP, &SYMBOL::GetDNP ),
                    groupAttributes );
    }
} _SCH_SYMBOL_DESC;

ENUM_TO_WXANY( SYMBOL_ORIENTATION_PROP )
