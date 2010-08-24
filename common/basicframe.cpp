/**
 * WinEDA_BasicFrame Class Functions
 * @file basicframe.cpp
 */

#include <wx/aboutdlg.h>
#include <wx/fontdlg.h>
#include <wx/clipbrd.h>
#include <wx/statline.h>
#include <wx/aboutdlg.h>
#include <wx/platinfo.h>

#include "build_version.h"
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "online_help.h"
#include "id.h"
#include "confirm.h"
#include "eda_doc.h"
#include "wxstruct.h"
#include "macros.h"
#include "bitmaps.h"


/*
 * Class constructor for WinEDA_BasicFrame general options
 */
WinEDA_BasicFrame::WinEDA_BasicFrame( wxWindow* father,
                                      int idtype,
                                      const wxString& title,
                                      const wxPoint& pos,
                                      const wxSize& size,
                                      long style ) :
    wxFrame( father, -1, title, pos, size, style )
{
    wxSize minsize;

    m_Ident          = idtype;
    m_HToolBar       = NULL;
    m_FrameIsActive  = TRUE;

    m_MsgFrameHeight = WinEDA_MsgPanel::GetRequiredHeight();

    minsize.x = 470;
    minsize.y = 350 + m_MsgFrameHeight;

    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    if( ( size.x < minsize.x ) || ( size.y < minsize.y ) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y ); /* dimensions of the user
                                                      * area of the main
                                                      * window */
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    Connect( ID_HELP_COPY_VERSION_STRING,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler( WinEDA_BasicFrame::CopyVersionInfoToClipboard ) );
}


WinEDA_BasicFrame::~WinEDA_BasicFrame()
{
    if( wxGetApp().m_HtmlCtrl )
        delete wxGetApp().m_HtmlCtrl;
    wxGetApp().m_HtmlCtrl = NULL;

    /* This needed for OSX: avoids further OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    this->Freeze();
}


/*
 * Virtual function
 */
void WinEDA_BasicFrame::ReCreateMenuBar()
{

}

/** Virtual function SetLanguage
 * called on a language menu selection
 * when using a derived function, do not forget to call this one
 */
void WinEDA_BasicFrame::SetLanguage( wxCommandEvent& event )
{
    int id = event.GetId();

    wxGetApp().SetLanguageIdentifier( id );
    if ( wxGetApp().SetLanguage() )
    {
        ReCreateMenuBar();
        Refresh();
    }
}


/**
 * Load common frame parameters from configuration.
 *
 * The method is virtual so you can override it to load frame specific
 * parameters.  Don't forget to call the base method or your frames won't
 * remember their positions and sizes.
 */
void WinEDA_BasicFrame::LoadSettings()
{
    wxString  text;
    int       Ypos_min;
    wxConfig* config;

    config = wxGetApp().m_EDA_Config;

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
}


/**
 * Save common frame parameters from configuration.
 *
 * The method is virtual so you can override it to save frame specific
 * parameters.  Don't forget to call the base method or your frames won't
 * remember their positions and sizes.
 */
void WinEDA_BasicFrame::SaveSettings()
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
}


void WinEDA_BasicFrame::PrintMsg( const wxString& text )
{
    SetStatusText( text );
}


/*
 * Display a bargraph (0 to 50 point length) for a PerCent value from 0 to 100
 */
void WinEDA_BasicFrame::DisplayActivity( int PerCent, const wxString& Text )
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


/*
 * Update the list of past projects.
 */
void WinEDA_BasicFrame::SetLastProject( const wxString& FullFileName )
{
    wxGetApp().m_fileHistory.AddFileToHistory( FullFileName );
    ReCreateMenuBar();
}


/*
 * Fetch the file name from the file history list.
 */
wxString WinEDA_BasicFrame::GetFileFromHistory( int cmdId, const wxString& type )
{
    wxString fn, msg;
    size_t   i;
    int      baseId = wxGetApp().m_fileHistory.GetBaseId();

    wxASSERT( cmdId >= baseId
              && cmdId < baseId + ( int )wxGetApp().m_fileHistory.GetCount() );

    i = ( size_t )( cmdId - baseId );

    if( i < wxGetApp().m_fileHistory.GetCount() )
    {
        fn = wxGetApp().m_fileHistory.GetHistoryFile( i );
        if( !wxFileName::FileExists( fn ) )
        {
            msg = type + _( " file <" ) + fn + _( "> was not found." );
            DisplayError( this, msg );
            wxGetApp().m_fileHistory.RemoveFileFromHistory( i );
            fn = wxEmptyString;
            ReCreateMenuBar();
        }
    }

    return fn;
}


/*
 *
 */
void WinEDA_BasicFrame::GetKicadHelp( wxCommandEvent& event )
{
    wxString msg;

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
        msg.Printf( _( "Help file %s not found" ), GetChars( wxGetApp().m_HelpFileName ) );
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
        GetAssociatedDocument( this, helpFile );

#else
#   error Help files format not defined
#endif
}


/*
 *
 */
void WinEDA_BasicFrame::GetKicadAbout( wxCommandEvent& WXUNUSED(event) )
{
    wxAboutDialogInfo info;
    InitKiCadAbout(info);
    wxAboutBox(info);
}


void WinEDA_BasicFrame::AddHelpVersionInfoMenuEntry( wxMenu* aMenu )
{
    wxASSERT( aMenu != NULL );

    wxMenuItem* item = NULL;

    // Copy version string to clipboard for bug report purposes.
    item = new wxMenuItem( aMenu, ID_HELP_COPY_VERSION_STRING,
                           _( "Copy &Version Information" ),
                           _( "Copy the version string to clipboard to send with bug reports" ) );

    // For some reason images are not always added to the OSX menu items.  Anyone want
    // to clarify as to why this is the case?  Putting this information in some formal
    // developer notes would be helpful.  A good place to put this information would be
    // ./documentation/guidelines/UIpolicies.txt.
#if !defined( __WXMAC__ )
    item->SetBitmap( copy_button );
#endif

    aMenu->Append( item );
}


void WinEDA_BasicFrame::CopyVersionInfoToClipboard( wxCommandEvent& WXUNUSED( event ) )
{
    if( !wxTheClipboard->Open() )
    {
        wxMessageBox( _( "Could not open clipboard to write version information." ),
                      _( "Clipboard Error" ), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    wxString tmp;
    wxPlatformInfo info;

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

#define KICAD_BUILD_OPTIONS_SIGNATURE \
    " (" __WX_BO_DEBUG "," __WX_BO_UNICODE \
     __ABI_VERSION __BO_COMPILER \
     __WX_BO_STL \
     __WX_BO_WXWIN_COMPAT_2_4 __WX_BO_WXWIN_COMPAT_2_6 \
     ")"

    tmp = wxT( "Application: " ) + wxGetApp().GetTitle() + wxT( "\n" );
    tmp += wxT( "Version: " ) + GetBuildVersion() + wxT( "\n" );
    tmp << wxT( "Build: " ) << wxVERSION_STRING
        << wxT( KICAD_BUILD_OPTIONS_SIGNATURE ) << wxT( "\n" )
        << wxT( "Platform: " ) << wxGetOsDescription() << wxT( ", " )
        << info.GetArchName() << wxT( ", " ) << info.GetEndiannessName() << wxT( ", " )
        << info.GetPortIdName();
    wxTheClipboard->SetData( new wxTextDataObject( tmp ) );
    wxTheClipboard->Close();
}
