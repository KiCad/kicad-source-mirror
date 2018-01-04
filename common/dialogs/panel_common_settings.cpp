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

#include <pgm_base.h>
#include <dialog_shim.h>
#include <kiface_i.h>
#include <bitmap_types.h>
#include <bitmaps.h>
#include <wx/graphics.h>
#include "panel_common_settings.h"

PANEL_COMMON_SETTINGS::PANEL_COMMON_SETTINGS( DIALOG_SHIM* aDialog, wxWindow* aParent ) :
        PANEL_COMMON_SETTINGS_BASE( aParent ),
        m_dialog( aDialog ),
        m_last_scale( -1 )
{
    m_scaleSlider->SetStep( 25 );

    m_textEditorBtn->SetBitmap( KiBitmap( folder_xpm ) );
    m_pdfViewerBtn->SetBitmap( KiBitmap( folder_xpm ) );
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

    int scale_fourths;
    commonSettings->Read( ICON_SCALE_KEY, &scale_fourths );

    if( scale_fourths <= 0 )
    {
        m_scaleAuto->SetValue( true );
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        m_scaleAuto->SetValue( false );
        m_scaleSlider->SetValue( scale_fourths * 25 );
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

    const int scale_fourths = m_scaleAuto->GetValue() ? -1 : m_scaleSlider->GetValue() / 25;
    commonSettings->Write( ICON_SCALE_KEY, scale_fourths );

    commonSettings->Write( USE_ICONS_IN_MENUS_KEY, m_checkBoxIconsInMenus->GetValue() );
    commonSettings->Write( ENBL_ZOOM_NO_CENTER_KEY, !m_ZoomCenterOpt->GetValue() );
    commonSettings->Write( ENBL_MOUSEWHEEL_PAN_KEY, m_MousewheelPANOpt->GetValue() );
    commonSettings->Write( ENBL_AUTO_PAN_KEY, m_AutoPANOpt->GetValue() );

    Pgm().SetEditorName( m_textEditorPath->GetValue() );

    Pgm().SetPdfBrowserName( m_PDFViewerPath->GetValue() );
    Pgm().ForceSystemPdfBrowser( m_defaultPDFViewer->GetValue() );
    Pgm().WritePdfBrowserInfos();

    return true;
}


void PANEL_COMMON_SETTINGS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_scaleAuto->SetValue( false );
}


void PANEL_COMMON_SETTINGS::OnScaleAuto( wxCommandEvent& aEvent )
{
    if( m_scaleAuto->GetValue() )
    {
        m_last_scale = m_scaleSlider->GetValue();
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_scaleSlider->SetValue( m_last_scale );
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

