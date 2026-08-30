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
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_field.h>
#include <pin_map.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>
#include <wx/button.h>


bool SCH_PROPERTIES_PANEL::m_selContainsJunctions;
bool SCH_PROPERTIES_PANEL::m_selContainsWiresOrBuses;

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
    m_selContainsJunctions = false;
    m_selContainsWiresOrBuses = false;

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() == SCH_JUNCTION_T )
        {
            m_selContainsJunctions = true;

            // Dummy property which allows concurrently-selected wires to be edited
            m_propMgr.AddProperty( new DUMMY_PROPERTY<SCH_JUNCTION, WIRE_STYLE>( _HKI( "Wire Style" ) ) )
                    .SetAvailableFunc(
                            []( INSPECTABLE* )
                            {
                                return m_selContainsWiresOrBuses;
                            } );

            // Dummy property which allows concurrently-selected wires to be edited
            m_propMgr.AddProperty( new DUMMY_PROPERTY<SCH_JUNCTION, int>( _HKI( "Line Width" ) ) )
                    .SetAvailableFunc(
                            []( INSPECTABLE* )
                            {
                                return m_selContainsWiresOrBuses;
                            } );
        }
        else if( item->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } ) )
        {
            m_selContainsWiresOrBuses = true;

            // Dummy property which allows concurrently-selected junctions to be edited
            m_propMgr.AddProperty( new DUMMY_PROPERTY<SCH_LINE, int>( _HKI( "Diameter" ) ) )
                    .SetAvailableFunc(
                            []( INSPECTABLE* )
                            {
                                return m_selContainsJunctions;
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

    return symbol->HasEffectiveAssociatedFootprint() ? symbol : nullptr;
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
    int retval = dlg.ShowQuasiModal();

    if( retval == SYMBOL_PROPS_EDIT_OK )
    {
        editFrame->OnModify();
        AfterCommit();
    }
    else if( retval == SYMBOL_PROPS_WANT_SET_VARIANT_SYMBOL )
    {
        editFrame->GetToolManager()->RunAction( SCH_ACTIONS::setVariantSymbol );
    }
    else if( retval == SYMBOL_PROPS_WANT_CLEAR_VARIANT_SYMBOL )
    {
        editFrame->GetToolManager()->RunAction( SCH_ACTIONS::clearVariantSymbol );
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

    if( aProperty->Name() == GetDefaultFieldName( FIELD_T::FOOTPRINT, UNTRANSLATED ) )
        prop->SetEditor( PG_FPID_EDITOR::BuildEditorName( m_frame ) );
    else if( aProperty->Name() == GetDefaultFieldName( FIELD_T::DATASHEET, UNTRANSLATED ) )
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

    PROPERTY_BASE* property = m_propMgr.GetProperty( firstItem, aEvent.GetPropertyName() );
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
        PROPERTY_BASE* property = m_propMgr.GetProperty( item, aEvent.GetPropertyName() );
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
