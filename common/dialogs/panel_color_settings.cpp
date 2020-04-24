/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <dialogs/dialog_color_picker.h>
#include <launch_ext.h>
#include <layers_id_colors_and_visibility.h>
#include <menus_helpers.h>
#include <panel_color_settings.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <validators.h>


// Width and height of every (color-displaying / bitmap) button in dialog units
const wxSize BUTTON_SIZE( 24, 12 );
const wxSize BUTTON_BORDER( 4, 4 );

// Button ID starting point
constexpr int FIRST_BUTTON_ID = 1800;


PANEL_COLOR_SETTINGS::PANEL_COLOR_SETTINGS( wxWindow* aParent ) :
        PANEL_COLOR_SETTINGS_BASE( aParent ),
        m_currentSettings( nullptr ),
        m_buttons(),
        m_copied( COLOR4D::UNSPECIFIED ),
        m_validLayers(),
        m_colorNamespace()
{
#ifdef __APPLE__
    m_btnOpenFolder->SetLabel( _( "Reveal Themes in Finder" ) );
#endif

    m_buttonSizePx = ConvertDialogToPixels( BUTTON_SIZE );
}


void PANEL_COLOR_SETTINGS::OnBtnOpenThemeFolderClicked( wxCommandEvent& event )
{
    wxString dir( SETTINGS_MANAGER::GetColorSettingsPath() );
    LaunchExternal( dir );
}


void PANEL_COLOR_SETTINGS::OnBtnResetClicked( wxCommandEvent& event )
{
    if( !m_currentSettings )
        return;

    for( const auto& pair : m_buttons )
    {
        int             layer  = pair.first;
        wxBitmapButton* button = pair.second;

        COLOR4D defaultColor = m_currentSettings->GetDefaultColor( layer );

        m_currentSettings->SetColor( layer, defaultColor );
        drawButton( button, defaultColor );
    }
}


void PANEL_COLOR_SETTINGS::OnThemeChanged( wxCommandEvent& event )
{
    int idx = m_cbTheme->GetSelection();

    if( idx == static_cast<int>( m_cbTheme->GetCount() ) - 2 )
    {
        // separator; re-select active theme
        m_cbTheme->SetStringSelection( m_currentSettings->GetName() );
        return;
    }

    if( idx == (int)m_cbTheme->GetCount() - 1 )
    {
        // New Theme...

        if( !saveCurrentTheme( false ) )
            return;

        MODULE_NAME_CHAR_VALIDATOR themeNameValidator;
        wxTextEntryDialog dlg( this, _( "New theme name:" ), _( "Add Color Theme" ) );
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

        for( auto layer : m_validLayers )
            newSettings->SetColor( layer, m_currentSettings->GetColor( layer ) );

        newSettings->SaveToFile( settingsMgr.GetPathForSettingsFile( newSettings ) );

        idx = m_cbTheme->Insert( themeName, idx - 1, static_cast<void*>( newSettings ) );
        m_cbTheme->SetSelection( idx );

        m_optOverrideColors->SetValue( newSettings->GetOverrideSchItemColors() );

        *m_currentSettings = *newSettings;
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

            *m_currentSettings = *selected;
            onNewThemeSelected();

            for( auto pair : m_buttons )
            {
                drawButton( pair.second, m_currentSettings->GetColor( pair.first ) );

                if( pair.first == LAYER_SHEET || pair.first == LAYER_SHEET_BACKGROUND )
                    pair.second->Show( selected->GetOverrideSchItemColors() );
            }
        }
    }
}


void PANEL_COLOR_SETTINGS::createThemeList( const COLOR_SETTINGS* aCurrent )
{
    m_cbTheme->Clear();

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        int pos = m_cbTheme->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings == aCurrent )
            m_cbTheme->SetSelection( pos );
    }

    m_cbTheme->Append( wxT( "---" ) );
    m_cbTheme->Append( _( "New Theme..." ) );
}


void PANEL_COLOR_SETTINGS::createButton( int aLayer, const KIGFX::COLOR4D& aColor,
                                         const wxString& aName )
{
    const int    flags  = wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxRIGHT;
    const wxSize border = ConvertDialogToPixels( BUTTON_BORDER );

    wxStaticText* label = new wxStaticText( m_colorsListWindow, wxID_ANY, aName );

    wxMemoryDC    iconDC;
    wxBitmap      bitmap( m_buttonSizePx );

    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush brush;
    brush.SetColour( aColor.ToColour() );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, m_buttonSizePx.x, m_buttonSizePx.y );

    int id = FIRST_BUTTON_ID + aLayer;

    auto button = new wxBitmapButton( m_colorsListWindow, id, bitmap, wxDefaultPosition,
                                      m_buttonSizePx + border + wxSize( 1, 1 ) );
    button->SetToolTip( _( "Edit color (right click for options)" ) );

    m_colorsGridSizer->Add( label, 0, flags, 5 );
    m_colorsGridSizer->Add( button, 0, flags, 5 );

    m_labels[aLayer]  = label;
    m_buttons[aLayer] = button;

    button->Bind( wxEVT_RIGHT_DOWN,
                  [&, aLayer]( wxMouseEvent& aEvent )
                  {
                    ShowColorContextMenu( aEvent, aLayer );
                  } );

    button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PANEL_COLOR_SETTINGS::SetColor, this );
}


void PANEL_COLOR_SETTINGS::ShowColorContextMenu( wxMouseEvent& aEvent, int aLayer )
{
    auto selected =
            static_cast<COLOR_SETTINGS*>( m_cbTheme->GetClientData( m_cbTheme->GetSelection() ) );

    COLOR4D current = m_currentSettings->GetColor( aLayer );
    COLOR4D saved   = selected->GetColor( aLayer );

    wxMenu menu;

    AddMenuItem( &menu, ID_COPY, _( "Copy color" ), KiBitmap( copy_xpm ) );

    if( m_copied != COLOR4D::UNSPECIFIED )
        AddMenuItem( &menu, ID_PASTE, _( "Paste color" ), KiBitmap( paste_xpm ) );

    if( current != saved )
        AddMenuItem( &menu, ID_REVERT, _( "Revert to saved color" ), KiBitmap( undo_xpm ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
            [&]( wxCommandEvent& aCmd ) {
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


void PANEL_COLOR_SETTINGS::SetColor( wxCommandEvent& event )
{
    auto button = static_cast<wxBitmapButton*>( event.GetEventObject() );
    auto layer  = static_cast<SCH_LAYER_ID>( button->GetId() - FIRST_BUTTON_ID );

    COLOR4D oldColor = m_currentSettings->GetColor( layer );
    COLOR4D newColor = COLOR4D::UNSPECIFIED;
    DIALOG_COLOR_PICKER dialog( this, oldColor, false );

    if( dialog.ShowModal() == wxID_OK )
        newColor = dialog.GetColor();

    if( newColor == COLOR4D::UNSPECIFIED || oldColor == newColor )
        return;

    updateColor( layer, newColor );
}


void PANEL_COLOR_SETTINGS::drawButton( wxBitmapButton* aButton, const COLOR4D& aColor ) const
{
    wxMemoryDC iconDC;

    wxBitmap bitmap = aButton->GetBitmapLabel();
    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush  brush;
    brush.SetColour( aColor.ToColour() );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, m_buttonSizePx.x, m_buttonSizePx.y );
    aButton->SetBitmapLabel( bitmap );
    aButton->Refresh();
}


void PANEL_COLOR_SETTINGS::updateColor( int aLayer, const KIGFX::COLOR4D& aColor )
{
    if( m_currentSettings )
        m_currentSettings->SetColor( aLayer, aColor );

    drawButton( m_buttons[aLayer], aColor );

    onColorChanged();
}


bool PANEL_COLOR_SETTINGS::saveCurrentTheme( bool aValidate )
{
    if( aValidate && !validateSave() )
        return false;

    SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();
    COLOR_SETTINGS* selected = settingsMgr.GetColorSettings( m_currentSettings->GetFilename() );

    selected->SetOverrideSchItemColors( m_optOverrideColors->GetValue() );

    for( auto layer : m_validLayers )
        selected->SetColor( layer, m_currentSettings->GetColor( layer ) );

    settingsMgr.SaveColorSettings( selected, m_colorNamespace );

    return true;
}
