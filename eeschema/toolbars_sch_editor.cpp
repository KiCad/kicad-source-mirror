/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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

#include <advanced_config.h>
#include <api/api_plugin_manager.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <kiface_base.h>
#include <bitmaps.h>
#include <eeschema_id.h>
#include <pgm_base.h>
#include <python_scripting.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tool/ui/toolbar_context_menu_registry.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <widgets/hierarchy_pane.h>
#include <widgets/wx_aui_utils.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_search_pane.h>
#include <toolbars_sch_editor.h>
#include <wx/choice.h>
#include <wx/stattext.h>


ACTION_TOOLBAR_CONTROL SCH_ACTION_TOOLBAR_CONTROLS::currentVariant( "control.currentVariant",
                                                                    _( "Current variant" ),
                                                                    _( "Selects the current schematic variant" ),
                                                                    { FRAME_SCH } );


std::optional<TOOLBAR_CONFIGURATION> SCH_EDIT_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::toggleGrid )
              .WithContextMenu(
                      []( TOOL_MANAGER* aToolMgr )
                      {
                          SCH_SELECTION_TOOL* selTool = aToolMgr->GetTool<SCH_SELECTION_TOOL>();
                          auto               menu = std::make_unique<ACTION_MENU>( false, selTool );
                          menu->Add( ACTIONS::gridProperties );
                          return menu;
                      } )
              .AppendAction( ACTIONS::toggleGridOverrides )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                            .AddAction( ACTIONS::inchesUnits )
                            .AddAction( ACTIONS::milsUnits )
                            .AddAction( ACTIONS::millimetersUnits ) )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Crosshair modes" ) )
                            .AddAction( ACTIONS::cursorSmallCrosshairs )
                            .AddAction( ACTIONS::cursorFullCrosshairs )
                            .AddAction( ACTIONS::cursor45Crosshairs ) );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::toggleHiddenPins );

        config.AppendSeparator()
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Line modes" ) )
                            .AddAction( SCH_ACTIONS::lineModeFree )
                            .AddAction( SCH_ACTIONS::lineMode90 )
                            .AddAction( SCH_ACTIONS::lineMode45 ) );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::toggleAnnotateAuto );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::showHierarchy )
              .AppendAction( ACTIONS::showProperties );

        if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
            config.AppendAction( ACTIONS::toggleBoundingBoxes );

        break;

    case TOOLBAR_LOC::RIGHT:
        config.AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Selection modes" ) )
                            .AddAction( ACTIONS::selectSetRect )
                            .AddAction( ACTIONS::selectSetLasso ) )
              .AppendAction( SCH_ACTIONS::highlightNetTool );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::placeSymbol )
              .AppendAction( SCH_ACTIONS::placePower )
              .AppendAction( SCH_ACTIONS::drawWire )
              .AppendAction( SCH_ACTIONS::drawBus )
              .AppendAction( SCH_ACTIONS::placeBusWireEntry )
              .AppendAction( SCH_ACTIONS::placeNoConnect )
              .AppendAction( SCH_ACTIONS::placeJunction )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Labels" ) )
                            .AddAction( SCH_ACTIONS::placeLabel )
                            .AddAction( SCH_ACTIONS::placeClassLabel )
                            .AddAction( SCH_ACTIONS::placeGlobalLabel )
                            .AddAction( SCH_ACTIONS::placeHierLabel ) )
              .AppendAction( SCH_ACTIONS::drawRuleArea )
              .AppendAction( SCH_ACTIONS::drawSheet )
              .AppendAction( SCH_ACTIONS::placeSheetPin )
              .AppendAction( SCH_ACTIONS::syncAllSheetsPins );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::placeSchematicText )
              .AppendAction( SCH_ACTIONS::drawTextBox )
              .AppendAction( SCH_ACTIONS::drawTable )
              .AppendAction( SCH_ACTIONS::drawRectangle )
              .AppendAction( SCH_ACTIONS::drawCircle )
              .AppendAction( SCH_ACTIONS::drawArc )
              .AppendAction( SCH_ACTIONS::drawBezier )
              .AppendAction( SCH_ACTIONS::drawLines )
              .AppendAction( SCH_ACTIONS::placeImage )
              .AppendAction( ACTIONS::deleteTool );

        break;

    case TOOLBAR_LOC::TOP_MAIN:
        if( Kiface().IsSingle() )   // not when under a project mgr
        {
            config.AppendAction( ACTIONS::doNew );
            config.AppendAction( ACTIONS::open );
        }

        config.AppendAction( ACTIONS::save );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::schematicSetup );

        config.AppendSeparator()
              .AppendAction( ACTIONS::pageSettings )
              .AppendAction( ACTIONS::print )
              .AppendAction( ACTIONS::plot );

        config.AppendSeparator()
              .AppendAction( ACTIONS::paste );

        config.AppendSeparator()
              .AppendAction( ACTIONS::undo )
              .AppendAction( ACTIONS::redo );

        config.AppendSeparator()
              .AppendAction( ACTIONS::find )
              .AppendAction( ACTIONS::findAndReplace );

        config.AppendSeparator()
              .AppendAction( ACTIONS::zoomRedraw )
              .AppendAction( ACTIONS::zoomInCenter )
              .AppendAction( ACTIONS::zoomOutCenter )
              .AppendAction( ACTIONS::zoomFitScreen )
              .AppendAction( ACTIONS::zoomFitObjects )
              .AppendAction( ACTIONS::zoomTool );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::navigateBack )
              .AppendAction( SCH_ACTIONS::navigateUp )
              .AppendAction( SCH_ACTIONS::navigateForward );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::rotateCCW )
              .AppendAction( SCH_ACTIONS::rotateCW )
              .AppendAction( SCH_ACTIONS::mirrorV )
              .AppendAction( SCH_ACTIONS::mirrorH )
              .AppendAction( ACTIONS::group )
              .AppendAction( ACTIONS::ungroup );

        config.AppendSeparator()
              .AppendAction( ACTIONS::showSymbolEditor )
              .AppendAction( ACTIONS::showSymbolBrowser )
              .AppendAction( ACTIONS::showFootprintEditor );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::annotate )
              .AppendAction( SCH_ACTIONS::runERC )
              .AppendAction( SCH_ACTIONS::showSimulator )
              .AppendAction( SCH_ACTIONS::assignFootprints )
              .AppendAction( SCH_ACTIONS::editSymbolFields )
              .AppendAction( SCH_ACTIONS::generateBOM );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::showPcbNew );

        config.AppendControl( SCH_ACTION_TOOLBAR_CONTROLS::currentVariant );

        // Insert all the IPC plugins here on the toolbar
        // TODO (ISM): Move this to individual actions for each script
        config.AppendControl( ACTION_TOOLBAR_CONTROLS::ipcScripting );

        break;
    }

    // clang-format on
    return config;
}


void SCH_EDIT_FRAME::configureToolbars()
{
    SCH_BASE_FRAME::configureToolbars();

    // Variant selection drop down control on main tool bar.
    auto variantSelectionCtrlFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                std::optional<wxString> currentVariantName = Schematic().GetCurrentVariant();
                wxString tmp = currentVariantName ? *currentVariantName : GetDefaultVariantName();

                m_currentVariantCtrl = new wxChoice( aToolbar, ID_TOOLBAR_SCH_SELECT_VARAIANT, wxDefaultPosition,
                                                     wxDefaultSize, Schematic().GetVariantNamesForUI(), 0,
                                                     wxDefaultValidator, tmp );

                m_currentVariantCtrl->SetToolTip( _( "Select the current variant to display and edit." ) );
                aToolbar->Add( m_currentVariantCtrl );
                UpdateVariantSelectionCtrl( Schematic().GetVariantNamesForUI() );
            };

    RegisterCustomToolbarControlFactory( SCH_ACTION_TOOLBAR_CONTROLS::currentVariant, variantSelectionCtrlFactory );

    // IPC/Scripting plugin control
    // TODO (ISM): Clean this up to make IPC actions just normal tool actions to get rid of this entire
    // control
    auto pluginControlFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                // Add scripting console and API plugins
                bool scriptingAvailable = SCRIPTING::IsWxAvailable();

#ifdef KICAD_IPC_API
                bool haveApiPlugins = Pgm().GetCommonSettings()->m_Api.enable_server
                                        && !Pgm().GetPluginManager().GetActionsForScope( PluginActionScope() ).empty();
#else
                bool haveApiPlugins = false;
#endif

                if( scriptingAvailable || haveApiPlugins )
                {
                    aToolbar->AddScaledSeparator( aToolbar->GetParent() );

                    if( haveApiPlugins )
                        AddApiPluginTools( aToolbar );
                }
            };

    RegisterCustomToolbarControlFactory( ACTION_TOOLBAR_CONTROLS::ipcScripting, pluginControlFactory );
}


void SCH_EDIT_FRAME::ClearToolbarControl( int aId )
{
    SCH_BASE_FRAME::ClearToolbarControl( aId );

    switch( aId )
    {
    case ID_TOOLBAR_SCH_SELECT_VARAIANT: m_currentVariantCtrl = nullptr; break;
    }
}


void SCH_EDIT_FRAME::UpdateVariantSelectionCtrl( const wxArrayString& aVariantNames )
{
    if( !m_currentVariantCtrl )
        return;

    // Fall back to the default if nothing is currently selected.
    wxString currentSelection = GetDefaultVariantName();
    int selectionIndex = m_currentVariantCtrl->GetSelection();

    if( selectionIndex != wxNOT_FOUND )
        currentSelection = m_currentVariantCtrl->GetString( selectionIndex );

    // Add all variant names, a separator, and "Add New Variant..." at the end
    wxArrayString contents = aVariantNames;
    contents.Add( wxS( "---" ) );
    contents.Add( _( "Add New Variant..." ) );

    m_currentVariantCtrl->Set( contents );

    selectionIndex = m_currentVariantCtrl->FindString( currentSelection );

    if( ( selectionIndex == wxNOT_FOUND ) && ( m_currentVariantCtrl->GetCount() != 0 ) )
        selectionIndex = 0;

    m_currentVariantCtrl->SetSelection( selectionIndex );
}


void SCH_EDIT_FRAME::onVariantSelected( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() != ID_TOOLBAR_SCH_SELECT_VARAIANT )
        return;

    int selection = m_currentVariantCtrl->GetSelection();
    int count = m_currentVariantCtrl->GetCount();

    if( selection == wxNOT_FOUND || count == 0 )
        return;

    // "Add New Variant..." is always the last item, separator is second-to-last
    if( selection == count - 1 )
    {
        // Restore previous selection before showing dialog
        int previousSelection = m_currentVariantCtrl->FindString(
                Schematic().GetCurrentVariant().IsEmpty() ? GetDefaultVariantName()
                                                          : Schematic().GetCurrentVariant() );

        if( previousSelection != wxNOT_FOUND )
            m_currentVariantCtrl->SetSelection( previousSelection );

        ShowAddVariantDialog();
        return;
    }

    // Separator line - ignore selection
    if( selection == count - 2 )
    {
        int previousSelection = m_currentVariantCtrl->FindString(
                Schematic().GetCurrentVariant().IsEmpty() ? GetDefaultVariantName()
                                                          : Schematic().GetCurrentVariant() );

        if( previousSelection != wxNOT_FOUND )
            m_currentVariantCtrl->SetSelection( previousSelection );

        return;
    }

    wxString selectedString = m_currentVariantCtrl->GetString( selection );
    wxString selectedVariant;

    if( selectedString != GetDefaultVariantName() )
        selectedVariant = selectedString;

    Schematic().SetCurrentVariant( selectedVariant );
    UpdateProperties();
    HardRedraw();
}


bool SCH_EDIT_FRAME::ShowAddVariantDialog()
{
    // Create a dialog with both name and description fields
    wxDialog dlg( this, wxID_ANY, _( "New Variant" ), wxDefaultPosition, wxDefaultSize,
                  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // Name field
    wxStaticText* nameLabel = new wxStaticText( &dlg, wxID_ANY, _( "Variant name:" ) );
    mainSizer->Add( nameLabel, 0, wxALL | wxEXPAND, 5 );

    wxTextCtrl* nameCtrl = new wxTextCtrl( &dlg, wxID_ANY );
    mainSizer->Add( nameCtrl, 0, wxALL | wxEXPAND, 5 );

    // Description field
    wxStaticText* descLabel = new wxStaticText( &dlg, wxID_ANY, _( "Description (optional):" ) );
    mainSizer->Add( descLabel, 0, wxALL | wxEXPAND, 5 );

    wxTextCtrl* descCtrl = new wxTextCtrl( &dlg, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                           wxSize( 300, 60 ), wxTE_MULTILINE );
    mainSizer->Add( descCtrl, 1, wxALL | wxEXPAND, 5 );

    // Buttons
    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton( new wxButton( &dlg, wxID_OK ) );
    btnSizer->AddButton( new wxButton( &dlg, wxID_CANCEL ) );
    btnSizer->Realize();
    mainSizer->Add( btnSizer, 0, wxALL | wxALIGN_RIGHT, 5 );

    dlg.SetSizer( mainSizer );
    dlg.Fit();
    dlg.Centre();

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    wxString variantName = nameCtrl->GetValue().Trim().Trim( false );
    wxString variantDesc = descCtrl->GetValue().Trim().Trim( false );

    // Empty strings, reserved names, and duplicate variant names are not allowed.
    if( variantName.IsEmpty() )
    {
        GetInfoBar()->ShowMessageFor( _( "Variant name cannot be empty." ),
                                       10000, wxICON_ERROR );
        return false;
    }

    // Check for reserved name (case-insensitive)
    if( variantName.CmpNoCase( GetDefaultVariantName() ) == 0 )
    {
        GetInfoBar()->ShowMessageFor( wxString::Format( _( "'%s' is a reserved variant name." ),
                                                         GetDefaultVariantName() ),
                                       10000, wxICON_ERROR );
        return false;
    }

    // Check for duplicate variant names (case-insensitive)
    for( const wxString& existingName : Schematic().GetVariantNames() )
    {
        if( existingName.CmpNoCase( variantName ) == 0 )
        {
            GetInfoBar()->ShowMessageFor( wxString::Format( _( "Variant '%s' already exists." ),
                                                             existingName ),
                                           10000, wxICON_ERROR );
            return false;
        }
    }

    // Add variant to the schematic
    Schematic().AddVariant( variantName );

    if( !variantDesc.IsEmpty() )
        Schematic().SetVariantDescription( variantName, variantDesc );

    // Update the variant selector and select the new variant
    UpdateVariantSelectionCtrl( Schematic().GetVariantNamesForUI() );
    SetCurrentVariant( variantName );
    UpdateProperties();
    HardRedraw();
    return true;
}


void SCH_EDIT_FRAME::SetCurrentVariant( const wxString& aVariantName )
{
    if( !m_currentVariantCtrl )
        return;

    wxString name = aVariantName.IsEmpty() ? GetDefaultVariantName() : aVariantName;

    int newSelection = m_currentVariantCtrl->FindString( name );

    if( newSelection == wxNOT_FOUND )
        return;

    int currentSelection = m_currentVariantCtrl->GetSelection();

    wxString selectedString;

    if( currentSelection != wxNOT_FOUND )
        selectedString = m_currentVariantCtrl->GetString( currentSelection );

    if( selectedString != name )
    {
        m_currentVariantCtrl->SetSelection( newSelection );
        Schematic().SetCurrentVariant( aVariantName );
    }
}
