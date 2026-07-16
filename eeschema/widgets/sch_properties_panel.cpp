/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sch_properties_panel.h"

#include <dialog_symbol_properties.h>
#include <font/fontconfig.h>
#include <pgm_base.h>
#include <common.h>
#include <confirm.h>
#include <connection_graph.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <schematic.h>
#include <sch_symbol.h>
#include <lib_symbol.h>
#include <sch_field.h>
#include <pin_map.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>
#include <wx/button.h>
#include <set>

static const wxString MISSING_FIELD_SENTINEL = wxS( "\uE000" );

class SCH_SYMBOL_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    { }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override
    {
        return PROPERTY_BASE::Writeable( aObject );
    }

    void setter( void* obj, wxAny& v ) override
    {
        wxString value;

        if( !v.GetAs( &value ) )
            return;

        SCH_SYMBOL* symbol = reinterpret_cast<SCH_SYMBOL*>( obj );
        SCH_FIELD*  field = symbol->GetField( m_name );

        wxString variantName;
        const SCH_SHEET_PATH* sheetPath = nullptr;

        if( symbol->Schematic() )
        {
            variantName = symbol->Schematic()->GetCurrentVariant();
            sheetPath = &symbol->Schematic()->CurrentSheet();
        }

        if( !field )
        {
            SCH_FIELD newField( symbol, FIELD_T::USER, m_name );
            newField.SetText( value, sheetPath, variantName );
            symbol->AddField( newField );
        }
        else
        {
            field->SetText( value, sheetPath, variantName );
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );
        const SCH_FIELD*  field = symbol->GetField( m_name );

        if( field )
        {
            wxString variantName;
            const SCH_SHEET_PATH* sheetPath = nullptr;

            if( symbol->Schematic() )
            {
                variantName = symbol->Schematic()->GetCurrentVariant();
                sheetPath = &symbol->Schematic()->CurrentSheet();
            }

            wxString text;

            if( !variantName.IsEmpty() && sheetPath )
                text = field->GetText( sheetPath, variantName );
            else
                text = field->GetText();

            return wxAny( text );
        }
        else
        {
            return wxAny( MISSING_FIELD_SENTINEL );
        }
    }

private:
    wxString m_name;
};

class SCH_SHEET_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SHEET_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SHEET ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SHEET ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override { return PROPERTY_BASE::Writeable( aObject ); }

    void setter( void* obj, wxAny& v ) override
    {
        wxString value;

        if( !v.GetAs( &value ) )
            return;

        SCH_SHEET* sheet = reinterpret_cast<SCH_SHEET*>( obj );
        SCH_FIELD* field = sheet->GetField( m_name );

        wxString              variantName;
        const SCH_SHEET_PATH* sheetPath = nullptr;

        if( sheet->Schematic() )
        {
            variantName = sheet->Schematic()->GetCurrentVariant();
            sheetPath = &sheet->Schematic()->CurrentSheet();
        }

        if( !field )
        {
            SCH_FIELD newField( sheet, FIELD_T::USER, m_name );
            newField.SetText( value, sheetPath, variantName );
            sheet->AddField( newField );
        }
        else
        {
            field->SetText( value, sheetPath, variantName );
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SHEET* sheet = reinterpret_cast<const SCH_SHEET*>( obj );
        const SCH_FIELD* field = sheet->GetField( m_name );

        if( field )
        {
            wxString              variantName;
            const SCH_SHEET_PATH* sheetPath = nullptr;

            if( sheet->Schematic() )
            {
                variantName = sheet->Schematic()->GetCurrentVariant();
                sheetPath = &sheet->Schematic()->CurrentSheet();
            }

            wxString text;

            if( !variantName.IsEmpty() && sheetPath )
                text = field->GetText( sheetPath, variantName );
            else
                text = field->GetText();

            return wxAny( text );
        }
        else
        {
            return wxAny( MISSING_FIELD_SENTINEL );
        }
    }

private:
    wxString m_name;
};

// Stable property names for the Pin Map group (issue #2282).  Kept canonical (untranslated) so the
// PROPERTY_MANAGER lookup is locale-independent; the displayed captions are translated by the grid.
static const wxString PIN_MAP_GROUP = _HKI( "Pin Map" );
static const wxString PIN_MAP_FOOTPRINT_PROP = wxS( "Pin Map Footprint" );
static const wxString PIN_MAP_MODE_PROP = wxS( "Pin Map Mode" );
static const wxString PIN_MAP_NAME_PROP = wxS( "Pin Map Name" );


/**
 * @return the associated footprint that the symbol's current footprint resolves to, or the first
 *         associated footprint when the symbol's footprint field matches none of them.  Returns
 *         nullptr when the symbol has no effective associated footprints.
 */
static const ASSOCIATED_FOOTPRINT* activeAssociatedFootprint( const SCH_SYMBOL* aSymbol )
{
    const std::unique_ptr<LIB_SYMBOL>& libSymbol = aSymbol->GetLibSymbolRef();

    if( !libSymbol )
        return nullptr;

    const std::vector<ASSOCIATED_FOOTPRINT>& assoc = libSymbol->GetEffectiveAssociatedFootprints();

    if( assoc.empty() )
        return nullptr;

    LIB_ID fpId;

    if( const SCH_FIELD* fpField = aSymbol->GetField( FIELD_T::FOOTPRINT ) )
        fpId.Parse( fpField->GetText() );

    for( const ASSOCIATED_FOOTPRINT& candidate : assoc )
    {
        if( candidate.m_FootprintLibId == fpId )
            return &candidate;
    }

    return &assoc.front();
}


static bool symbolHasAssociatedFootprint( INSPECTABLE* aObject )
{
    const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( aObject );

    return symbol && activeAssociatedFootprint( symbol ) != nullptr;
}


/**
 * Read-only property showing the footprint a symbol's pin map resolves against (issue #2282).
 */
class SCH_SYMBOL_PIN_MAP_FOOTPRINT_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_PIN_MAP_FOOTPRINT_PROPERTY() :
            PROPERTY_BASE( PIN_MAP_FOOTPRINT_PROP )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override { return false; }

    void setter( void* obj, wxAny& v ) override {}

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );

        if( const ASSOCIATED_FOOTPRINT* active = activeAssociatedFootprint( symbol ) )
            return wxAny( active->m_FootprintLibId.Format().wx_str() );

        return wxAny( wxEmptyString );
    }
};


/**
 * Override-mode selector for a symbol's per-instance pin map (issue #2282).
 *
 * The three user-selectable modes are Library default, Named map and Identity.  DELEGATE_TO_UNIT_1
 * is internal to multi-unit symbols and is resolved away by SCH_SYMBOL::GetPinMapOverride, so it is
 * never offered here.
 */
class SCH_SYMBOL_PIN_MAP_MODE_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_PIN_MAP_MODE_PROPERTY() :
            PROPERTY_BASE( PIN_MAP_MODE_PROP )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t TypeHash() const override { return TYPE_HASH( int ); }

    static wxPGChoices BuildChoices()
    {
        wxPGChoices choices;
        choices.Add( _( "Library default" ), static_cast<int>( PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT ) );
        choices.Add( _( "Named map" ), static_cast<int>( PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP ) );
        choices.Add( _( "Identity" ), static_cast<int>( PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY ) );

        return choices;
    }

    void setter( void* obj, wxAny& v ) override
    {
        int value = 0;

        if( !v.GetAs( &value ) )
            return;

        SCH_SYMBOL* symbol = reinterpret_cast<SCH_SYMBOL*>( obj );

        const SCH_SHEET_PATH* sheetPath = nullptr;
        wxString              variantName;

        if( symbol->Schematic() )
        {
            sheetPath = &symbol->Schematic()->CurrentSheet();
            variantName = symbol->Schematic()->GetCurrentVariant();
        }

        PIN_MAP_INSTANCE_OVERRIDE override = symbol->GetPinMapOverride( sheetPath, variantName );
        override.m_Mode = static_cast<PIN_MAP_OVERRIDE_MODE>( value );

        // The active-map name is meaningful only in named-map mode; the library default is used
        // otherwise.  Seed it from the symbol's active associated footprint when switching to
        // named-map mode and no name has been chosen yet.
        if( override.m_Mode == PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP && override.m_ActiveMapName.IsEmpty() )
        {
            if( const ASSOCIATED_FOOTPRINT* active = activeAssociatedFootprint( symbol ) )
                override.m_ActiveMapName = active->m_MapName;
        }

        symbol->SetPinMapOverride( override, sheetPath, variantName );
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );

        const SCH_SHEET_PATH* sheetPath = nullptr;
        wxString              variantName;

        if( symbol->Schematic() )
        {
            sheetPath = &symbol->Schematic()->CurrentSheet();
            variantName = symbol->Schematic()->GetCurrentVariant();
        }

        PIN_MAP_INSTANCE_OVERRIDE override = symbol->GetPinMapOverride( sheetPath, variantName );

        // Internal delegation is resolved away by GetPinMapOverride, but guard against any leak by
        // mapping unknown modes to the library default for display.
        switch( override.m_Mode )
        {
        case PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT:
        case PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP:
        case PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY: return wxAny( static_cast<int>( override.m_Mode ) );

        default: return wxAny( static_cast<int>( PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT ) );
        }
    }
};


class SCH_SYMBOL_PIN_MAP_NAME_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_PIN_MAP_NAME_PROPERTY() :
            PROPERTY_BASE( PIN_MAP_NAME_PROP )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t TypeHash() const override { return TYPE_HASH( int ); }

    static std::vector<wxString> MapNames( const SCH_SYMBOL* aSymbol )
    {
        std::vector<wxString> names;

        if( const LIB_SYMBOL* lib = aSymbol->GetLibSymbolRef().get() )
        {
            for( const PIN_MAP& map : lib->GetEffectivePinMaps().GetAll() )
                names.push_back( map.GetName() );
        }

        return names;
    }

    static wxPGChoices BuildChoices( const SCH_SYMBOL* aSymbol )
    {
        wxPGChoices                 choices;
        const std::vector<wxString> names = MapNames( aSymbol );

        for( size_t ii = 0; ii < names.size(); ++ii )
            choices.Add( names[ii], (int) ii );

        return choices;
    }

    void setter( void* obj, wxAny& v ) override
    {
        int value = 0;

        if( !v.GetAs( &value ) )
            return;

        SCH_SYMBOL*                 symbol = reinterpret_cast<SCH_SYMBOL*>( obj );
        const std::vector<wxString> names = MapNames( symbol );

        if( value < 0 || value >= (int) names.size() )
            return;

        const SCH_SHEET_PATH* sheetPath = nullptr;
        wxString              variantName;

        if( symbol->Schematic() )
        {
            sheetPath = &symbol->Schematic()->CurrentSheet();
            variantName = symbol->Schematic()->GetCurrentVariant();
        }

        PIN_MAP_INSTANCE_OVERRIDE override = symbol->GetPinMapOverride( sheetPath, variantName );
        override.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
        override.m_ActiveMapName = names[value];
        symbol->SetPinMapOverride( override, sheetPath, variantName );
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );

        const SCH_SHEET_PATH* sheetPath = nullptr;
        wxString              variantName;

        if( symbol->Schematic() )
        {
            sheetPath = &symbol->Schematic()->CurrentSheet();
            variantName = symbol->Schematic()->GetCurrentVariant();
        }

        PIN_MAP_INSTANCE_OVERRIDE override = symbol->GetPinMapOverride( sheetPath, variantName );

        wxString active;

        if( override.m_Mode == PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP )
            active = override.m_ActiveMapName;

        const std::vector<wxString> names = MapNames( symbol );

        // A stale named-map reference falls back to the library default for display, matching resolution.
        if( std::find( names.begin(), names.end(), active ) == names.end() )
        {
            if( const ASSOCIATED_FOOTPRINT* fp = activeAssociatedFootprint( symbol ) )
                active = fp->m_MapName;
        }

        for( size_t ii = 0; ii < names.size(); ++ii )
        {
            if( names[ii] == active )
                return wxAny( (int) ii );
        }

        return wxAny( 0 );
    }
};


// Stable group caption for the effective pin->pad table rows (issue #2282).  Nested under the Pin
// Map group as one read-only child property per symbol pin.
static const wxString PIN_MAP_TABLE_GROUP = _HKI( "Effective Pin Map" );

// Caption prefix for a per-pin table property.  The property name doubles as its displayed label in
// wxPropertyGrid, so it is kept human-readable ("Pin <number>") and untranslated for a stable,
// locale-independent PROPERTY_MANAGER lookup key.
static const wxString PIN_MAP_ENTRY_PREFIX = wxS( "Pin " );


/**
 * Read-only per-pin row of the effective pin->pad table (issue #2282).
 *
 * One property is registered per distinct symbol pin number; its value is the effective pad the pin
 * resolves to for the current sheet and variant.  A pin that is not remapped resolves 1:1 and is
 * shown as "<pad> (identity)", so a reviewer can see at a glance which pins the active map remaps.
 */
class SCH_SYMBOL_PIN_MAP_ENTRY_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_PIN_MAP_ENTRY_PROPERTY( const wxString& aName, const wxString& aPinNumber ) :
            PROPERTY_BASE( aName ),
            m_pinNumber( aPinNumber )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override { return false; }

    void setter( void* obj, wxAny& v ) override {}

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );

        const SCH_SHEET_PATH* sheetPath = nullptr;
        wxString              variantName;

        if( symbol->Schematic() )
        {
            sheetPath = &symbol->Schematic()->CurrentSheet();
            variantName = symbol->Schematic()->GetCurrentVariant();
        }

        if( !sheetPath )
            return wxAny( m_pinNumber );

        for( const SCH_PIN* pin : symbol->GetPins( sheetPath ) )
        {
            std::vector<wxString> logical = pin->GetStackedPinNumbers();

            if( std::find( logical.begin(), logical.end(), m_pinNumber ) == logical.end() )
                continue;

            wxString pad = pin->GetEffectivePadNumber( *sheetPath, variantName );

            if( pad == m_pinNumber )
                return wxAny( wxString::Format( _( "%s (identity)" ), pad ) );

            return wxAny( pad );
        }

        return wxAny( m_pinNumber );
    }

private:
    wxString m_pinNumber;
};


std::set<wxString> SCH_PROPERTIES_PANEL::m_currentSymbolFieldNames;
std::set<wxString> SCH_PROPERTIES_PANEL::m_currentSheetFieldNames;
std::set<wxString> SCH_PROPERTIES_PANEL::m_currentPinMapPinNumbers;

SCH_PROPERTIES_PANEL::SCH_PROPERTIES_PANEL( wxWindow* aParent, SCH_BASE_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() ),
        m_unitEditorInstance( nullptr ),
        m_checkboxEditorInstance( nullptr ),
        m_colorEditorInstance( nullptr ),
        m_fpEditorInstance( nullptr ),
        m_urlEditorInstance( nullptr ),
        m_editPinMapButton( nullptr )
{
    // Pin Map editor launcher (issue #2282).  The button lives below the property grid and is only
    // shown when a single symbol with an effective associated footprint is selected.
    m_editPinMapButton = new wxButton( this, wxID_ANY, _( "Edit Pin Map..." ) );
    m_editPinMapButton->Hide();
    GetSizer()->Add( m_editPinMapButton, 0, wxALL | wxEXPAND, 5 );
    m_editPinMapButton->Bind( wxEVT_BUTTON, &SCH_PROPERTIES_PANEL::onEditPinMap, this );

    m_propMgr.Rebuild();
    bool found = false;

    wxASSERT( wxPGGlobalVars );

    wxString editorKey = PG_UNIT_EDITOR::BuildEditorName( m_frame );

    auto it = wxPGGlobalVars->m_mapEditorClasses.find( editorKey );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( it->second );
        m_unitEditorInstance->UpdateFrame( m_frame );
        found = true;
    }

    if( !found )
    {
        PG_UNIT_EDITOR* new_editor = new PG_UNIT_EDITOR( m_frame );
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( wxPropertyGrid::RegisterEditorClass( new_editor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_CHECKBOX_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_CHECKBOX_EDITOR* cbEditor = new PG_CHECKBOX_EDITOR();
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( wxPropertyGrid::RegisterEditorClass( cbEditor ) );
    }
    else
    {
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_COLOR_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_COLOR_EDITOR* colorEditor = new PG_COLOR_EDITOR();
        m_colorEditorInstance = static_cast<PG_COLOR_EDITOR*>( wxPropertyGrid::RegisterEditorClass( colorEditor ) );
    }
    else
    {
        m_colorEditorInstance = static_cast<PG_COLOR_EDITOR*>( it->second );
    }

    auto netlistCallback = [this]()
    {
        SCH_SELECTION& sel = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>()->GetSelection();
        LIB_SYMBOL*    libSymbol = nullptr;

        for( EDA_ITEM* item : sel )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( !libSymbol )
                    libSymbol = symbol->GetLibSymbolRef().get();
                else if( libSymbol != symbol->GetLibSymbolRef().get() )
                    return std::string( "" );
            }
        }

        if( !libSymbol )
            return std::string( "" );

        wxString      symbolNetlist;
        wxArrayString pins;

        for( SCH_PIN* pin : libSymbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
            pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );

        if( !pins.IsEmpty() )
            symbolNetlist << EscapeString( wxJoin( pins, '\t' ), CTX_LINE );

        symbolNetlist << wxS( "\r" );

        wxArrayString fpFilters = libSymbol->GetFPFilters();

        if( !fpFilters.IsEmpty() )
            symbolNetlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );

        symbolNetlist << wxS( "\r" );

        return symbolNetlist.ToStdString();
    };

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_FPID_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( it->second );
        m_fpEditorInstance->UpdateFrame( m_frame );
        m_fpEditorInstance->UpdateCallback( netlistCallback );
    }
    else
    {
        PG_FPID_EDITOR* fpEditor = new PG_FPID_EDITOR( m_frame, netlistCallback );
        m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( wxPropertyGrid::RegisterEditorClass( fpEditor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( it->second );
        m_urlEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_URL_EDITOR* urlEditor = new PG_URL_EDITOR( m_frame );
        m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( wxPropertyGrid::RegisterEditorClass( urlEditor ) );
    }
}


SCH_PROPERTIES_PANEL::~SCH_PROPERTIES_PANEL()
{
    m_unitEditorInstance->UpdateFrame( nullptr );
    m_fpEditorInstance->UpdateFrame( nullptr );
    m_urlEditorInstance->UpdateFrame( nullptr );
}


const SELECTION& SCH_PROPERTIES_PANEL::getSelection( SELECTION& aFallbackSelection )
{
    SCH_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() && m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* symbolFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

        if( symbolFrame->GetCurSymbol() )
        {
            aFallbackSelection.Clear();
            aFallbackSelection.Add( symbolFrame->GetCurSymbol() );
            return aFallbackSelection;
        }
    }

    return selection;
}


EDA_ITEM* SCH_PROPERTIES_PANEL::getFrontItem()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    return selection.Empty() ? nullptr : selection.Front();
}


void SCH_PROPERTIES_PANEL::UpdateData()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::AfterCommit()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    m_currentSymbolFieldNames.clear();
    m_currentSheetFieldNames.clear();
    m_currentPinMapPinNumbers.clear();

    // The effective pin->pad table is only meaningful for a single pin-mapped symbol; collecting the
    // pin numbers here lets the per-pin rows hide for everything else.
    if( SCH_SYMBOL* pinMapped = getSinglePinMappedSymbol() )
    {
        const SCH_SHEET_PATH* sheetPath = pinMapped->Schematic() ? &pinMapped->Schematic()->CurrentSheet() : nullptr;

        if( sheetPath )
        {
            // Use logical (stacked-expanded) pin numbers so the rows match the editor grid's keys.
            for( const SCH_PIN* pin : pinMapped->GetPins( sheetPath ) )
            {
                for( const wxString& number : pin->GetStackedPinNumbers() )
                {
                    if( !number.IsEmpty() )
                        m_currentPinMapPinNumbers.insert( number );
                }
            }
        }
    }

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( const SCH_FIELD& field : symbol->GetFields() )
            {
                if( field.IsPrivate() )
                    continue;

                m_currentSymbolFieldNames.insert( field.GetCanonicalName() );
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

            for( const SCH_FIELD& field : sheet->GetFields() )
            {
                if( field.IsPrivate() )
                    continue;

                m_currentSheetFieldNames.insert( field.GetCanonicalName() );
            }
        }
    }

    const wxString groupFields = _HKI( "Fields" );

    for( const wxString& name : m_currentSymbolFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), name ) )
        {
            m_propMgr.AddProperty( new SCH_SYMBOL_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc(
                            [name]( INSPECTABLE* )
                            {
                                return SCH_PROPERTIES_PANEL::m_currentSymbolFieldNames.count( name );
                            } );
        }
    }

    for( const wxString& name : m_currentSheetFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SHEET ), name ) )
        {
            m_propMgr.AddProperty( new SCH_SHEET_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc(
                            [name]( INSPECTABLE* )
                            {
                                return SCH_PROPERTIES_PANEL::m_currentSheetFieldNames.count( name );
                            } );
        }
    }

    // Pin Map group (issue #2282).  These properties are only available when the symbol carries at
    // least one effective associated footprint, so they stay hidden for ordinary symbols.
    const wxString groupPinMap = PIN_MAP_GROUP;

    if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), PIN_MAP_FOOTPRINT_PROP ) )
    {
        m_propMgr.AddProperty( new SCH_SYMBOL_PIN_MAP_FOOTPRINT_PROPERTY(), groupPinMap )
                .SetAvailableFunc( symbolHasAssociatedFootprint );
    }

    if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), PIN_MAP_MODE_PROP ) )
    {
        m_propMgr.AddProperty( new SCH_SYMBOL_PIN_MAP_MODE_PROPERTY(), groupPinMap )
                .SetAvailableFunc( symbolHasAssociatedFootprint )
                .SetChoicesFunc(
                        []( INSPECTABLE* ) -> wxPGChoices
                        {
                            return SCH_SYMBOL_PIN_MAP_MODE_PROPERTY::BuildChoices();
                        } );
    }

    if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), PIN_MAP_NAME_PROP ) )
    {
        m_propMgr.AddProperty( new SCH_SYMBOL_PIN_MAP_NAME_PROPERTY(), groupPinMap )
                .SetAvailableFunc(
                        []( INSPECTABLE* aObject )
                        {
                            const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( aObject );

                            return symbol && symbolHasAssociatedFootprint( aObject )
                                   && SCH_SYMBOL_PIN_MAP_NAME_PROPERTY::MapNames( symbol ).size() > 1;
                        } )
                .SetChoicesFunc(
                        []( INSPECTABLE* aObject ) -> wxPGChoices
                        {
                            if( const SCH_SYMBOL* symbol = dynamic_cast<const SCH_SYMBOL*>( aObject ) )
                                return SCH_SYMBOL_PIN_MAP_NAME_PROPERTY::BuildChoices( symbol );

                            return wxPGChoices();
                        } );
    }

    // One read-only row per symbol pin under a collapsible group, showing the effective pad each pin
    // resolves to (issue #2282).  wxPropertyGrid has no table widget, so the table is expressed as
    // nested category rows; they register once per pin number and gate on the current selection.
    const wxString groupPinMapTable = PIN_MAP_TABLE_GROUP;

    for( const wxString& pinNumber : m_currentPinMapPinNumbers )
    {
        const wxString propName = PIN_MAP_ENTRY_PREFIX + pinNumber;

        if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), propName ) )
        {
            m_propMgr.AddProperty( new SCH_SYMBOL_PIN_MAP_ENTRY_PROPERTY( propName, pinNumber ), groupPinMapTable )
                    .SetAvailableFunc(
                            [pinNumber]( INSPECTABLE* )
                            {
                                return SCH_PROPERTIES_PANEL::m_currentPinMapPinNumbers.count( pinNumber );
                            } );
        }
    }

    PROPERTIES_PANEL::rebuildProperties( aSelection );

    // The Edit Pin Map button targets the schematic-editor symbol properties dialog, so it is only
    // shown there for a single pin-mapped symbol.
    bool showEditButton = m_frame->IsType( FRAME_SCH ) && getSinglePinMappedSymbol() != nullptr;

    if( m_editPinMapButton && m_editPinMapButton->IsShown() != showEditButton )
    {
        m_editPinMapButton->Show( showEditButton );
        Layout();
    }
}


SCH_SYMBOL* SCH_PROPERTIES_PANEL::getSinglePinMappedSymbol()
{
    SELECTION        fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    if( selection.Size() != 1 || selection.Front()->Type() != SCH_SYMBOL_T )
        return nullptr;

    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( selection.Front() );

    return symbolHasAssociatedFootprint( symbol ) ? symbol : nullptr;
}


void SCH_PROPERTIES_PANEL::onEditPinMap( wxCommandEvent& aEvent )
{
    SCH_SYMBOL* symbol = getSinglePinMappedSymbol();

    if( !symbol || !m_frame->IsType( FRAME_SCH ) )
        return;

    SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( m_frame );

    DIALOG_SYMBOL_PROPERTIES dlg( editFrame, symbol );
    dlg.SelectPinMapPage();

    // The dialog can subsequently invoke a KIWAY_PLAYER as a quasimodal frame, so it must run
    // quasimodally to keep that support working.
    if( dlg.ShowQuasiModal() == SYMBOL_PROPS_EDIT_OK )
    {
        editFrame->OnModify();
        AfterCommit();
    }
}


wxPGProperty* SCH_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    wxPGProperty* prop = PGPropertyFactory( aProperty, m_frame );

    if( auto colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( prop ) )
    {
        COLOR4D bg = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
        colorProp->SetBackgroundColor( bg );
    }

    if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        prop->SetEditor( PG_FPID_EDITOR::BuildEditorName( m_frame ) );
    else if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        prop->SetEditor( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    return prop;
}


PROPERTY_BASE* SCH_PROPERTIES_PANEL::getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const
{
    EDA_ITEM* item = const_cast<SCH_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item || !item->IsSCH_ITEM() )
        return nullptr;

    SCH_ITEM* firstItem = static_cast<SCH_ITEM*>( item );

    wxCHECK_MSG( firstItem, nullptr, wxT( "getPropertyFromEvent for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ), aEvent.GetPropertyName() );
    wxCHECK_MSG( property, nullptr, wxT( "getPropertyFromEvent for a property not found on the selected item!" ) );

    return property;
}


void SCH_PROPERTIES_PANEL::valueChanging( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents )
        return;

    EDA_ITEM* frontItem = getFrontItem();

    if( !frontItem )
        return;

    if( PROPERTY_BASE* property = getPropertyFromEvent( aEvent ) )
    {
        wxVariant newValue = aEvent.GetPropertyValue();

        if( VALIDATOR_RESULT validationFailure = property->Validate( newValue.GetAny(), frontItem ) )
        {
            wxString errorMsg = wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                                  validationFailure->get()->Format( m_frame ) );
            m_frame->ShowInfoBarError( errorMsg );
            aEvent.Veto();
            return;
        }

        aEvent.Skip();
    }
}


void SCH_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    wxCHECK( getPropertyFromEvent( aEvent ), /* void */ );

    wxVariant   newValue = aEvent.GetPropertyValue();
    SCH_COMMIT  changes( m_frame );
    SCH_SCREEN* screen = m_frame->GetScreen();

    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* edaItem : selection )
    {
        if( !edaItem->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
        PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *item ), aEvent.GetPropertyName() );
        wxCHECK2( property, continue );

        // Editing reference text in the schematic must go through the parent symbol in order to handle
        // symbol instance data properly.
        if( item->Type() == SCH_FIELD_T && static_cast<SCH_FIELD*>( item )->GetId() == FIELD_T::REFERENCE
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() );
            wxCHECK2( symbol, continue );

            changes.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
            symbol->SetRefProp( newValue.GetString() );
            symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property );
            continue;
        }

        // Editing field text in the schematic when a variant is active must use variant-aware
        // SetText to properly store the value as a variant override.
        if( item->Type() == SCH_FIELD_T
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() );

            if( symbol && symbol->Schematic() )
            {
                wxString variantName = symbol->Schematic()->GetCurrentVariant();

                if( !variantName.IsEmpty() )
                {
                    changes.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
                    field->SetText( newValue.GetString(), &symbol->Schematic()->CurrentSheet(), variantName );
                    symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, nullptr, variantName );
                    continue;
                }
            }
        }

        // Changing a sheet's filename field requires file operations to match the dialog behavior.
        if( item->Type() == SCH_FIELD_T
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );
            SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item->GetParent() );

            if( sheet && field->GetId() == FIELD_T::SHEET_FILENAME )
            {
                SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( m_frame );

                if( !handleSheetFilenameChange( editFrame, sheet, changes, newValue.GetString() ) )
                {
                    UpdateData();
                    return;
                }

                continue;
            }
        }

        if( item->Type() == SCH_TABLECELL_T )
            changes.Modify( item->GetParent(), screen, RECURSE_MODE::NO_RECURSE );
        else
            changes.Modify( item, screen, RECURSE_MODE::NO_RECURSE );

        item->Set( property, newValue );

        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
        {
            symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property,
                                    symbol->Schematic()->GetCurrentVariant() );
        }
    }

    changes.Push( _( "Edit Properties" ) );

    // Force a repaint of the items whose properties were changed
    // This is necessary to update field displays in the schematic view
    for( EDA_ITEM* edaItem : selection )
        m_frame->UpdateItem( edaItem );

    // Perform grid updates as necessary based on value change
    AfterCommit();

    aEvent.Skip();
}


bool SCH_PROPERTIES_PANEL::handleSheetFilenameChange( SCH_EDIT_FRAME* aFrame, SCH_SHEET* aSheet,
                                                       SCH_COMMIT& aChanges,
                                                       const wxString& aNewFilename )
{
    wxString newFilename = EnsureFileExtension( aNewFilename, FILEEXT::KiCadSchematicFileExtension );

    if( newFilename.IsEmpty() || !IsFullFileNameValid( newFilename ) )
    {
        DisplayError( aFrame, _( "A sheet must have a valid file name." ) );
        return false;
    }

    // Normalize separators to unix notation
    newFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    wxString oldFilename = aSheet->GetFileName();
    oldFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    if( newFilename == oldFilename )
        return true;

    if( !aFrame->ChangeSheetFile( aSheet, newFilename ) )
        return false;

    SCH_SCREEN* currentScreen = aFrame->GetCurrentSheet().LastScreen();
    aChanges.Modify( aSheet, currentScreen, RECURSE_MODE::NO_RECURSE );
    aSheet->SetFileName( newFilename );

    return true;
}


void SCH_PROPERTIES_PANEL::OnLanguageChanged( wxCommandEvent& aEvent )
{
    PROPERTIES_PANEL::OnLanguageChanged( aEvent );

    aEvent.Skip();
}


bool SCH_PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    // For SCH_FIELD "Text" property, return the variant-aware value when a variant is active
    if( aItem->Type() == SCH_FIELD_T
            && m_frame->IsType( FRAME_SCH )
            && aProperty->Name() == wxT( "Text" ) )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( aItem );
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( field->GetParentSymbol() );

        if( symbol && symbol->Schematic() )
        {
            wxString variantName = symbol->Schematic()->GetCurrentVariant();

            if( !variantName.IsEmpty() )
            {
                wxString text = field->GetText( &symbol->Schematic()->CurrentSheet(), variantName );
                aValue = wxVariant( text );
                return true;
            }
        }
    }

    return PROPERTIES_PANEL::getItemValue( aItem, aProperty, aValue );
}
