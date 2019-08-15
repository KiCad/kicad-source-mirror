/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_common_settings.h"

#include <bitmap_types.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <dpi_scaling.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <id.h>

static constexpr int dpi_scaling_precision = 1;
static constexpr double dpi_scaling_increment = 0.5;


PANEL_COMMON_SETTINGS::PANEL_COMMON_SETTINGS( DIALOG_SHIM* aDialog, wxWindow* aParent )
        : PANEL_COMMON_SETTINGS_BASE( aParent ),
          m_dialog( aDialog ),
          m_last_scale( -1 )
{
    m_canvasScaleCtrl->SetRange(
            DPI_SCALING::GetMinScaleFactor(), DPI_SCALING::GetMaxScaleFactor() );
    m_canvasScaleCtrl->SetDigits( dpi_scaling_precision );
    m_canvasScaleCtrl->SetIncrement( dpi_scaling_increment );
    m_canvasScaleCtrl->SetValue( DPI_SCALING::GetDefaultScaleFactor() );

    m_canvasScaleCtrl->SetToolTip(
            _( "Set the scale for the canvas."
               "\n\n"
               "On high-DPI displays on some platforms, KiCad cannot determine the "
               "scaling factor. In this case you may need to set this to a value to "
               "match your system's DPI scaling. 2.0 is a common value. "
               "\n\n"
               "If this does not match the system DPI scaling, the canvas will "
               "not match the window size and cursor position." ) );

    m_canvasScaleAuto->SetToolTip(
            _( "Use an automatic value for the canvas scale."
               "\n\n"
               "On some platforms, the automatic value is incorrect and should be "
               "set manually." ) );

    m_iconScaleSlider->SetStep( 25 );

    m_textEditorBtn->SetBitmap( KiBitmap( folder_xpm ) );
    m_pdfViewerBtn->SetBitmap( KiBitmap( folder_xpm ) );

    m_canvasScaleCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COMMON_SETTINGS::OnCanvasScaleChange ), NULL, this );
}


PANEL_COMMON_SETTINGS::~PANEL_COMMON_SETTINGS()
{
    m_canvasScaleCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COMMON_SETTINGS::OnCanvasScaleChange ), NULL, this );
}


bool PANEL_COMMON_SETTINGS::TransferDataToWindow()
{
    wxConfigBase* commonSettings = Pgm().CommonSettings();

    int timevalue;
    wxString msg;

    commonSettings->Read( AUTOSAVE_INTERVAL_KEY, &timevalue );
    msg << timevalue / 60;
    m_SaveTime->SetValue( msg );

    int fileHistorySize;
    commonSettings->Read( FILE_HISTORY_SIZE_KEY, &fileHistorySize, DEFAULT_FILE_HISTORY_SIZE );
    m_fileHistorySize->SetValue( fileHistorySize );

    int antialiasingMode;
    commonSettings->Read( GAL_ANTIALIASING_MODE_KEY, &antialiasingMode, 0 );
    m_antialiasing->SetSelection( antialiasingMode );

    commonSettings->Read( CAIRO_ANTIALIASING_MODE_KEY, &antialiasingMode, 0 );
    m_antialiasingFallback->SetSelection( antialiasingMode );

    int icon_scale_fourths;
    commonSettings->Read( ICON_SCALE_KEY, &icon_scale_fourths );

    if( icon_scale_fourths <= 0 )
    {
        m_iconScaleAuto->SetValue( true );
        m_iconScaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        m_iconScaleAuto->SetValue( false );
        m_iconScaleSlider->SetValue( icon_scale_fourths * 25 );
    }

    {
        const DPI_SCALING dpi( commonSettings, this );
        m_canvasScaleCtrl->SetValue( dpi.GetScaleFactor() );
        m_canvasScaleAuto->SetValue( dpi.GetCanvasIsAutoScaled() );
    }

    bool option;
    commonSettings->Read( USE_ICONS_IN_MENUS_KEY, &option );
    m_checkBoxIconsInMenus->SetValue( option );

    commonSettings->Read( ENBL_ZOOM_NO_CENTER_KEY, &option );
    m_ZoomCenterOpt->SetValue( !option );

    commonSettings->Read( ENBL_MOUSEWHEEL_PAN_KEY, &option );
    m_MousewheelPANOpt->SetValue( option );

    commonSettings->Read( ENBL_AUTO_PAN_KEY, &option );
    m_AutoPANOpt->SetValue( option );

    if( !commonSettings->Read( PREFER_SELECT_TO_DRAG_KEY, &option ) )
    {
        // Legacy versions stored the property only for PCBNew, so see if we have it there
        std::unique_ptr<wxConfigBase> pcbSettings = GetNewConfig( wxT( "pcbnew" ) );
        pcbSettings->Read( "DragSelects", &option, true );
    }
    m_PreferSelectToDrag->SetValue( option );

    commonSettings->Read( WARP_MOUSE_ON_MOVE_KEY, &option );
    m_warpMouseOnMove->SetValue( option );

    commonSettings->Read( IMMEDIATE_ACTIONS_KEY, &option );
    m_NonImmediateActions->SetValue( !option );

    m_textEditorPath->SetValue( Pgm().GetEditorName( false ) );
    m_defaultPDFViewer->SetValue( Pgm().UseSystemPdfBrowser() );
    m_otherPDFViewer->SetValue( !Pgm().UseSystemPdfBrowser() );
    m_PDFViewerPath->SetValue( Pgm().GetPdfBrowserName() );

    return true;
}


bool PANEL_COMMON_SETTINGS::TransferDataFromWindow()
{
    wxConfigBase* commonSettings = Pgm().CommonSettings();

    commonSettings->Write( AUTOSAVE_INTERVAL_KEY, m_SaveTime->GetValue() * 60 );

    commonSettings->Write( FILE_HISTORY_SIZE_KEY, m_fileHistorySize->GetValue() );

    commonSettings->Write( GAL_ANTIALIASING_MODE_KEY, m_antialiasing->GetSelection() );

    commonSettings->Write( CAIRO_ANTIALIASING_MODE_KEY, m_antialiasingFallback->GetSelection() );

    const int scale_fourths = m_iconScaleAuto->GetValue() ? -1 : m_iconScaleSlider->GetValue() / 25;
    commonSettings->Write( ICON_SCALE_KEY, scale_fourths );

    {
        DPI_SCALING dpi( commonSettings, this );
        dpi.SetDpiConfig( m_canvasScaleAuto->GetValue(), m_canvasScaleCtrl->GetValue() );
    }

    commonSettings->Write( USE_ICONS_IN_MENUS_KEY, m_checkBoxIconsInMenus->GetValue() );
    commonSettings->Write( ENBL_ZOOM_NO_CENTER_KEY, !m_ZoomCenterOpt->GetValue() );
    commonSettings->Write( ENBL_MOUSEWHEEL_PAN_KEY, m_MousewheelPANOpt->GetValue() );
    commonSettings->Write( ENBL_AUTO_PAN_KEY, m_AutoPANOpt->GetValue() );
    commonSettings->Write( PREFER_SELECT_TO_DRAG_KEY, m_PreferSelectToDrag->GetValue() );
    commonSettings->Write( WARP_MOUSE_ON_MOVE_KEY, m_warpMouseOnMove->GetValue() );
    commonSettings->Write( IMMEDIATE_ACTIONS_KEY, !m_NonImmediateActions->GetValue() );

    Pgm().SetEditorName( m_textEditorPath->GetValue() );

    Pgm().SetPdfBrowserName( m_PDFViewerPath->GetValue() );
    Pgm().ForceSystemPdfBrowser( m_defaultPDFViewer->GetValue() );
    Pgm().WritePdfBrowserInfos();

    return true;
}


void PANEL_COMMON_SETTINGS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_iconScaleAuto->SetValue( false );
}


void PANEL_COMMON_SETTINGS::OnIconScaleAuto( wxCommandEvent& aEvent )
{
    if( m_iconScaleAuto->GetValue() )
    {
        m_last_scale = m_iconScaleAuto->GetValue();
        m_iconScaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_iconScaleSlider->SetValue( m_last_scale );
    }
}


void PANEL_COMMON_SETTINGS::OnCanvasScaleChange( wxCommandEvent& aEvent )
{
    m_canvasScaleAuto->SetValue( false );
}


void PANEL_COMMON_SETTINGS::OnCanvasScaleAuto( wxCommandEvent& aEvent )
{
    const bool automatic = m_canvasScaleAuto->GetValue();

    if( automatic )
    {
        // set the scale to the auto value, without consulting the config
        DPI_SCALING dpi( nullptr, this );

        // update the field (no events sent)
        m_canvasScaleCtrl->SetValue( dpi.GetScaleFactor() );
    }
}


void PANEL_COMMON_SETTINGS::OnTextEditorClick( wxCommandEvent& event )
{
    // Ask the user to select a new editor, but suggest the current one as the default.
    wxString editorname = Pgm().AskUserForPreferredEditor( m_textEditorPath->GetValue() );

    // If we have a new editor name request it to be copied to m_editor_name and saved
    // to the preferences file. If the user cancelled the dialog then the previous
    // value will be retained.
    if( !editorname.IsEmpty() )
        m_textEditorPath->SetValue( editorname );
}


void PANEL_COMMON_SETTINGS::OnPDFViewerClick( wxCommandEvent& event )
{
    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".exe" );
#endif

    wxString wildcard = _( "Executable files (" ) + mask + wxT( ")|" ) + mask;

    Pgm().ReadPdfBrowserInfos();
    wxFileName fn = Pgm().GetPdfBrowserName();

    wxFileDialog dlg( this, _( "Select Preferred PDF Browser" ), fn.GetPath(), fn.GetFullPath(),
                      wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_otherPDFViewer->SetValue( true );
    m_PDFViewerPath->SetValue( dlg.GetPath() );
}


void PANEL_COMMON_SETTINGS::onUpdateUIPdfPath( wxUpdateUIEvent& event )
{
    bool enabled = m_otherPDFViewer->GetValue();
    m_PDFViewerPath->Enable( enabled );
    m_pdfViewerBtn->Enable( enabled );
}

