/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>
#include <confirm.h>
#include <eda_doc.h>
#include <embedded_files.h>
#include <gestfich.h>
#include <settings/common_settings.h>

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mimetype.h>
#include <wx/uri.h>


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


bool GetAssociatedDocument( wxWindow* aParent, const wxString& aDocName, PROJECT* aProject,
                            SEARCH_STACK* aPaths, std::vector<EMBEDDED_FILES*> aFilesStack )
{
    wxString      docname;
    wxString      fullfilename;
    wxString      msg;
    wxString      command;
    bool          success = false;

    // Replace before resolving as we might have a URL in a variable
    docname = ResolveUriByEnvVars( aDocName, aProject );

    // We don't want the wx error message about not being able to open the URI
    {
        wxURI     uri( docname );
        wxLogNull logNo; // Disable log messages

        if( uri.HasScheme() )
        {
            wxString scheme = uri.GetScheme().Lower();

            if( scheme != FILEEXT::KiCadUriPrefix )
            {
                if( wxLaunchDefaultBrowser( docname ) )
                    return true;
            }
            else
            {
                if( aFilesStack.empty() )
                {
                    wxLogTrace( wxT( "KICAD_EMBED" ),
                                wxT( "No EMBEDDED_FILES object provided for kicad_embed URI" ) );
                    return false;
                }

                if( !docname.starts_with( FILEEXT::KiCadUriPrefix + "://" ) )
                {
                    wxLogTrace( wxT( "KICAD_EMBED" ),
                                wxT( "Invalid kicad_embed URI '%s'" ), docname );
                    return false;
                }

                docname = docname.Mid( wxString( FILEEXT::KiCadUriPrefix + "://" ).length() );

                wxFileName temp_file = aFilesStack[0]->GetTemporaryFileName( docname );
                int        ii = 1;

                while( !temp_file.IsOk() && ii < (int) aFilesStack.size() )
                    temp_file = aFilesStack[ii++]->GetTemporaryFileName( docname );

                if( !temp_file.IsOk() )
                {
                    wxLogTrace( wxT( "KICAD_EMBED" ),
                                wxT( "Failed to get temp file '%s' for kicad_embed URI" ), docname );
                    return false;
                }

                wxLogTrace( wxT( "KICAD_EMBED" ),
                            wxT( "Opening embedded file '%s' as '%s'" ),
                            docname,
                            temp_file.GetFullPath() );
                docname = temp_file.GetFullPath();
            }
        }
    }

#ifdef __WINDOWS__
    docname.Replace( UNIX_STRING_DIR_SEP, WIN_STRING_DIR_SEP );
#else
    docname.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
#endif

    /* Compute the full file name */
    if( wxIsAbsolutePath( docname ) || aPaths == nullptr )
        fullfilename = docname;
    /* If the file exists, this is a trivial case: return the filename "as this".  the name can
     * be an absolute path, or a relative path like ./filename or ../<filename>.
     */
    else if( wxFileName::FileExists( docname ) )
        fullfilename = docname;
    else
        fullfilename = aPaths->FindValidPath( docname );

    wxString extension;

#ifdef __WINDOWS__
    extension = wxT( ".*" );
#endif

    if( wxIsWild( fullfilename ) )
    {
        fullfilename = wxFileSelector( _( "Documentation File" ), wxPathOnly( fullfilename ),
                                       fullfilename, extension, wxFileSelectorDefaultWildcardStr,
                                       wxFD_OPEN, aParent );

        if( fullfilename.IsEmpty() )
            return false;
    }

    if( !wxFileExists( fullfilename ) )
    {
        msg.Printf( _( "Documentation file '%s' not found." ), docname );
        DisplayErrorMessage( aParent, msg );
        return false;
    }

    wxFileName currentFileName( fullfilename );

    // Use wxWidgets to resolve any "." and ".." in the path
    fullfilename = currentFileName.GetAbsolutePath();

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
        mimeDatabase = nullptr;
    }

    if( filetype )
    {
        wxFileType::MessageParameters params( fullfilename, type );

        success = filetype->GetOpenCommand( &command, params );
        delete filetype;

        if( success )
            success = wxExecute( command );
    }

    if( !success )
    {
        msg.Printf( _( "Unknown MIME type for documentation file '%s'" ), fullfilename );
        DisplayErrorMessage( aParent, msg );
    }

    return success;
}
