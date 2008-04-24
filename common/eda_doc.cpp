/***************/
/* eda_doc.cpp */
/***************/

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/mimetype.h>
#include <wx/tokenzr.h>

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"

/*****************************************/
void WinEDA_App::ReadPdfBrowserInfos()
/*****************************************/

/* Read from Common config the Pdf browser choice
 */
{
    if( m_EDA_CommonConfig )
    {
        m_PdfBrowserIsDefault = m_EDA_CommonConfig->Read( wxT( "PdfBrowserIsDefault" ), TRUE );
        m_PdfBrowser = m_EDA_CommonConfig->Read( wxT( "PdfBrowserName" ), wxEmptyString );
    }
    if( m_PdfBrowser.IsEmpty() )
        m_PdfBrowserIsDefault = TRUE;
}


/*****************************************/
void WinEDA_App::WritePdfBrowserInfos()
/*****************************************/

/* Write into Common config the Pdf browser choice
 */
{
    if( !m_EDA_CommonConfig )
        return;
    if( m_PdfBrowser.IsEmpty() )
        m_PdfBrowserIsDefault = TRUE;
    m_EDA_CommonConfig->Write( wxT( "PdfBrowserIsDefault" ), m_PdfBrowserIsDefault );
    m_EDA_CommonConfig->Write( wxT( "PdfBrowserName" ), m_PdfBrowser );
}


//  Mime type extensions
static wxMimeTypesManager*  mimeDatabase;
static const wxFileTypeInfo EDAfallbacks[] =
{
    wxFileTypeInfo( wxT( "text/pdf" ),
        wxT( "xpdf %s" ),
        wxT( "xpdf -p %s" ),
        wxT( "pdf document (from Kicad)" ),
        wxT( "pdf" ), wxT( "PDF" ),  NULL ),

    wxFileTypeInfo( wxT( "text/html" ),
        wxT( "wxhtml %s" ),
        wxT( "wxhtml %s" ),
        wxT( "html document (from Kicad)" ),
        wxT( "htm" ), wxT( "html" ), NULL ),

    wxFileTypeInfo( wxT( "application/sch" ),
        wxT( "eeschema %s" ),
        wxT( "eeschema -p %s" ),
        wxT( "sch document (from Kicad)" ),
        wxT( "sch" ), wxT( "SCH" ),  NULL ),

    // must terminate the table with this!
    wxFileTypeInfo()
};


/********************************************************************/
bool GetAssociatedDocument( wxFrame* frame, const wxString& LibPath,
                            const wxString& DocName )
/*********************************************************************/

/* Launch the viewer for the doc file  DocName (mime type)
  * LibPath is the doc file search path:
  * (kicad/library)
  * DocName is the short filename with ext. Wildcarts are allowed
  * Seach file is made in LibPath/doc/DocName
 *
  * if DocName is starting by http: or ftp: or www. the default internet browser is launched
 */
{
    wxString docpath, fullfilename;
    wxString Line;
    wxString command;
    bool     success = FALSE;

    // Is an internet url
    static const wxString url_header[3] = { wxT( "http:" ), wxT( "ftp:" ), wxT( "www." ) };

    for( int ii = 0; ii < 3; ii++ )
    {
        if( DocName.First( url_header[ii] ) == 0 )   //. seems an internet url
        {
            wxLaunchDefaultBrowser( DocName );
            return TRUE;
        }
    }

    /* Compute the full file name */
    if( wxIsAbsolutePath( DocName ) )
        fullfilename = DocName;
    else
    {
        docpath      = LibPath + wxT( "doc/" );
        fullfilename = docpath + DocName;
    }

#ifdef __WINDOWS__
    fullfilename.Replace( UNIX_STRING_DIR_SEP, WIN_STRING_DIR_SEP );
#else
    fullfilename.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
#endif

    wxString mask( wxT( "*" ) ), extension;

#ifdef __WINDOWS__
    mask     += wxT( ".*" );
    extension = wxT( ".*" );
#endif

    if( wxIsWild( fullfilename ) )
    {
        fullfilename =
            EDA_FileSelector( _( "Doc Files" ),     /* Titre de la fenetre */
                wxPathOnly( fullfilename ),         /* Chemin par defaut */
                fullfilename,                       /* nom fichier par defaut */
                extension,                          /* extension par defaut */
                mask,                               /* Masque d'affichage */
                frame,                              /* parent frame */
                wxFD_OPEN,                          /* wxSAVE, wxFD_OPEN ..*/
                TRUE,                               /* true = ne change pas le repertoire courant */
                wxPoint( -1, -1 )
                );
        if( fullfilename.IsEmpty() )
            return FALSE;
    }

    if( !wxFileExists( fullfilename ) )
    {
        Line = _( "Doc File " ) + fullfilename + _( " not found" );
        DisplayError( frame, Line );
        return FALSE;
    }

    /* Try to launch some browser (usefull under linux) */
    g_EDA_Appl->ReadPdfBrowserInfos();
    if( g_EDA_Appl->m_PdfBrowserIsDefault )
    {
        wxFileType* filetype;
        wxFileName CurrentFileName( fullfilename );

        wxString    ext, type;
        ext      = CurrentFileName.GetExt();
        filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( ext );

        if( !filetype )   // 2ieme tentative
        {
            mimeDatabase = new wxMimeTypesManager;
            mimeDatabase->AddFallbacks( EDAfallbacks );
            filetype = mimeDatabase->GetFileTypeFromExtension( ext );
            delete mimeDatabase;
            mimeDatabase = NULL;
        }

        if( filetype )
        {
            wxFileType::MessageParameters params( fullfilename, type );

            success = filetype->GetOpenCommand( &command, params );
            delete filetype;
            if( success )
                ProcessExecute( command );
        }

        if( !success )
        {
#ifdef __LINUX__
            if( ext == wxT( "pdf" ) )
            {
                success = TRUE; command.Empty();
                if( wxFileExists( wxT( "/usr/bin/kpdf" ) ) )
                    command = wxT( "xpdf " ) + fullfilename;
                else if( wxFileExists( wxT( "/usr/bin/konqueror" ) ) )
                    command = wxT( "konqueror " ) + fullfilename;
                else if( wxFileExists( wxT( "/usr/bin/gpdf" ) ) )
                    command = wxT( "gpdf " ) + fullfilename;
                if( command.IsEmpty() ) // not started
                {
                    DisplayError( frame,
                        _( " Cannot find the PDF viewer (kpdf, gpdf or konqueror) in /usr/bin/" ) );
                    success = FALSE;
                }
                else
                    ProcessExecute( command );
            }
            else
#endif
            {
                Line.Printf( _( "Unknown MIME type for Doc File [%s] (%s)" ),
                    fullfilename.GetData(), ext.GetData() );
                DisplayError( frame, Line );
            }
        }
    }
    else
    {
        command = g_EDA_Appl->m_PdfBrowser;
        if( wxFileExists( command ) )
        {
            success = TRUE;
            AddDelimiterString( fullfilename );
            command += wxT( " " ) + fullfilename;
            ProcessExecute( command );
        }
        else
        {
            Line.Printf( _( "Cannot find Pdf viewer %s" ), command.GetData() );
            DisplayError( frame, Line );
        }
    }

    return success;
}


/******************************************************************/
int KeyWordOk( const wxString& KeyList, const wxString& Database )
/******************************************************************/

/* Recherche si dans le texte Database on retrouve tous les mots
  * cles donnes dans KeyList ( KeyList = suite de mots cles
  * separes par des espaces
  * Retourne:
  *     0 si aucun mot cle trouvé
  *     1 si mot cle trouvé
 */
{
    wxString KeysCopy, DataList;

    if( KeyList.IsEmpty() )
        return 0;

    KeysCopy = KeyList; KeysCopy.MakeUpper();
    DataList = Database; DataList.MakeUpper();

    wxStringTokenizer Token( KeysCopy, wxT( " \n\r" ) );

    while( Token.HasMoreTokens() )
    {
        wxString Key = Token.GetNextToken();

        // Search Key in Datalist:
        wxStringTokenizer Data( DataList, wxT( " \n\r" ) );

        while( Data.HasMoreTokens() )
        {
            wxString word = Data.GetNextToken();
            if( word == Key )
                return 1;                   // Key found !
        }
    }

    // keyword not found
    return 0;
}
