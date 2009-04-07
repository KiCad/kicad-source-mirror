/***************/
/* eda_doc.cpp */
/***************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include <wx/mimetype.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>


/*****************************************/
void WinEDA_App::ReadPdfBrowserInfos()
/*****************************************/

/* Read from Common config the Pdf browser choice
 */
{
    wxASSERT( m_EDA_CommonConfig != NULL );

    m_PdfBrowserIsDefault = m_EDA_CommonConfig->Read( wxT( "PdfBrowserIsDefault" ),
                                                  true );
    m_PdfBrowser = m_EDA_CommonConfig->Read( wxT( "PdfBrowserName" ),
                                         wxEmptyString );

    if( m_PdfBrowser.IsEmpty() )
        m_PdfBrowserIsDefault = true;
}


/*****************************************/
void WinEDA_App::WritePdfBrowserInfos()
/*****************************************/

/* Write into Common config the Pdf browser choice
 */
{
    wxASSERT( m_EDA_CommonConfig != NULL );

    if( m_PdfBrowser.IsEmpty() )
        m_PdfBrowserIsDefault = true;

    m_EDA_CommonConfig->Write( wxT( "PdfBrowserIsDefault" ),
                           m_PdfBrowserIsDefault );
    m_EDA_CommonConfig->Write( wxT( "PdfBrowserName" ), m_PdfBrowser );
}


//  Mime type extensions (PDF files are not considered here)
static wxMimeTypesManager*  mimeDatabase;
static const wxFileTypeInfo EDAfallbacks[] =
{
    wxFileTypeInfo( wxT( "text/html" ),
        wxT( "wxhtml %s" ),
        wxT( "wxhtml %s" ),
        wxT( "html document (from Kicad)" ),
        wxT( "htm" ),                         wxT( "html" ),NULL ),

    wxFileTypeInfo( wxT( "application/sch" ),
        wxT( "eeschema %s" ),
        wxT( "eeschema -p %s" ),
        wxT( "sch document (from Kicad)" ),
        wxT( "sch" ),                         wxT( "SCH" ), NULL ),

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
    wxString docpath, fullfilename, file_ext;
    wxString msg;
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
        msg = _( "Doc File " );
        msg << wxT("\"") << fullfilename << wxT("\"") << _( " not found" );
        DisplayError( frame, msg );
        return FALSE;
    }

    wxFileName CurrentFileName( fullfilename );
    file_ext = CurrentFileName.GetExt();
    if( file_ext == wxT( "pdf" ) )
    {
        success = OpenPDF( fullfilename );
        return success;
    }

    /* Try to launch some browser (usefull under linux) */
    wxFileType* filetype;

    wxString    type;
    filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( file_ext );

    if( !filetype )       // 2ieme tentative
    {
        mimeDatabase = new wxMimeTypesManager;
        mimeDatabase->AddFallbacks( EDAfallbacks );
        filetype = mimeDatabase->GetFileTypeFromExtension( file_ext );
        delete mimeDatabase;
        mimeDatabase = NULL;
    }

    if( filetype )
    {
        wxFileType::MessageParameters params( fullfilename, type );

        success = filetype->GetOpenCommand( &command, params );
        delete filetype;
        if( success )
            success = ProcessExecute( command );
    }

    if( !success )
    {
        msg.Printf( _( "Unknown MIME type for doc file <%s>" ),
            fullfilename.GetData() );
        DisplayError( frame, msg );
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
        wxString          Key = Token.GetNextToken();

        // Search Key in Datalist:
        wxStringTokenizer Data( DataList, wxT( " \n\r" ) );

        while( Data.HasMoreTokens() )
        {
            wxString word = Data.GetNextToken();
            if( word == Key )
                return 1; // Key found !
        }
    }

    // keyword not found
    return 0;
}
