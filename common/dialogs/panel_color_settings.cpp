/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitmaps.h>
#include <launch_ext.h>
#include <layer_ids.h>
#include <dialogs/panel_color_settings.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <validators.h>
#include <widgets/ui_common.h>
#include <widgets/color_swatch.h>
#include <widgets/wx_panel.h>

#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/textdlg.h>

// Button ID starting point
constexpr int FIRST_BUTTON_ID = 1800;


PANEL_COLOR_SETTINGS::PANEL_COLOR_SETTINGS( wxWindow* aParent ) :
        PANEL_COLOR_SETTINGS_BASE( aParent ),
        m_currentSettings( nullptr ),
        m_swatches(),
        m_copied( COLOR4D::UNSPECIFIED ),
        m_validLayers(),
        m_backgroundLayer( LAYER_PCB_BACKGROUND ),
        m_colorNamespace()
{
#ifdef __APPLE__
    m_btnOpenFolder->SetLabel( _( "Reveal Themes in Finder" ) );

    // Simple border is too dark on OSX
    m_colorsListWindow->SetWindowStyle( wxBORDER_SUNKEN|wxVSCROLL );
#endif

    m_cbTheme->SetMinSize( FromDIP( m_cbTheme->GetMinSize() ) );
    m_colorsListWindow->SetMinSize( FromDIP( m_colorsListWindow->GetMinSize() ) );
    m_colorsGridSizer->SetMinSize( FromDIP( m_colorsGridSizer->GetMinSize() ) );
    m_panel1->SetBorders( false, false, true, false );
}


void PANEL_COLOR_SETTINGS::OnBtnOpenThemeFolderClicked( wxCommandEvent& event )
{
    wxString dir( SETTINGS_MANAGER::GetColorSettingsPath() );
    LaunchExternal( dir );
}


void PANEL_COLOR_SETTINGS::ResetPanel()
{
    if( !m_currentSettings || m_currentSettings->IsReadOnly() )
        return;

    for( const std::pair<const int, COLOR_SWATCH*>& pair : m_swatches )
    {
        int           layer  = pair.first;
        COLOR_SWATCH* button = pair.second;

        COLOR4D defaultColor = m_currentSettings->GetDefaultColor( layer );

        m_currentSettings->SetColor( layer, defaultColor );
        button->SetSwatchColor( defaultColor, false );
    }
}


bool PANEL_COLOR_SETTINGS::Show( bool show )
{
    if( show )
    {
        // In case changes have been made to the current theme in another panel:
        int themeSel = m_cbTheme->GetSelection();

        if( themeSel >= 0 )
        {
            COLOR_SETTINGS* settings = static_cast<COLOR_SETTINGS*>( m_cbTheme->GetClientData( themeSel ) );

            if( settings )
                *m_currentSettings = *settings;

            onNewThemeSelected();
            updateSwatches();
        }
    }

    return PANEL_COLOR_SETTINGS_BASE::Show( show );
}


void PANEL_COLOR_SETTINGS::OnLeftDownTheme( wxMouseEvent& event )
{
    // Lazy rebuild of theme menu to catch any colour theme changes made in other panels
    createThemeList( m_currentSettings->GetFilename() );

    event.Skip();
}


void PANEL_COLOR_SETTINGS::OnThemeChanged( wxCommandEvent& event )
{
    int idx = m_cbTheme->GetSelection();

    if( idx == static_cast<int>( m_cbTheme->GetCount() ) - 2 )
    {
        m_cbTheme->SetStringSelection( GetSettingsDropdownName( m_currentSettings ) );
        return;
    }

    if( idx == (int)m_cbTheme->GetCount() - 1 )
    {
        // New Theme...

        if( !saveCurrentTheme( false ) )
            return;

        FOOTPRINT_NAME_VALIDATOR themeNameValidator;
        wxTextEntryDialog dlg( wxGetTopLevelParent( this ), _( "New theme name:" ),
                               _( "Add Color Theme" ) );
        dlg.SetTextValidator( themeNameValidator );

        if( dlg.ShowModal() != wxID_OK )
            return;

        wxString themeName = dlg.GetValue();
        wxFileName fn( themeName + wxT( ".json" ) );
        fn.SetPath( SETTINGS_MANAGER::GetColorSettingsPath() );

        if( fn.Exists() )
        {
            wxMessageBox( _( "Theme already exists!" ) );
            return;
        }

        SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();
        COLOR_SETTINGS*   newSettings = settingsMgr.AddNewColorSettings( themeName );
        newSettings->SetName( themeName );
        newSettings->SetReadOnly( false );

        for( int layer : m_validLayers )
            newSettings->SetColor( layer, m_currentSettings->GetColor( layer ) );

        newSettings->SaveToFile( settingsMgr.GetPathForSettingsFile( newSettings ) );

        idx = m_cbTheme->Insert( themeName, idx - 1, static_cast<void*>( newSettings ) );
        m_cbTheme->SetSelection( idx );

        m_optOverrideColors->SetValue( newSettings->GetOverrideSchItemColors() );
        m_optOverrideColors->Enable( !newSettings->IsReadOnly() );

        *m_currentSettings = *newSettings;
        updateSwatches();
        onNewThemeSelected();
    }
    else
    {
        COLOR_SETTINGS* selected = static_cast<COLOR_SETTINGS*>( m_cbTheme->GetClientData( idx ) );

        if( selected->GetFilename() != m_currentSettings->GetFilename() )
        {
            if( !saveCurrentTheme( false ) )
                return;

            m_optOverrideColors->SetValue( selected->GetOverrideSchItemColors() );
            m_optOverrideColors->Enable( !selected->IsReadOnly() );

            *m_currentSettings = *selected;
            onNewThemeSelected();
            updateSwatches();
        }
    }
}


void PANEL_COLOR_SETTINGS::updateSwatches()
{
    if( m_swatches.empty() )
        createSwatches();

    bool    isReadOnly = m_currentSettings->IsReadOnly();
    COLOR4D background = m_currentSettings->GetColor( m_backgroundLayer );

    for( std::pair<int, COLOR_SWATCH*> pair : m_swatches )
    {
        pair.second->SetSwatchBackground( background );
        pair.second->SetSwatchColor( m_currentSettings->GetColor( pair.first ), false );
        pair.second->SetReadOnly( isReadOnly );
    }
}


void PANEL_COLOR_SETTINGS::createThemeList( const wxString& aCurrent )
{
    int width    = 0;
    int height   = 0;

    m_cbTheme->GetTextExtent( _( "New Theme..." ), &width, &height );
    int minwidth = width;

    m_cbTheme->Clear();

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        wxString name = GetSettingsDropdownName( settings );

        int pos = m_cbTheme->Append( name, static_cast<void*>( settings ) );

        if( settings->GetFilename() == aCurrent )
            m_cbTheme->SetSelection( pos );

        m_cbTheme->GetTextExtent( name, &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_cbTheme->Append( wxT( "---" ) );
    m_cbTheme->Append( _( "New Theme..." ) );

    m_cbTheme->SetMinSize( wxSize( minwidth + 50, -1 ) );
}


void PANEL_COLOR_SETTINGS::createSwatch( int aLayer, const wxString& aName )
{
    wxStaticText* label = new wxStaticText( m_colorsListWindow, wxID_ANY, aName );

    // The previously selected theme can be deleted and cannot be selected.
    // so select the default theme (first theme of the list)
    if( m_cbTheme->GetSelection() < 0 )
    {
        m_cbTheme->SetSelection( 0 );
        onNewThemeSelected();
    }

    void*           clientData = m_cbTheme->GetClientData( m_cbTheme->GetSelection() );
    COLOR_SETTINGS* selected = static_cast<COLOR_SETTINGS*>( clientData );

    int             id = FIRST_BUTTON_ID + aLayer;
    COLOR4D         defaultColor    = selected->GetDefaultColor( aLayer );
    COLOR4D         color           = m_currentSettings->GetColor( aLayer );
    COLOR4D         backgroundColor = m_currentSettings->GetColor( m_backgroundLayer );

    COLOR_SWATCH* swatch = new COLOR_SWATCH( m_colorsListWindow, color, id, backgroundColor,
                                             defaultColor, SWATCH_MEDIUM );
    swatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    // Display the swatches in the left column and the layer name in the right column
    // The right column is sometimes (depending on the translation) truncated for long texts.
    // We cannot allow swatches to be truncated or not shown
    m_colorsGridSizer->Add( swatch, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3 );
    m_colorsGridSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT, 5 );

    m_labels[aLayer]   = label;
    m_swatches[aLayer] = swatch;

    swatch->Bind( wxEVT_RIGHT_DOWN,
                  [&, aLayer]( wxMouseEvent& aEvent )
                  {
                      ShowColorContextMenu( aEvent, aLayer );
                  } );

    swatch->Bind( COLOR_SWATCH_CHANGED, &PANEL_COLOR_SETTINGS::OnColorChanged, this );
}


void PANEL_COLOR_SETTINGS::ShowColorContextMenu( wxMouseEvent& aEvent, int aLayer )
{
    int themeSel = m_cbTheme->GetSelection();

    if( themeSel >= 0 )
    {
        COLOR_SETTINGS* selected = static_cast<COLOR_SETTINGS*>( m_cbTheme->GetClientData( themeSel ) );

        wxCHECK_RET( selected, wxT( "Invalid color theme selected" ) );

        COLOR4D current  = m_currentSettings->GetColor( aLayer );
        COLOR4D saved    = selected->GetColor( aLayer );
        bool    readOnly = m_currentSettings->IsReadOnly();

        wxMenu menu;

        KIUI::AddMenuItem( &menu, ID_COPY, _( "Copy color" ), KiBitmap( BITMAPS::copy ) );

        if( !readOnly && m_copied != COLOR4D::UNSPECIFIED )
            KIUI::AddMenuItem( &menu, ID_PASTE, _( "Paste color" ), KiBitmap( BITMAPS::paste ) );

        if( !readOnly && current != saved )
            KIUI::AddMenuItem( &menu, ID_REVERT, _( "Revert to saved color" ), KiBitmap( BITMAPS::undo ) );

        menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
                [&]( wxCommandEvent& aCmd )
                {
                    switch( aCmd.GetId() )
                    {
                    case ID_COPY:
                        m_copied = current;
                        break;

                    case ID_PASTE:
                        updateColor( aLayer, m_copied );
                        break;

                    case ID_REVERT:
                        updateColor( aLayer, saved );
                        break;

                    default:
                        aCmd.Skip();
                    }
                } );

        PopupMenu( &menu );
    }
}


void PANEL_COLOR_SETTINGS::OnColorChanged( wxCommandEvent& aEvent )
{
    COLOR_SWATCH* swatch = static_cast<COLOR_SWATCH*>( aEvent.GetEventObject() );
    COLOR4D       newColor = swatch->GetSwatchColor();
    int           layer = static_cast<SCH_LAYER_ID>( swatch->GetId() - FIRST_BUTTON_ID );

    updateColor( layer, newColor );
}


void PANEL_COLOR_SETTINGS::updateColor( int aLayer, const KIGFX::COLOR4D& aColor )
{
    if( m_currentSettings )
        m_currentSettings->SetColor( aLayer, aColor );

    // Colors must be persisted when edited because multiple PANEL_COLOR_SETTINGS could be
    // referring to the same theme.
    saveCurrentTheme( false );

    m_swatches[aLayer]->SetSwatchColor( aColor, false );

    if( m_currentSettings && aLayer == m_backgroundLayer )
    {
        COLOR4D background = m_currentSettings->GetColor( m_backgroundLayer );

        for( std::pair<int, COLOR_SWATCH*> pair : m_swatches )
            pair.second->SetSwatchBackground( background );
    }

    onColorChanged();
}


bool PANEL_COLOR_SETTINGS::saveCurrentTheme( bool aValidate )
{
    if( m_currentSettings->IsReadOnly() )
        return true;

    if( aValidate && !validateSave() )
        return false;

    COLOR_SETTINGS* selected = ::GetColorSettings( m_currentSettings->GetFilename() );

    selected->SetOverrideSchItemColors( m_optOverrideColors->GetValue() );

    for( int layer : m_validLayers )
        selected->SetColor( layer, m_currentSettings->GetColor( layer ) );

    Pgm().GetSettingsManager().SaveColorSettings( selected, m_colorNamespace );

    return true;
}


wxString PANEL_COLOR_SETTINGS::GetSettingsDropdownName(COLOR_SETTINGS* aSettings)
{
    wxString name = aSettings->GetName();

    if( aSettings->IsReadOnly() )
        name += wxS( " " ) + _( "(read-only)" );

    return name;
}
