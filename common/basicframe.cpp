/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file basicframe.cpp
 * @brief EDA_BASE_FRAME class implementation.
 */

#include <wx/aboutdlg.h>
#include <wx/fontdlg.h>
#include <wx/clipbrd.h>
#include <wx/statline.h>
#include <wx/platinfo.h>

#include "build_version.h"
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "online_help.h"
#include "id.h"
#include "confirm.h"
#include "eda_doc.h"
#include "wxstruct.h"
#include "macros.h"


/// The default auto save interval is 10 minutes.
#define DEFAULT_AUTO_SAVE_INTERVAL 600


static const wxChar* entryAutoSaveInterval = wxT( "AutoSaveInterval" );


EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* father,
                                int idtype,
                                const wxString& title,
                                const wxPoint& pos,
                                const wxSize& size,
                                long style ) :
    wxFrame( father, -1, title, pos, size, style )
{
    wxSize minsize;

    m_Ident = idtype;
    m_HToolBar = NULL;
    m_FrameIsActive = true;
    m_hasAutoSave = false;
    m_autoSaveState = false;
    m_autoSaveInterval = -1;
    m_autoSaveTimer = new wxTimer( this, ID_AUTO_SAVE_TIMER );

    m_MsgFrameHeight = EDA_MSG_PANEL::GetRequiredHeight();

    minsize.x = 470;
    minsize.y = 350 + m_MsgFrameHeight;

    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    if( ( size.x < minsize.x ) || ( size.y < minsize.y ) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y ); /* dimensions of the user area of the main
                                                      * window */
    m_FramePos.x = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    Connect( ID_HELP_COPY_VERSION_STRING,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler( EDA_BASE_FRAME::CopyVersionInfoToClipboard ) );

    Connect( ID_AUTO_SAVE_TIMER, wxEVT_TIMER,
             wxTimerEventHandler( EDA_BASE_FRAME::onAutoSaveTimer ) );
}


EDA_BASE_FRAME::~EDA_BASE_FRAME()
{
    if( wxGetApp().m_HtmlCtrl )
        delete wxGetApp().m_HtmlCtrl;

    wxGetApp().m_HtmlCtrl = NULL;

    delete m_autoSaveTimer;

    /* This needed for OSX: avoids further OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    this->Freeze();
}


bool EDA_BASE_FRAME::ProcessEvent( wxEvent& aEvent )
{
    if( !wxFrame::ProcessEvent( aEvent ) )
        return false;

    if( m_hasAutoSave && (m_autoSaveState != isModified()) && (m_autoSaveInterval > 0) )
    {
        if( !m_autoSaveState )
        {
            m_autoSaveTimer->Start( m_autoSaveInterval * 1000, wxTIMER_ONE_SHOT );
            m_autoSaveState = true;
        }
        else
        {
            m_autoSaveTimer->Stop();
            m_autoSaveState = false;
        }
    }

    return true;
}


void EDA_BASE_FRAME::onAutoSaveTimer( wxTimerEvent& aEvent )
{
    if( !doAutoSave() )
        m_autoSaveTimer->Start( m_autoSaveInterval * 1000, wxTIMER_ONE_SHOT );
}


bool EDA_BASE_FRAME::doAutoSave()
{
    wxCHECK_MSG( false, true, wxT( "Auto save timer function not overridden.  Bad programmer!" ) );
}


void EDA_BASE_FRAME::ReCreateMenuBar()
{
}


void EDA_BASE_FRAME::SetLanguage( wxCommandEvent& event )
{
    int id = event.GetId();

    wxGetApp().SetLanguageIdentifier( id );
    wxGetApp().SetLanguage();
    ReCreateMenuBar();
    GetMenuBar()->Refresh();
}


void EDA_BASE_FRAME::LoadSettings()
{
    wxString  text;
    int       Ypos_min;
    wxConfig* config;

    config = wxGetApp().m_EDA_Config;

    int maximized = 0;

    if( config )
    {
        text = m_FrameName + wxT( "Pos_x" );
        config->Read( text, &m_FramePos.x );
        text = m_FrameName + wxT( "Pos_y" );
        config->Read( text, &m_FramePos.y );
        text = m_FrameName + wxT( "Size_x" );
        config->Read( text, &m_FrameSize.x, 600 );
        text = m_FrameName + wxT( "Size_y" );
        config->Read( text, &m_FrameSize.y, 400 );
        text = m_FrameName + wxT( "Maximized" );
        config->Read( text, &maximized, 0 );

        if( m_hasAutoSave )
        {
            text = m_FrameName + entryAutoSaveInterval;
            config->Read( text, &m_autoSaveInterval, DEFAULT_AUTO_SAVE_INTERVAL );
        }
    }

    // Ensure Window title bar is visible
#if defined( __WXMAC__ )
    // for macOSX, the window must be below system (macOSX) toolbar
//    Ypos_min = GetMBarHeight(); seems no more exist in new API (subject to change)
    Ypos_min = 20;
#else
    Ypos_min = 0;
#endif
    if( m_FramePos.y < Ypos_min )
        m_FramePos.y = Ypos_min;

    if( maximized )
        Maximize();
}


void EDA_BASE_FRAME::SaveSettings()
{
    wxString text;
    wxConfig* config;

    config = wxGetApp().m_EDA_Config;

    if( ( config == NULL ) || IsIconized() )
        return;

    m_FrameSize = GetSize();
    m_FramePos  = GetPosition();

    text = m_FrameName + wxT( "Pos_x" );
    config->Write( text, (long) m_FramePos.x );
    text = m_FrameName + wxT( "Pos_y" );
    config->Write( text, (long) m_FramePos.y );
    text = m_FrameName + wxT( "Size_x" );
    config->Write( text, (long) m_FrameSize.x );
    text = m_FrameName + wxT( "Size_y" );
    config->Write( text, (long) m_FrameSize.y );
    text = m_FrameName + wxT( "Maximized" );
    config->Write( text, IsMaximized() );

    if( m_hasAutoSave )
    {
        text = m_FrameName + entryAutoSaveInterval;
        config->Write( text, m_autoSaveInterval );
    }
}


void EDA_BASE_FRAME::PrintMsg( const wxString& text )
{
    SetStatusText( text );
}


void EDA_BASE_FRAME::DisplayActivity( int PerCent, const wxString& Text )
{
    wxString Line;

    Line = Text;

    PerCent  = (PerCent < 0) ? 0 : PerCent;
    PerCent  = (PerCent > 100) ? 100 : PerCent;
    PerCent /= 2;   // Bargraph is 0 .. 50 points from 0% to 100%

    if( PerCent )
        Line.Pad( PerCent, '*' );

    SetStatusText( Line );
}


void EDA_BASE_FRAME::UpdateFileHistory( const wxString& FullFileName,
                                        wxFileHistory * aFileHistory )
{
    wxFileHistory * fileHistory = aFileHistory;

    if( fileHistory == NULL )
        fileHistory = & wxGetApp().m_fileHistory;

    fileHistory->AddFileToHistory( FullFileName );
}


wxString EDA_BASE_FRAME::GetFileFromHistory( int cmdId, const wxString& type,
                                             wxFileHistory * aFileHistory )
{
    wxString fn, msg;
    size_t   i;
    wxFileHistory * fileHistory = aFileHistory;

    if( fileHistory == NULL )
        fileHistory = & wxGetApp().m_fileHistory;

    int baseId = fileHistory->GetBaseId();

    wxASSERT( cmdId >= baseId && cmdId < baseId + ( int )fileHistory->GetCount() );

    i = ( size_t )( cmdId - baseId );

    if( i < fileHistory->GetCount() )
    {
        fn = fileHistory->GetHistoryFile( i );

        if( !wxFileName::FileExists( fn ) )
        {
            msg = type + _( " file <" ) + fn + _( "> was not found." );
            DisplayError( this, msg );
            fileHistory->RemoveFileFromHistory( i );
            fn = wxEmptyString;
        }
    }

    return fn;
}


void EDA_BASE_FRAME::GetKicadHelp( wxCommandEvent& event )
{
    wxString msg;

    /* We have to get document for beginners,
     * or the the full specific doc
     * if event id is wxID_INDEX, we want the document for beginners.
     * else the specific doc file (its name is in wxGetApp().m_HelpFileName)
     * The document for beginners is the same for all KiCad utilities
     */
    if( event.GetId() == wxID_INDEX )
    {
        // Temporary change the help filename
        wxString tmp = wxGetApp().m_HelpFileName;
        wxGetApp().m_HelpFileName = wxT( "Getting_Started_in_KiCad.pdf" );
        wxString helpFile = wxGetApp().GetHelpFile();

        if( !helpFile )
        {
            msg.Printf( _( "Help file %s could not be found." ),
                        GetChars( wxGetApp().m_HelpFileName ) );
            DisplayError( this, msg );
        }
        else
        {
            GetAssociatedDocument( this, helpFile );
        }

        wxGetApp().m_HelpFileName = tmp;
        return;
    }

#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML

    if( wxGetApp().m_HtmlCtrl == NULL )
    {
        wxGetApp().InitOnLineHelp();
    }


    if( wxGetApp().m_HtmlCtrl )
    {
        wxGetApp().m_HtmlCtrl->DisplayContents();
        wxGetApp().m_HtmlCtrl->Display( wxGetApp().m_HelpFileName );
    }
    else
    {
        msg.Printf( _( "Help file %s not found." ), GetChars( wxGetApp().m_HelpFileName ) );
        DisplayError( this, msg );
    }

#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    wxString helpFile = wxGetApp().GetHelpFile();

    if( !helpFile )
    {
        msg.Printf( _( "Help file %s could not be found." ),
                    GetChars( wxGetApp().m_HelpFileName ) );
        DisplayError( this, msg );
    }
    else
    {
        GetAssociatedDocument( this, helpFile );
    }

#else
#   error Help files format not defined
#endif
}


void EDA_BASE_FRAME::OnSelectPreferredEditor( wxCommandEvent& event )
{
    wxFileName fn = wxGetApp().m_EditorName;
    wxString wildcard( wxT( "*" ) );

#ifdef __WINDOWS__
    wildcard += wxT( ".exe" );
#endif

    wildcard = _( "Executable file (" ) + wildcard + wxT( ")|" ) + wildcard;

    wxFileDialog dlg( this, _( "Select Preferred Editor" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxASSERT( wxGetApp().m_EDA_CommonConfig );

    wxConfig* cfg = wxGetApp().m_EDA_CommonConfig;
    wxGetApp().m_EditorName = dlg.GetPath();
    cfg->Write( wxT( "Editor" ), wxGetApp().m_EditorName );
}


void EDA_BASE_FRAME::GetKicadAbout( wxCommandEvent& event )
{
    bool ShowAboutDialog(wxWindow * parent);
    ShowAboutDialog(this);
}


void EDA_BASE_FRAME::AddHelpVersionInfoMenuEntry( wxMenu* aMenu )
{
    wxASSERT( aMenu != NULL );

    // Copy version string to clipboard for bug report purposes.
    AddMenuItem( aMenu, ID_HELP_COPY_VERSION_STRING,
                 _( "Copy &Version Information" ),
                 _( "Copy the version string to clipboard to send with bug reports" ),
                 KiBitmap( copy_button_xpm ) );
}


// This is an enhanced version of the compiler build macro provided by wxWidgets
// in <wx/build.h>. Please do not make any of these strings translatable.  They
// are used for conveying troubleshooting information to developers.

#if defined(__GXX_ABI_VERSION)
    #define __ABI_VERSION  ",compiler with C++ ABI " __WX_BO_STRINGIZE(__GXX_ABI_VERSION)
#else
    #define __ABI_VERSION  ",compiler without C++ ABI "
#endif

#if defined(__INTEL_COMPILER)
    #define __BO_COMPILER ",Intel C++"
#elif defined(__GNUG__)
    #define __BO_COMPILER ",GCC " \
            __WX_BO_STRINGIZE(__GNUC__) "." \
            __WX_BO_STRINGIZE(__GNUC_MINOR__) "." \
            __WX_BO_STRINGIZE(__GNUC_PATCHLEVEL__)
#elif defined(__VISUALC__)
    #define __BO_COMPILER ",Visual C++"
#elif defined(__BORLANDC__)
    #define __BO_COMPILER ",Borland C++"
#elif defined(__DIGITALMARS__)
    #define __BO_COMPILER ",DigitalMars"
#elif defined(__WATCOMC__)
    #define __BO_COMPILER ",Watcom C++"
#else
    #define __BO_COMPILER ",unknown"
#endif

#if wxCHECK_VERSION( 2, 9, 0 )

static inline const char* KICAD_BUILD_OPTIONS_SIGNATURE()
{
    return
    " (" __WX_BO_UNICODE __ABI_VERSION __BO_COMPILER __WX_BO_STL
    __WX_BO_WXWIN_COMPAT_2_6 __WX_BO_WXWIN_COMPAT_2_8 ")"
    ;
}

#else

static inline const char* KICAD_BUILD_OPTIONS_SIGNATURE()
{
    return
    " (" __WX_BO_DEBUG ","
    __WX_BO_UNICODE __ABI_VERSION __BO_COMPILER __WX_BO_STL
    __WX_BO_WXWIN_COMPAT_2_4 __WX_BO_WXWIN_COMPAT_2_6 ")"
    ;
}

#endif

void EDA_BASE_FRAME::CopyVersionInfoToClipboard( wxCommandEvent&  event )
{
    if( !wxTheClipboard->Open() )
    {
        wxMessageBox( _( "Could not open clipboard to write version information." ),
                      _( "Clipboard Error" ), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    wxString tmp;
    wxPlatformInfo info;

    tmp = wxT( "Application: " ) + wxGetApp().GetTitle() + wxT( "\n" );
    tmp += wxT( "Version: " ) + GetBuildVersion() + wxT( "\n" );
    tmp << wxT( "Build: " ) << wxVERSION_STRING
        << FROM_UTF8( KICAD_BUILD_OPTIONS_SIGNATURE() ) << wxT( "\n" )
        << wxT( "Platform: " ) << wxGetOsDescription() << wxT( ", " )
        << info.GetArchName() << wxT( ", " ) << info.GetEndiannessName() << wxT( ", " )
        << info.GetPortIdName() << wxT( "\n" );

    tmp << wxT( "Options: " );

    tmp << wxT( "USE_PNG_BITMAPS=" );
#ifdef USE_PNG_BITMAPS
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         KICAD_GOST=" );
#ifdef KICAD_GOST
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         USE_WX_GRAPHICS_CONTEXT=" );
#ifdef USE_WX_GRAPHICS_CONTEXT
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         USE_WX_OVERLAY=" );
#ifdef USE_WX_OVERLAY
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    tmp << wxT( "         USE_BOOST_POLYGON_LIBRARY=" );
#ifdef USE_BOOST_POLYGON_LIBRARY
    tmp << wxT( "ON\n" );
#else
    tmp << wxT( "OFF\n" );
#endif

    wxTheClipboard->SetData( new wxTextDataObject( tmp ) );
    wxTheClipboard->Close();
}


bool EDA_BASE_FRAME::IsWritable( const wxFileName& aFileName )
{
    wxString msg;

    wxCHECK_MSG( aFileName.IsOk(), false, wxT( "Invalid file name object.  Bad programmer!" ) );

    if( aFileName.IsDir() && !aFileName.IsDirWritable() )
    {
        msg.Printf( _( "You do not have write permissions to folder <%s>." ),
                    GetChars( aFileName.GetPath() ) );
    }
    else if( !aFileName.FileExists() && !aFileName.IsDirWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file <%s> to folder <%s>." ),
                    GetChars( aFileName.GetFullName() ), GetChars( aFileName.GetPath() ) );
    }
    else if( aFileName.FileExists() && !aFileName.IsFileWritable() )
    {
        msg.Printf( _( "You do not have write permissions to save file <%s>." ),
                    GetChars( aFileName.GetFullPath() ) );
    }

    if( !msg.IsEmpty() )
    {
        DisplayError( this, msg );
        return false;
    }

    return true;
}
