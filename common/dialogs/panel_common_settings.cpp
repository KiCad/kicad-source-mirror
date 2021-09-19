/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/panel_common_settings.h>

#include <advanced_config.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <gal/dpi_scaling.h>
#include <kiface_base.h>
#include <pgm_base.h>
#include <id.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <widgets/stepped_slider.h>
#include <wx/filedlg.h>

/*
 * What follows is a whole lot of ugly to handle various platform GUI deficiences with respect
 * to light/dark mode, DPI scaling, and other foibles.
 *
 * Ugly as it all is, it does improve our usability on various platforms.
 */

PANEL_COMMON_SETTINGS::PANEL_COMMON_SETTINGS( DIALOG_SHIM* aDialog, wxWindow* aParent )
        : PANEL_COMMON_SETTINGS_BASE( aParent ),
          m_dialog( aDialog ),
          m_iconScaleLabel( nullptr ),
          m_iconScaleSlider( nullptr ),
          m_iconScaleAuto( nullptr ),
          m_last_scale( -1 )
{
    /*
     * Cairo canvas doesn't work on Mac, so no need for fallback anti-aliasing options
     */
#ifdef __WXMAC__
    m_antialiasingFallback->Show( false );
    m_antialiasingFallbackLabel->Show( false );
#endif

    m_textEditorBtn->SetBitmap( KiBitmap( BITMAPS::small_folder ) );
    m_pdfViewerBtn->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    /*
     * Automatic dark mode detection works fine on Mac, so no need for the explicit options.
     */
#ifdef __WXMAC__
    m_stIconTheme->Show( false );
    m_rbIconThemeLight->Show( false );
    m_rbIconThemeDark->Show( false );
    m_rbIconThemeAuto->Show( false );
#endif

    /*
     * Automatic icon scaling works fine on Mac.  It works mostly fine on MSW, but perhaps not
     * uniformly enough to exclude the explicit controls there.
     */
#if defined( __WXGTK__ ) || defined( __WXMSW__ )
    // Sadly wxSlider is poorly implemented and adds its legends as sibiling windows (so that
    // showing/hiding the control doesn't work).  So we have to create it conditionally.
    wxWindow*       parent = m_sbUserInterface->GetStaticBox();
    wxGridBagSizer* gb = m_gbUserInterface;

    m_iconScaleLabel = new wxStaticText( parent, wxID_ANY, _( "Icon scale:" ) );
   	m_iconScaleLabel->Wrap( -1 );
   	gb->Add( m_iconScaleLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

   	m_iconScaleSlider = new STEPPED_SLIDER( parent, wxID_ANY, 100, 50, 275, wxDefaultPosition,
                                            wxDefaultSize, wxSL_HORIZONTAL|wxSL_VALUE_LABEL );
   	m_iconScaleSlider->SetStep( 25 );
   	gb->Add( m_iconScaleSlider, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxEXPAND|wxBOTTOM, 5 );

   	m_iconScaleAuto = new wxCheckBox( parent, wxID_ANY, _( "Automatic" ) );
   	gb->Add( m_iconScaleAuto, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );
#endif

   	/*
   	 * Automatic canvas scaling works fine on Mac and MSW, and on GTK under wxWidgets 3.1 or
   	 * better.
   	 */
#if defined( __WXGTK__ ) && !wxCHECK_VERSION( 3, 1, 0 )
   	static constexpr int dpi_scaling_precision = 1;
   	static constexpr double dpi_scaling_increment = 0.5;

    m_canvasScaleCtrl->SetRange( DPI_SCALING::GetMinScaleFactor(),
                                 DPI_SCALING::GetMaxScaleFactor() );
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
#else
    m_staticTextCanvasScale->Show( false );
    m_canvasScaleCtrl->Show( false );
    m_canvasScaleCtrl = nullptr;
    m_canvasScaleAuto->Show( false );
#endif

    /*
     * Font scaling hacks are only needed on GTK under wxWidgets 3.0.
     */
#if defined( __WXGTK__ ) && !wxCHECK_VERSION( 3, 1, 0 )
    m_fontScalingHelp->SetFont( KIUI::GetInfoFont( this ).Italic() );
#else
    m_scaleFonts->Show( false );
    m_fontScalingHelp->Show( false );
#endif

    if( m_iconScaleSlider )
    {
        m_iconScaleSlider->Connect( wxEVT_SCROLL_TOP, 
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_BOTTOM, 
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_LINEUP, 
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_LINEDOWN,
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_PAGEUP,
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN,
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, 
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, 
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleSlider->Connect( wxEVT_SCROLL_CHANGED,
                                    wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ), 
                                    nullptr, this );
        m_iconScaleAuto->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED,
                                  wxCommandEventHandler( PANEL_COMMON_SETTINGS::OnIconScaleAuto ),
                                  nullptr, this );
    }

    if( m_canvasScaleCtrl )
    {
        m_canvasScaleCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED,
                                    wxCommandEventHandler( PANEL_COMMON_SETTINGS::OnCanvasScaleChange ),
                                    nullptr, this );
    }
}


PANEL_COMMON_SETTINGS::~PANEL_COMMON_SETTINGS()
{
    if( m_iconScaleSlider )
    {
        m_iconScaleSlider->Disconnect( wxEVT_SCROLL_TOP,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_LINEUP,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleSlider->Disconnect( wxEVT_SCROLL_CHANGED,
                                       wxScrollEventHandler( PANEL_COMMON_SETTINGS::OnScaleSlider ),
                                       nullptr, this );
       	m_iconScaleAuto->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED,
                                     wxCommandEventHandler( PANEL_COMMON_SETTINGS::OnIconScaleAuto ),
                                     nullptr, this );
    }

    if( m_canvasScaleCtrl )
    {
        m_canvasScaleCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED,
                                       wxCommandEventHandler( PANEL_COMMON_SETTINGS::OnCanvasScaleChange ),
                                       nullptr, this );
    }
}


bool PANEL_COMMON_SETTINGS::TransferDataToWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    applySettingsToPanel( *commonSettings );

    // TODO(JE) Move these into COMMON_SETTINGS probably
    m_textEditorPath->SetValue( Pgm().GetTextEditor( false ) );
    m_defaultPDFViewer->SetValue( Pgm().UseSystemPdfBrowser() );
    m_otherPDFViewer->SetValue( !Pgm().UseSystemPdfBrowser() );
    m_PDFViewerPath->SetValue( Pgm().GetPdfBrowserName() );

    return true;
}


bool PANEL_COMMON_SETTINGS::TransferDataFromWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    commonSettings->m_System.autosave_interval = m_SaveTime->GetValue() * 60;
    commonSettings->m_System.file_history_size = m_fileHistorySize->GetValue();
    commonSettings->m_System.clear_3d_cache_interval = m_Clear3DCacheFilesOlder->GetValue();

    commonSettings->m_Graphics.opengl_aa_mode = m_antialiasing->GetSelection();
    commonSettings->m_Graphics.cairo_aa_mode = m_antialiasingFallback->GetSelection();

    if( m_iconScaleSlider )
    {
        int scale_fourths = m_iconScaleAuto->GetValue() ? -1 : m_iconScaleSlider->GetValue() / 25;
        commonSettings->m_Appearance.icon_scale = scale_fourths;
    }

    if( m_canvasScaleCtrl )
    {
        DPI_SCALING dpi( commonSettings, this );
        dpi.SetDpiConfig( m_canvasScaleAuto->GetValue(), m_canvasScaleCtrl->GetValue() );
    }

    if( m_rbIconThemeLight->GetValue() )
        commonSettings->m_Appearance.icon_theme = ICON_THEME::LIGHT;
    else if( m_rbIconThemeDark->GetValue() )
        commonSettings->m_Appearance.icon_theme = ICON_THEME::DARK;
    else if( m_rbIconThemeAuto->GetValue() )
        commonSettings->m_Appearance.icon_theme = ICON_THEME::AUTO;

    commonSettings->m_Appearance.use_icons_in_menus = m_checkBoxIconsInMenus->GetValue();
    commonSettings->m_Appearance.apply_icon_scale_to_fonts = m_scaleFonts->GetValue();

    commonSettings->m_Input.immediate_actions   = !m_NonImmediateActions->GetValue();
    commonSettings->m_Input.warp_mouse_on_move  = m_warpMouseOnMove->GetValue();

    commonSettings->m_Backup.enabled            = m_cbBackupEnabled->GetValue();
    commonSettings->m_Backup.backup_on_autosave = m_cbBackupAutosave->GetValue();
    commonSettings->m_Backup.limit_total_files  = m_backupLimitTotalFiles->GetValue();
    commonSettings->m_Backup.limit_daily_files  = m_backupLimitDailyFiles->GetValue();
    commonSettings->m_Backup.min_interval       = m_backupMinInterval->GetValue() * 60;
    commonSettings->m_Backup.limit_total_size   = m_backupLimitTotalSize->GetValue() * 1024 * 1024;

    commonSettings->m_Session.remember_open_files = m_cbRememberOpenFiles->GetValue();

    Pgm().SetTextEditor( m_textEditorPath->GetValue());

    Pgm().SetPdfBrowserName( m_PDFViewerPath->GetValue() );
    Pgm().ForceSystemPdfBrowser( m_defaultPDFViewer->GetValue() );
    Pgm().WritePdfBrowserInfos();

    Pgm().GetSettingsManager().Save( commonSettings );

    return true;
}


void PANEL_COMMON_SETTINGS::ResetPanel()
{
    COMMON_SETTINGS defaultSettings;

    defaultSettings.ResetToDefaults();

    applySettingsToPanel( defaultSettings );

    // TODO(JE) Move these into COMMON_SETTINGS probably
    m_textEditorPath->SetValue( defaultSettings.m_System.text_editor );
    m_defaultPDFViewer->SetValue( defaultSettings.m_System.use_system_pdf_viewer );
    m_otherPDFViewer->SetValue( !defaultSettings.m_System.use_system_pdf_viewer );
    m_PDFViewerPath->SetValue( defaultSettings.m_System.pdf_viewer_name );
}


void PANEL_COMMON_SETTINGS::applySettingsToPanel( COMMON_SETTINGS& aSettings )
{
    int timevalue = aSettings.m_System.autosave_interval;
    wxString msg;

    msg << timevalue / 60;
    m_SaveTime->SetValue( msg );

    m_fileHistorySize->SetValue( aSettings.m_System.file_history_size );

    m_antialiasing->SetSelection( aSettings.m_Graphics.opengl_aa_mode );
    m_antialiasingFallback->SetSelection( aSettings.m_Graphics.cairo_aa_mode );

    m_Clear3DCacheFilesOlder->SetValue( aSettings.m_System.clear_3d_cache_interval );

    if( m_iconScaleSlider )
    {
        int icon_scale_fourths = aSettings.m_Appearance.icon_scale;

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
    }

    if( m_canvasScaleCtrl )
    {
        const DPI_SCALING dpi( &aSettings, this );
        m_canvasScaleCtrl->SetValue( dpi.GetScaleFactor() );
        m_canvasScaleAuto->SetValue( dpi.GetCanvasIsAutoScaled() );
    }

    switch( aSettings.m_Appearance.icon_theme )
    {
    case ICON_THEME::LIGHT: m_rbIconThemeLight->SetValue( true );   break;
    case ICON_THEME::DARK:  m_rbIconThemeDark->SetValue( true );    break;
    case ICON_THEME::AUTO:  m_rbIconThemeAuto->SetValue( true );    break;
    }

    m_checkBoxIconsInMenus->SetValue( aSettings.m_Appearance.use_icons_in_menus );
    m_scaleFonts->SetValue( aSettings.m_Appearance.apply_icon_scale_to_fonts );

    m_warpMouseOnMove->SetValue( aSettings.m_Input.warp_mouse_on_move );
    m_NonImmediateActions->SetValue( !aSettings.m_Input.immediate_actions );

    m_cbRememberOpenFiles->SetValue( aSettings.m_Session.remember_open_files );

    m_cbBackupEnabled->SetValue( aSettings.m_Backup.enabled );
    m_cbBackupAutosave->SetValue( aSettings.m_Backup.backup_on_autosave );
    m_backupLimitTotalFiles->SetValue( aSettings.m_Backup.limit_total_files );
    m_backupLimitDailyFiles->SetValue( aSettings.m_Backup.limit_daily_files );
    m_backupMinInterval->SetValue( aSettings.m_Backup.min_interval / 60 );
    m_backupLimitTotalSize->SetValue( aSettings.m_Backup.limit_total_size / ( 1024 * 1024 ) );
}


void PANEL_COMMON_SETTINGS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_iconScaleAuto->SetValue( false );
    aEvent.Skip();
}


void PANEL_COMMON_SETTINGS::OnIconScaleAuto( wxCommandEvent& aEvent )
{
    if( m_iconScaleSlider )
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
}


void PANEL_COMMON_SETTINGS::OnCanvasScaleChange( wxCommandEvent& aEvent )
{
    m_canvasScaleAuto->SetValue( false );
}


void PANEL_COMMON_SETTINGS::OnCanvasScaleAuto( wxCommandEvent& aEvent )
{
    const bool automatic = m_canvasScaleAuto->GetValue();

    if( automatic && m_canvasScaleCtrl )
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

    // If we have a new editor name request it to be copied to m_text_editor and saved
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

    wxFileDialog dlg( this, _( "Select Preferred PDF Viewer" ), fn.GetPath(), fn.GetFullPath(),
                      wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_otherPDFViewer->SetValue( true );
    m_PDFViewerPath->SetValue( dlg.GetPath() );
}


void PANEL_COMMON_SETTINGS::onUpdateUIPdfPath( wxUpdateUIEvent& event )
{
    // Used by both the m_pdfViewerBtn and m_PDFViewerPath
    event.Enable( m_otherPDFViewer->GetValue() );
}
