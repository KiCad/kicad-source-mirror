/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file eda_doc.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <common.h>
#include <confirm.h>
#include <gestfich.h>

#include <wx/mimetype.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/uri.h>
#include <macros.h>


void PGM_BASE::ReadPdfBrowserInfos()
{
    wxASSERT( m_common_settings );

    wxString browser = m_common_settings->Read( wxT( "PdfBrowserName" ), wxEmptyString );
    SetPdfBrowserName( browser );

    int tmp;
    m_common_settings->Read( wxT( "UseSystemBrowser" ), &tmp, 0 );
    m_use_system_pdf_browser = bool( tmp );
}


void PGM_BASE::WritePdfBrowserInfos()
{
    wxASSERT( m_common_settings );

    m_common_settings->Write( wxT( "PdfBrowserName" ), GetPdfBrowserName() );
    m_common_settings->Write( wxT( "UseSystemBrowser" ), m_use_system_pdf_browser );
}


//  Mime type extensions (PDF files are not considered here)
static wxMimeTypesManager*  mimeDatabase;
static const wxFileTypeInfo EDAfallbacks[] =
{
    wxFileTypeInfo( wxT( "text/html" ),
                    wxT( "wxhtml %s" ),
                    wxT( "wxhtml %s" ),
                    wxT( "html document (from KiCad)" ),
                    wxT( "htm" ),
                    wxT( "html" ),wxNullPtr ),

    wxFileTypeInfo( wxT( "application/sch" ),
                    wxT( "eeschema %s" ),
                    wxT( "eeschema -p %s" ),
                    wxT( "sch document (from KiCad)" ),
                    wxT( "sch" ),
                    wxT( "SCH" ), wxNullPtr ),

    // must terminate the table with this!
    wxFileTypeInfo()
};


bool GetAssociatedDocument( wxWindow* aParent,
                            const wxString& aDocName,
                            const wxPathList* aPaths)

{
    wxString docname, fullfilename;
    wxString msg;
    wxString command;
    bool     success = false;

    // Is an internet url
    static const wxChar* url_header[] = {
        wxT( "http:" ),
        wxT( "https:" ),
        wxT( "ftp:" ),
        wxT( "www." ),
        wxT( "file:" )
    };

    for( unsigned ii = 0; ii < arrayDim(url_header); ii++ )
    {
        if( aDocName.First( url_header[ii] ) == 0 )   // looks like an internet url
        {
            wxURI uri( aDocName );
            wxLaunchDefaultBrowser( uri.BuildURI() );
            return true;
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
        fullfilename = EDA_FILE_SELECTOR( _( "Doc Files" ),
                                          wxPathOnly( fullfilename ),
                                          fullfilename,
                                          extension,
                                          mask,
                                          aParent,
                                          wxFD_OPEN,
                                          true,
                                          wxPoint( -1, -1 ) );
        if( fullfilename.IsEmpty() )
            return false;
    }

    if( !wxFileExists( fullfilename ) )
    {
        msg.Printf( _( "Doc File \"%s\" not found" ), GetChars( aDocName ) );
        DisplayError( aParent, msg );
        return false;
    }

    wxFileName currentFileName( fullfilename );

    wxString file_ext = currentFileName.GetExt();

    if( file_ext.Lower() == wxT( "pdf" ) )
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
        msg.Printf( _( "Unknown MIME type for doc file \"%s\"" ), GetChars( fullfilename ) );
        DisplayError( aParent, msg );
    }

    return success;
}


bool KeywordMatch( const wxString& aKeys, const wxString& aDatabase )
{
    if( aKeys.IsEmpty() )
        return false;

    wxStringTokenizer keyTokenizer( aKeys, wxT( ", \t\n\r" ), wxTOKEN_STRTOK );

    while( keyTokenizer.HasMoreTokens() )
    {
        wxString key = keyTokenizer.GetNextToken();

        // Search for key in aDatabase:
        wxStringTokenizer dataTokenizer( aDatabase, wxT( ", \t\n\r" ), wxTOKEN_STRTOK );

        while( dataTokenizer.HasMoreTokens() )
        {
            if( dataTokenizer.GetNextToken() == key )
                return true;
        }
    }

    // keyword not found
    return false;
}
