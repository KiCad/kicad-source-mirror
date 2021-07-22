/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_rectangle.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <trace_helpers.h>
#include <trigo.h>
#include <refdes_utils.h>
#include <wx/log.h>
#include <kicad_string.h>

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
 * DEF DUMMY U 0 40 Y Y 1 0 N
 * F0 "U" 0 -350 60 H V
 * F1 "DUMMY" 0 350 60 H V
 * DRAW
 * T 0 0 0 150 0 0 0 ??
 * S -200 200 200 -200 0 1 0
 * ENDDRAW
 * ENDDEF
 */
static LIB_SYMBOL* dummy()
{
    static LIB_SYMBOL* symbol;

    if( !symbol )
    {
        symbol = new LIB_SYMBOL( wxEmptyString );

        LIB_RECTANGLE* square = new LIB_RECTANGLE( symbol );

        square->MoveTo( wxPoint( Mils2iu( -200 ), Mils2iu( 200 ) ) );
        square->SetEndPosition( wxPoint( Mils2iu( 200 ), Mils2iu( -200 ) ) );

        LIB_TEXT* text = new LIB_TEXT( symbol );

        text->SetTextSize( wxSize( Mils2iu( 150 ), Mils2iu( 150 ) ) );
        text->SetText( wxString( wxT( "??" ) ) );

        symbol->AddDrawItem( square );
        symbol->AddDrawItem( text );
    }

    return symbol;
}


SCH_SYMBOL::SCH_SYMBOL( const wxPoint& aPos, SCH_ITEM* aParent ) :
    SCH_ITEM( aParent, SCH_SYMBOL_T )
{
    Init( aPos );
}


SCH_SYMBOL::SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const LIB_ID& aLibId,
                        const SCH_SHEET_PATH* aSheet, int unit, int convert, const wxPoint& pos ) :
    SCH_ITEM( nullptr, SCH_SYMBOL_T )
{
    Init( pos );

    m_unit      = unit;
    m_convert   = convert;
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

    // Inherit the include in bill of materials and board netlist settings from library symbol.
    m_inBom = aSymbol.GetIncludeInBom();
    m_onBoard = aSymbol.GetIncludeOnBoard();
}


SCH_SYMBOL::SCH_SYMBOL( const LIB_SYMBOL& aSymbol, const SCH_SHEET_PATH* aSheet,
                        const PICKED_SYMBOL& aSel, const wxPoint& pos ) :
    SCH_SYMBOL( aSymbol, aSel.LibId, aSheet, aSel.Unit, aSel.Convert, pos )
{
    // Set any fields that were modified as part of the symbol selection
    for( const std::pair<int, wxString>& i : aSel.Fields )
    {
        SCH_FIELD* field = GetFieldById( i.first );

        if( field )
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
    m_inBom       = aSymbol.m_inBom;
    m_onBoard     = aSymbol.m_onBoard;

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


void SCH_SYMBOL::Init( const wxPoint& pos )
{
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
    m_inBom = true;
    m_onBoard = true;
}


EDA_ITEM* SCH_SYMBOL::Clone() const
{
    return new SCH_SYMBOL( *this );
}


void SCH_SYMBOL::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount      = 3;
    aLayers[0]  = LAYER_DEVICE;
    aLayers[1]  = LAYER_DEVICE_BACKGROUND;
    aLayers[2]  = LAYER_SELECTION_SHADOWS;
}


void SCH_SYMBOL::SetLibId( const LIB_ID& aLibId )
{
    if( m_lib_id != aLibId )
    {
        m_lib_id = aLibId;
        SetModified();
    }
}


wxString SCH_SYMBOL::GetSchSymbolLibraryName() const
{
    if( !m_schLibSymbolName.IsEmpty() )
        return m_schLibSymbolName;
    else
        return m_lib_id.Format().wx_str();
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

    for( LIB_PIN* libPin = m_part->GetNextPin(); libPin; libPin = m_part->GetNextPin( libPin ) )
    {
        wxASSERT( libPin->Type() == LIB_PIN_T );

        if( libPin->GetConvert() && m_convert && ( m_convert != libPin->GetConvert() ) )
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
    if( m_unit != aUnit )
    {
        m_unit = aUnit;
        SetModified();
    }
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
        SetModified();
    }
}


void SCH_SYMBOL::SetTransform( const TRANSFORM& aTransform )
{
    if( m_transform != aTransform )
    {
        m_transform = aTransform;
        SetModified();
    }
}


int SCH_SYMBOL::GetUnitCount() const
{
    if( m_part )
        return m_part->GetUnitCount();

    return 0;
}


void SCH_SYMBOL::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    LIB_SYMBOL_OPTIONS opts;
    opts.transform = m_transform;
    opts.draw_visible_fields = false;
    opts.draw_hidden_fields = false;

    if( m_part )
    {
        m_part->Print( aSettings, m_pos + aOffset, m_unit, m_convert, opts );
    }
    else    // Use dummy() part if the actual cannot be found.
    {
        dummy()->Print( aSettings, m_pos + aOffset, 0, 0, opts );
    }

    for( SCH_FIELD& field : m_fields )
        field.Print( aSettings, aOffset );
}


void SCH_SYMBOL::AddHierarchicalReference( const KIID_PATH& aPath, const wxString& aRef,
                                           int aUnit, const wxString& aValue,
                                           const wxString& aFootprint )
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

    SYMBOL_INSTANCE_REFERENCE instance;
    instance.m_Path = aPath;
    instance.m_Reference = aRef;
    instance.m_Unit = aUnit;
    instance.m_Value = aValue;
    instance.m_Footprint = aFootprint;

    wxLogTrace( traceSchSheetPaths, "Adding symbol instance:\n"
                                    "  sheet path %s\n"
                                    "  reference %s, unit %d to symbol %s.",
                                    aPath.AsString(),
                                    aRef,
                                    aUnit,
                                    m_Uuid.AsString() );

    m_instanceReferences.push_back( instance );
}


const wxString SCH_SYMBOL::GetRef( const SCH_SHEET_PATH* sheet, bool aIncludeUnit ) const
{
    KIID_PATH path = sheet->Path();
    wxString  ref;

    for( const SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            ref = instance.m_Reference;
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
        ref += LIB_SYMBOL::SubReference( GetUnit() );

    return ref;
}


bool SCH_SYMBOL::IsReferenceStringValid( const wxString& aReferenceString )
{
    return !UTIL::GetRefDesPrefix( aReferenceString ).IsEmpty();
}


void SCH_SYMBOL::SetRef( const SCH_SHEET_PATH* sheet, const wxString& ref )
{
    KIID_PATH path = sheet->Path();
    bool      notInArray = true;

    // check to see if it is already there before inserting it
    for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            instance.m_Reference = ref;
            notInArray = false;
        }
    }

    if( notInArray )
        AddHierarchicalReference( path, ref, m_unit );

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
        pin->ClearDefaultNetName( sheet );

    SCH_FIELD* rf = GetField( REFERENCE_FIELD );

    rf->SetText( ref );  // for drawing.

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

    for( const SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
            return instance.m_Reference.Last() != '?';
    }

    return false;
}


int SCH_SYMBOL::GetUnitSelection( const SCH_SHEET_PATH* aSheet ) const
{
    KIID_PATH path = aSheet->Path();

    for( const SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
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
    for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
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
    for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
        instance.m_Unit = aUnitSelection;
}


const wxString SCH_SYMBOL::GetValue( const SCH_SHEET_PATH* sheet, bool aResolve ) const
{
    KIID_PATH path = sheet->Path();

    for( const SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path && !instance.m_Value.IsEmpty() )
        {
            // This can only be an override from an Update Schematic from PCB, and therefore
            // will always be fully resolved.
            return instance.m_Value;
        }
    }

    if( !aResolve )
        return GetField( VALUE_FIELD )->GetText();

    return GetField( VALUE_FIELD )->GetShownText();
}


void SCH_SYMBOL::SetValue( const SCH_SHEET_PATH* sheet, const wxString& aValue )
{
    if( sheet == nullptr )
    {
        // Clear instance overrides and set primary field value
        for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
            instance.m_Value = wxEmptyString;

        m_fields[ VALUE_FIELD ].SetText( aValue );
        return;
    }

    KIID_PATH path = sheet->Path();

    // check to see if it is already there before inserting it
    for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            instance.m_Value = aValue;
            return;
        }
    }

    // didn't find it; better add it
    AddHierarchicalReference( path, UTIL::GetRefDesUnannotated( m_prefix ), m_unit,
                              aValue, wxEmptyString );
}


const wxString SCH_SYMBOL::GetFootprint( const SCH_SHEET_PATH* sheet, bool aResolve ) const
{
    KIID_PATH path = sheet->Path();

    for( const SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path && !instance.m_Footprint.IsEmpty() )
        {
            // This can only be an override from an Update Schematic from PCB, and therefore
            // will always be fully resolved.
            return instance.m_Footprint;
        }
    }

    if( !aResolve )
        return GetField( FOOTPRINT_FIELD )->GetText();

    return GetField( FOOTPRINT_FIELD )->GetShownText();
}


void SCH_SYMBOL::SetFootprint( const SCH_SHEET_PATH* sheet, const wxString& aFootprint )
{
    if( sheet == nullptr )
    {
        // Clear instance overrides and set primary field value
        for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
            instance.m_Footprint = wxEmptyString;

        m_fields[ FOOTPRINT_FIELD ].SetText( aFootprint );
        return;
    }

    KIID_PATH path = sheet->Path();

    // check to see if it is already there before inserting it
    for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
    {
        if( instance.m_Path == path )
        {
            instance.m_Footprint = aFootprint;
            return;
        }
    }

    // didn't find it; better add it
    AddHierarchicalReference( path, UTIL::GetRefDesUnannotated( m_prefix ), m_unit,
                              wxEmptyString, aFootprint );
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


wxString SCH_SYMBOL::GetFieldText( const wxString& aFieldName, SCH_EDIT_FRAME* aFrame ) const
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
        if( !aVisibleOnly || ( field.IsVisible() && !field.IsVoid() ) )
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


SCH_FIELD* SCH_SYMBOL::FindField( const wxString& aFieldName, bool aIncludeDefaultFields )
{
    unsigned start = aIncludeDefaultFields ? 0 : MANDATORY_FIELDS;

    for( unsigned i = start; i < m_fields.size(); ++i )
    {
        if( aFieldName == m_fields[i].GetName( false ) )
            return &m_fields[i];
    }

    return nullptr;
}


void SCH_SYMBOL::UpdateFields( const SCH_SHEET_PATH* aPath, bool aUpdateStyle, bool aUpdateRef,
                               bool aUpdateOtherFields, bool aResetRef, bool aResetOtherFields )
{
    if( m_part )
    {
        wxString                symbolName;
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
                    SCH_FIELD newField( wxPoint( 0, 0), GetFieldCount(), this, fieldName );
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
                if( aResetOtherFields )
                    SetRef( aPath, m_part->GetReferenceField().GetText() );
                else if( aUpdateRef )
                    SetRef( aPath, libField->GetText() );
            }
            else if( id == VALUE_FIELD )
            {
                if( aResetOtherFields )
                    SetValue( UnescapeString( m_lib_id.GetLibItemName() ) ); // alias-specific value
                else
                    SetValue( UnescapeString( libField->GetText() ) );
            }
            else if( id == FOOTPRINT_FIELD )
            {
                if( aResetOtherFields || aUpdateOtherFields )
                    SetFootprint( libField->GetText() );
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


SCH_PIN* SCH_SYMBOL::GetPin( LIB_PIN* aLibPin )
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

    for( const auto& p : m_pins )
    {
        if( unit && p->GetLibPin()->GetUnit() && ( p->GetLibPin()->GetUnit() != unit ) )
            continue;

        pins.push_back( p.get() );
    }

    return pins;
}


void SCH_SYMBOL::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != nullptr) && (aItem->Type() == SCH_SYMBOL_T),
                 wxT( "Cannot swap data with invalid symbol." ) );

    SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem;

    std::swap( m_lib_id, symbol->m_lib_id );

    LIB_SYMBOL* libSymbol = symbol->m_part.release();
    symbol->m_part.reset( m_part.release() );
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

    std::swap( m_instanceReferences, symbol->m_instanceReferences );
    std::swap( m_schLibSymbolName, symbol->m_schLibSymbolName );
}


void SCH_SYMBOL::GetContextualTextVars( wxArrayString* aVars ) const
{
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
        aVars->push_back( m_fields[i].GetCanonicalName().Upper() );

    for( size_t i = MANDATORY_FIELDS; i < m_fields.size(); ++i )
        aVars->push_back( m_fields[i].GetName() );

    aVars->push_back( wxT( "FOOTPRINT_LIBRARY" ) );
    aVars->push_back( wxT( "FOOTPRINT_NAME" ) );
    aVars->push_back( wxT( "UNIT" ) );
}


bool SCH_SYMBOL::ResolveTextVar( wxString* token, int aDepth ) const
{
    SCHEMATIC* schematic = Schematic();

    // SCH_SYMOL object has no context outside a schematic.
    wxCHECK( schematic, false );

    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        if( token->IsSameAs( m_fields[ i ].GetCanonicalName().Upper() ) )
        {
            if( i == REFERENCE_FIELD )
                *token = GetRef( &schematic->CurrentSheet(), true );
            else if( i == VALUE_FIELD )
                *token = GetValue( &schematic->CurrentSheet(), true );
            else if( i == FOOTPRINT_FIELD )
                *token = GetFootprint( &schematic->CurrentSheet(), true );
            else
                *token = m_fields[ i ].GetShownText( aDepth + 1 );

            return true;
        }
    }

    for( size_t i = MANDATORY_FIELDS; i < m_fields.size(); ++i )
    {
        if( token->IsSameAs( m_fields[ i ].GetName() )
            || token->IsSameAs( m_fields[ i ].GetName().Upper() ) )
        {
            *token = m_fields[ i ].GetShownText( aDepth + 1 );
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
        wxString footprint;

        footprint = GetFootprint( &schematic->CurrentSheet(), true );

        wxArrayString parts = wxSplit( footprint, ':' );

        *token = parts[ 0 ];
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        wxString footprint;

        footprint = GetFootprint( &schematic->CurrentSheet(), true );

        wxArrayString parts = wxSplit( footprint, ':' );

        *token = parts[ std::min( 1, (int) parts.size() - 1 ) ];
        return true;
    }
    else if( token->IsSameAs( wxT( "UNIT" ) ) )
    {
        int unit;

        unit = GetUnitSelection( &schematic->CurrentSheet() );

        *token = LIB_SYMBOL::SubReference( unit );
        return true;
    }

    return false;
}


void SCH_SYMBOL::ClearAnnotation( const SCH_SHEET_PATH* aSheetPath )
{
    // Build a reference with no annotation, i.e. a reference ending with a single '?'
    wxString defRef = UTIL::GetRefDesUnannotated( m_prefix );

    if( aSheetPath )
    {
        KIID_PATH path = aSheetPath->Path();

        for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
        {
            if( instance.m_Path == path )
                instance.m_Reference = defRef;
        }
    }
    else
    {
        for( SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
            instance.m_Reference = defRef;
    }

    for( std::unique_ptr<SCH_PIN>& pin : m_pins )
        pin->ClearDefaultNetName( aSheetPath );

    // These 2 changes do not work in complex hierarchy.
    // When a clear annotation is made, the calling function must call a
    // UpdateAllScreenReferences for the active sheet.
    // But this call cannot made here.
    m_fields[REFERENCE_FIELD].SetText( defRef ); //for drawing.
}


bool SCH_SYMBOL::AddSheetPathReferenceEntryIfMissing( const KIID_PATH& aSheetPath )
{
    // a empty sheet path is illegal:
    wxCHECK( aSheetPath.size() > 0, false );

    wxString reference_path;

    for( const SYMBOL_INSTANCE_REFERENCE& instance : m_instanceReferences )
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
                [ aOldSheetPath ]( SYMBOL_INSTANCE_REFERENCE& r )->bool
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


int SCH_SYMBOL::GetOrientation()
{
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

    for( int type_rotate : rotate_values )
    {
        SetOrientation( type_rotate );

        if( transform == m_transform )
            return type_rotate;
    }

    // Error: orientation not found in list (should not happen)
    wxFAIL_MSG( "Schematic symbol orientation matrix internal error." );
    m_transform = transform;

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


EDA_RECT SCH_SYMBOL::GetBodyBoundingBox() const
{
    EDA_RECT    bBox;

    if( m_part )
        bBox = m_part->GetBodyBoundingBox( m_unit, m_convert );
    else
        bBox = dummy()->GetBodyBoundingBox( m_unit, m_convert );

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
    return bBox;
}


const EDA_RECT SCH_SYMBOL::GetBoundingBox() const
{
    EDA_RECT bbox = GetBodyBoundingBox();

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.IsVisible() )
            bbox.Merge( field.GetBoundingBox() );
    }

    return bbox;
}


const EDA_RECT SCH_SYMBOL::GetBoundingBox( bool aIncludeInvisibleText ) const
{
    EDA_RECT bbox = GetBodyBoundingBox();

    for( const SCH_FIELD& field : m_fields )
    {
        if( field.IsVisible() || aIncludeInvisibleText )
            bbox.Merge( field.GetBoundingBox() );
    }

    return bbox;
}


void SCH_SYMBOL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );
    SCH_SHEET_PATH* currentSheet = schframe ? &schframe->GetCurrentSheet() : nullptr;

    // part and alias can differ if alias is not the root
    if( m_part )
    {
        if( m_part.get() != dummy() )
        {
            aList.push_back( MSG_PANEL_ITEM( _( "Reference" ), GetRef( currentSheet ) ) );

            msg = m_part->IsPower() ? _( "Power symbol" ) : _( "Value" );

            aList.push_back( MSG_PANEL_ITEM( msg, GetValue( currentSheet, true ) ) );

#if 0       // Display symbol flags, for debug only
            aList.push_back( MSG_PANEL_ITEM( _( "flags" ),
                                             wxString::Format( "%X", GetEditFlags() ) ) );
#endif

            // Display symbol reference in library and library
            aList.push_back( MSG_PANEL_ITEM( _( "Name" ),
                                             UnescapeString( GetLibId().GetLibItemName() ) ) );

            if( !m_part->IsRoot() )
            {
                msg = _( "Missing parent" );

                std::shared_ptr< LIB_SYMBOL > parent = m_part->GetParent().lock();

                if( parent )
                    msg = parent->GetName();

                aList.push_back( MSG_PANEL_ITEM( _( "Alias of" ), UnescapeString( msg ) ) );
            }
            else if( !m_lib_id.GetLibNickname().empty() )
            {
                aList.push_back( MSG_PANEL_ITEM( _( "Library" ), m_lib_id.GetLibNickname() ) );
            }
            else
            {
                aList.push_back( MSG_PANEL_ITEM( _( "Library" ), _( "Undefined!!!" ) ) );
            }

            // Display the current associated footprint, if exists.
            msg = GetFootprint( currentSheet, true );

            if( msg.IsEmpty() )
                msg = _( "<Unknown>" );

            aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), msg ) );

            // Display description of the symbol, and keywords found in lib
            aList.push_back( MSG_PANEL_ITEM( _( "Description" ), m_part->GetDescription(),
                                             DARKCYAN ) );
            aList.push_back( MSG_PANEL_ITEM( _( "Keywords" ), m_part->GetKeyWords() ) );
        }
    }
    else
    {
        aList.push_back( MSG_PANEL_ITEM( _( "Reference" ), GetRef( currentSheet ) ) );

        aList.push_back( MSG_PANEL_ITEM( _( "Value" ), GetValue( currentSheet, true ) ) );
        aList.push_back( MSG_PANEL_ITEM( _( "Name" ), GetLibId().GetLibItemName() ) );

        wxString libNickname = GetLibId().GetLibNickname();

        if( libNickname.empty() )
        {
            aList.push_back( MSG_PANEL_ITEM( _( "Library" ), _( "No library defined!" ) ) );
        }
        else
        {
            msg.Printf( _( "Symbol not found in %s!" ), libNickname );
            aList.push_back( MSG_PANEL_ITEM( _( "Library" ), msg ) );
        }
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
        wxPoint pos = field.GetTextPos();
        pos.x -= dx;
        field.SetTextPos( pos );
    }
}


void SCH_SYMBOL::MirrorVertically( int aCenter )
{
    int dy = m_pos.y;

    SetOrientation( SYM_MIRROR_X );
    MIRROR( m_pos.y, aCenter );
    dy -= m_pos.y;     // dy,0 is the move vector for this transform

    for( SCH_FIELD& field : m_fields )
    {
        // Move the fields to the new position because the symbol itself has moved.
        wxPoint pos = field.GetTextPos();
        pos.y -= dy;
        field.SetTextPos( pos );
    }
}


void SCH_SYMBOL::Rotate( const wxPoint& aCenter )
{
    wxPoint prev = m_pos;

    RotatePoint( &m_pos, aCenter, 900 );

    SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );

    for( SCH_FIELD& field : m_fields )
    {
        // Move the fields to the new position because the symbol itself has moved.
        wxPoint pos = field.GetTextPos();
        pos.x -= prev.x - m_pos.x;
        pos.y -= prev.y - m_pos.y;
        field.SetTextPos( pos );
    }
}


bool SCH_SYMBOL::Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText( EDA_UNITS::MILLIMETRES ) );

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

        wxPoint pos = m_transform.TransformCoordinate( pin->GetLocalPosition() ) + m_pos;

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
            case WIRE_START_END:
            case WIRE_END_END:
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


wxPoint SCH_SYMBOL::GetPinPhysicalPosition( const LIB_PIN* Pin ) const
{
    wxCHECK_MSG( Pin != nullptr && Pin->Type() == LIB_PIN_T, wxPoint( 0, 0 ),
                 wxT( "Cannot get physical position of pin." ) );

    return m_transform.TransformCoordinate( Pin->GetPosition() ) + m_pos;
}


std::vector<wxPoint> SCH_SYMBOL::GetConnectionPoints() const
{
    std::vector<wxPoint> retval;

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


LIB_ITEM* SCH_SYMBOL::GetDrawItem( const wxPoint& aPosition, KICAD_T aType )
{
    if( m_part )
    {
        // Calculate the position relative to the symbol.
        wxPoint libPosition = aPosition - m_pos;

        return m_part->LocateDrawItem( m_unit, m_convert, aType, libPosition, m_transform );
    }

    return nullptr;
}


wxString SCH_SYMBOL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Symbol %s [%s]" ),
                             GetField( REFERENCE_FIELD )->GetShownText(),
                             GetLibId().GetLibItemName().wx_str() );
}


SEARCH_RESULT SCH_SYMBOL::Visit( INSPECTOR aInspector, void* aTestData,
                                 const KICAD_T aFilterTypes[] )
{
    KICAD_T     stype;

    for( const KICAD_T* p = aFilterTypes; (stype = *p) != EOT; ++p )
    {
        if( stype == SCH_LOCATE_ANY_T
          || ( stype == SCH_SYMBOL_T )
          || ( stype == SCH_SYMBOL_LOCATE_POWER_T && m_part && m_part->IsPower() ) )
        {
            if( SEARCH_RESULT::QUIT == aInspector( this, aTestData ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_FIELD_T )
        {
            for( SCH_FIELD& field : m_fields )
            {
                if( SEARCH_RESULT::QUIT == aInspector( &field, (void*) this ) )
                    return SEARCH_RESULT::QUIT;
            }
        }

        if( stype == SCH_FIELD_LOCATE_REFERENCE_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( REFERENCE_FIELD ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_FIELD_LOCATE_VALUE_T
                || ( stype == SCH_SYMBOL_LOCATE_POWER_T && m_part && m_part->IsPower() ) )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( VALUE_FIELD ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_FIELD_LOCATE_FOOTPRINT_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( FOOTPRINT_FIELD ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_FIELD_LOCATE_DATASHEET_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetField( DATASHEET_FIELD ), (void*) this ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_PIN_T )
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

                if( SEARCH_RESULT::QUIT == aInspector( pin.get(), (void*) this ) )
                    return SEARCH_RESULT::QUIT;
            }
        }
    }

    return SEARCH_RESULT::CONTINUE;
}


bool SCH_SYMBOL::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto symbol = static_cast<const SCH_SYMBOL*>( &aItem );

    EDA_RECT rect = GetBodyBoundingBox();

    if( rect.GetArea() != symbol->GetBodyBoundingBox().GetArea() )
        return rect.GetArea() < symbol->GetBodyBoundingBox().GetArea();

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


bool SCH_SYMBOL::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBodyBoundingBox();
    bBox.Inflate( aAccuracy );

    if( bBox.Contains( aPosition ) )
        return true;

    return false;
}


bool SCH_SYMBOL::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & STRUCT_DELETED || m_flags & SKIP_STRUCT )
        return false;

    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBodyBoundingBox() );

    return rect.Intersects( GetBodyBoundingBox() );
}


bool SCH_SYMBOL::doIsConnected( const wxPoint& aPosition ) const
{
    wxPoint new_pos = m_transform.InverseTransform().TransformCoordinate( aPosition - m_pos );

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


void SCH_SYMBOL::Plot( PLOTTER* aPlotter ) const
{
    if( m_part )
    {
        TRANSFORM temp = GetTransform();
        aPlotter->StartBlock( nullptr );

        m_part->Plot( aPlotter, GetUnit(), GetConvert(), m_pos, temp );

        for( SCH_FIELD field : m_fields )
            field.Plot( aPlotter );

        aPlotter->EndBlock( nullptr );
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


bool SCH_SYMBOL::IsPointClickableAnchor( const wxPoint& aPos ) const
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
