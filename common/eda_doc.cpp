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
#include "macros.h"


/* Read from Common config the Pdf browser choice
 */
void WinEDA_App::ReadPdfBrowserInfos()
{
    wxASSERT( m_EDA_CommonConfig != NULL );

    m_PdfBrowserIsDefault =
        m_EDA_CommonConfig->Read( wxT( "PdfBrowserIsDefault" ), true );
    m_PdfBrowser = m_EDA_CommonConfig->Read( wxT( "PdfBrowserName" ),
                                             wxEmptyString );

    if( m_PdfBrowser.IsEmpty() )
        m_PdfBrowserIsDefault = true;
}


/* Write into Common config the Pdf browser choice
 */
void WinEDA_App::WritePdfBrowserInfos()
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
                    wxT( "htm" ),
                    wxT( "html" ),NULL ),

    wxFileTypeInfo( wxT( "application/sch" ),
                    wxT( "eeschema %s" ),
                    wxT( "eeschema -p %s" ),
                    wxT( "sch document (from Kicad)" ),
                    wxT( "sch" ),
                    wxT( "SCH" ), NULL ),

    // must terminate the table with this!
    wxFileTypeInfo()
};


/**
 * Function GetAssociatedDocument
 * open a document (file) with the suitable browser
 * @param aFrame = main frame
 * if DocName is starting by http: or ftp: or www. the default internet
 * browser is launched
 * @param aDocName = filename of file to open (Full filename or short filename)
 * @param aPaths = a wxPathList to explore.
 *                 if NULL or aDocName is a full filename, aPath is not used.
 */
bool    GetAssociatedDocument( wxFrame* aFrame,
                               const wxString& aDocName,
                               const wxPathList* aPaths)

{
    wxString docname, fullfilename, file_ext;
    wxString msg;
    wxString command;
    bool     success = FALSE;

    // Is an internet url
    static const wxString url_header[3] = { wxT( "http:" ), wxT( "ftp:" ),
                                            wxT( "www." ) };

    for( int ii = 0; ii < 3; ii++ )
    {
        if( aDocName.First( url_header[ii] ) == 0 )   //. seems an internet url
        {
            wxLaunchDefaultBrowser( aDocName );
            return TRUE;
        }
    }

    docname = aDocName;
    #ifdef __WINDOWS__
    docname.Replace( UNIX_STRING_DIR_SEP, WIN_STRING_DIR_SEP );
#else
    docname.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
#endif


    /* Compute the full file name */
    if( wxIsAbsolutePath( aDocName ) || aPaths == NULL)
        fullfilename = aDocName;
    /* If the file exists, this is a trivial case: return the filename
     * "as this".  the name can be an absolute path, or a relative path
     * like ./filename or ../<filename>
     */
    else if( wxFileName::FileExists( aDocName ) )
        fullfilename = aDocName;
    else
    {
        fullfilename = aPaths->FindValidPath( aDocName );
    }

    wxString mask( wxT( "*" ) ), extension;

#ifdef __WINDOWS__
    mask     += wxT( ".*" );
    extension = wxT( ".*" );
#endif

    if( wxIsWild( fullfilename ) )
    {
        fullfilename = EDA_FileSelector( _( "Doc Files" ),
                                         wxPathOnly( fullfilename ),
                                         fullfilename,
                                         extension,
                                         mask,
                                         aFrame,
                                         wxFD_OPEN,
                                         TRUE,
                                         wxPoint( -1, -1 ) );
        if( fullfilename.IsEmpty() )
            return FALSE;
    }

    if( !wxFileExists( fullfilename ) )
    {
        msg = _( "Doc File " );
        msg << wxT("\"") << aDocName << wxT("\"") << _( " not found" );
        DisplayError( aFrame, msg );
        return FALSE;
    }

    wxFileName CurrentFileName( fullfilename );
    file_ext = CurrentFileName.GetExt();
    if( file_ext == wxT( "pdf" ) )
    {
        success = OpenPDF( fullfilename );
        return success;
    }

    /* Try to launch some browser (useful under linux) */
    wxFileType* filetype;

    wxString    type;
    filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( file_ext );

    if( !filetype )       // 2nd attempt.
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
            GetChars( fullfilename ) );
        DisplayError( aFrame, msg );
    }

    return success;
}


/* Search if the text Database found all the words in the KeyList.
 * Give articles in keylist (keylist = Following Keywords
 * Separated by spaces
 * Returns:
 * 0 if no keyword found
 * 1 if keyword found
 */
int KeyWordOk( const wxString& KeyList, const wxString& Database )
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
