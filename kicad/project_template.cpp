/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
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


#include <wx/bitmap.h>
#include <wx/dir.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <unordered_map>

#include <wildcards_and_files_ext.h>
#include "project_template.h"


#define SEP wxFileName::GetPathSeparator()


PROJECT_TEMPLATE::PROJECT_TEMPLATE( const wxString& aPath )
{
    m_basePath = wxFileName::DirName( aPath );
    m_metaPath = wxFileName::DirName( aPath + SEP + METADIR );
    m_metaHtmlFile = wxFileName::FileName( aPath + SEP + METADIR + SEP + METAFILE_INFO_HTML );
    m_metaIconFile = wxFileName::FileName( aPath + SEP + METADIR + SEP + METAFILE_ICON );

    m_title = wxEmptyString;

    // Test the project template requirements to make sure aPath is a valid template structure.
    if( !wxFileName::DirExists( m_basePath.GetPath() ) )
    {
        // Error, the path doesn't exist!
        m_title.Printf( _( "Could not open the template path '%s'" ), aPath );
    }
    else if( !wxFileName::DirExists( m_metaPath.GetPath() ) )
    {
        // Error, the meta information directory doesn't exist!
        m_title.Printf( _( "Could not find the expected 'meta' directory at '%s'" ), m_metaPath.GetPath() );
    }
    else if( !wxFileName::FileExists( m_metaHtmlFile.GetFullPath() ) )
    {
        // Error, the meta information directory doesn't contain the informational html file!
        m_title.Printf( _( "Could not find the expected meta HTML file at '%s'" ), m_metaHtmlFile.GetFullPath() );
    }

    // Try to load an icon
    if( !wxFileName::FileExists( m_metaIconFile.GetFullPath() ) )
        m_metaIcon = &wxNullBitmap;
    else
        m_metaIcon = new wxBitmap( m_metaIconFile.GetFullPath(), wxBITMAP_TYPE_PNG );
}


class FILE_TRAVERSER : public wxDirTraverser
{
public:
    FILE_TRAVERSER( std::vector<wxFileName>& files, const wxString& exclude ) :
            m_files( files ),
            m_exclude( exclude )
    {
    }

    virtual wxDirTraverseResult OnFile( const wxString& filename ) override
    {
        wxFileName fn( filename );
        wxString   path( fn.GetPathWithSep() );

        EnsureGitFiles( path );

        if( IsIgnored( path, fn.GetFullName(), false ) )
            return wxDIR_CONTINUE;

        bool exclude = fn.GetName().Contains( "fp-info-cache" )
                       || fn.GetName().StartsWith( FILEEXT::LockFilePrefix );

        if( !exclude )
            m_files.emplace_back( wxFileName( filename ) );

        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir( const wxString& dirname ) override
    {
        wxFileName dir( dirname );
        wxString   parent = dir.GetPathWithSep();

        EnsureGitFiles( parent );

        if( dir.GetFullName() == wxT( ".git" ) || IsIgnored( parent, dir.GetFullName(), true )
            || dirname.StartsWith( m_exclude ) || dirname.EndsWith( "-backups" ) )
        {
            return wxDIR_IGNORE;
        }

        m_files.emplace_back( wxFileName::DirName( dirname ) );
        EnsureGitFiles( dirname + wxFileName::GetPathSeparator() );
        return wxDIR_CONTINUE;
    }

private:
    void EnsureGitFiles( const wxString& path )
    {
        if( m_gitIgnores.find( path ) != m_gitIgnores.end() )
            return;

        wxString gitignore = path + wxT( ".gitignore" );

        if( wxFileExists( gitignore ) )
        {
            wxFileInputStream input( gitignore );
            wxTextInputStream text( input, wxT( "\x9" ), wxConvUTF8 );

            while( input.IsOk() && !input.Eof() )
            {
                wxString line = text.ReadLine();

                line.Trim().Trim( false );

                if( line.IsEmpty() || line.StartsWith( wxT( "#" ) ) )
                    continue;

                m_gitIgnores[path].push_back( line );
            }

            m_files.emplace_back( wxFileName( gitignore ) );
        }
        else
        {
            m_gitIgnores[path] = {};
        }

        wxString gitattributes = path + wxT( ".gitattributes" );

        if( wxFileExists( gitattributes ) )
            m_files.emplace_back( wxFileName( gitattributes ) );
    }

    bool IsIgnored( const wxString& path, const wxString& name, bool isDir )
    {
        auto it = m_gitIgnores.find( path );

        if( it == m_gitIgnores.end() )
            return false;

        for( const wxString& pattern : it->second )
        {
            bool     dirOnly = pattern.EndsWith( wxT( "/" ) );
            wxString pat = dirOnly ? pattern.substr( 0, pattern.length() - 1 ) : pattern;

            if( dirOnly && !isDir )
                continue;

            if( wxMatchWild( pat, name ) )
                return true;
        }

        return false;
    }

    std::vector<wxFileName>&                            m_files;
    wxString                                            m_exclude;
    std::unordered_map<wxString, std::vector<wxString>> m_gitIgnores;
};


std::vector<wxFileName> PROJECT_TEMPLATE::GetFileList()
{
    std::vector<wxFileName> files;
    FILE_TRAVERSER          sink( files, m_metaPath.GetPath() );
    wxDir                   dir( m_basePath.GetPath() );

    dir.Traverse( sink, wxEmptyString, ( wxDIR_FILES | wxDIR_DIRS ) );
    return files;
}


wxString PROJECT_TEMPLATE::GetPrjDirName()
{
    return m_basePath.GetDirs()[m_basePath.GetDirCount() - 1];
}


PROJECT_TEMPLATE::~PROJECT_TEMPLATE()
{
}


wxFileName PROJECT_TEMPLATE::GetHtmlFile()
{
    return m_metaHtmlFile;
}


wxBitmap* PROJECT_TEMPLATE::GetIcon()
{
    return m_metaIcon;
}


size_t PROJECT_TEMPLATE::GetDestinationFiles( const wxFileName& aNewProjectPath, std::vector<wxFileName>& aDestFiles )
{
    std::vector<wxFileName> srcFiles = GetFileList();

    // Find the template file name base. this is the name of the .pro template file
    wxString basename;
    bool     multipleProjectFilesFound = false;

    for( wxFileName& file : srcFiles )
    {
        if( file.GetExt() == FILEEXT::ProjectFileExtension || file.GetExt() == FILEEXT::LegacyProjectFileExtension )
        {
            if( !basename.IsEmpty() && basename != file.GetName() )
                multipleProjectFilesFound = true;

            basename = file.GetName();
        }
    }

    if( multipleProjectFilesFound )
        basename = GetPrjDirName();

    for( wxFileName& srcFile : srcFiles )
    {
        // Replace the template path
        wxFileName destFile = srcFile;

        // Replace the template filename with the project filename for the new project creation
        wxString name = destFile.GetName();
        name.Replace( basename, aNewProjectPath.GetName() );
        destFile.SetName( name );

        // Replace the template path with the project path, also renaming any subdirectories
        // that contain the template basename.
        wxString path = destFile.GetPathWithSep();
        path.Replace( m_basePath.GetPathWithSep(), aNewProjectPath.GetPathWithSep() );
        path.Replace( SEP + basename + SEP, SEP + aNewProjectPath.GetName() + SEP );
        path.Replace( SEP + basename + wxS( "-" ), SEP + aNewProjectPath.GetName() + wxS( "-" ) );
        destFile.SetPath( path );

        aDestFiles.push_back( destFile );
    }

    return aDestFiles.size();
}


bool PROJECT_TEMPLATE::CreateProject( wxFileName& aNewProjectPath, wxString* aErrorMsg )
{
    // CreateProject copy the files from template to the new project folder and renames files
    // which have the same name as the template .kicad_pro file
    bool result = true;

    std::vector<wxFileName> srcFiles = GetFileList();

    // Find the template file name base. this is the name of the .kicad_pro (or .pro) template
    // file
    wxString basename;
    bool     multipleProjectFilesFound = false;

    for( wxFileName& file : srcFiles )
    {
        if( file.GetExt() == FILEEXT::ProjectFileExtension || file.GetExt() == FILEEXT::LegacyProjectFileExtension )
        {
            if( !basename.IsEmpty() && basename != file.GetName() )
                multipleProjectFilesFound = true;

            basename = file.GetName();
        }
    }

    if( multipleProjectFilesFound )
        basename = GetPrjDirName();

    for( wxFileName& srcFile : srcFiles )
    {
        // Replace the template path
        wxFileName destFile = srcFile;

        // Replace the template filename with the project filename for the new project creation
        wxString currname = destFile.GetName();

        if( destFile.GetExt() == FILEEXT::DrawingSheetFileExtension )
        {
            // Don't rename drawing sheet definitions; they're often shared
        }
        else if( destFile.GetName().EndsWith( "-cache" ) || destFile.GetName().EndsWith( "-rescue" ) )
        {
            currname.Replace( basename, aNewProjectPath.GetName() );
        }
        else if( destFile.GetExt() == FILEEXT::LegacySymbolDocumentFileExtension
                 || destFile.GetExt() == FILEEXT::LegacySymbolLibFileExtension
                 // Footprint libraries are directories not files, so GetExt() won't work
                 || destFile.GetPath().EndsWith( '.' + FILEEXT::KiCadFootprintLibPathExtension ) )
        {
            // Don't rename project-specific libraries.  This will break the library tables and
            // cause broken links in the schematic/pcb.
        }
        else
        {
            currname.Replace( basename, aNewProjectPath.GetName() );
        }

        destFile.SetName( currname );

        // Replace the template path with the project path for the new project creation,
        // also renaming any subdirectories that contain the template basename.
        wxString destpath = destFile.GetPathWithSep();
        destpath.Replace( m_basePath.GetPathWithSep(), aNewProjectPath.GetPathWithSep() );
        destpath.Replace( SEP + basename + SEP, SEP + aNewProjectPath.GetName() + SEP );
        destpath.Replace( SEP + basename + wxS( "-" ), SEP + aNewProjectPath.GetName() + wxS( "-" ) );

        // Check to see if the path already exists, if not attempt to create it here.
        if( !wxFileName::DirExists( destpath ) )
        {
            if( !wxFileName::Mkdir( destpath, 0777, wxPATH_MKDIR_FULL ) )
            {
                if( aErrorMsg )
                {
                    if( !aErrorMsg->empty() )
                        *aErrorMsg += "\n";

                    wxString msg;

                    msg.Printf( _( "Cannot create folder '%s'." ), destpath );
                    *aErrorMsg += msg;
                }

                continue;
            }
        }

        destFile.SetPath( destpath );

        if( srcFile.FileExists() && !wxCopyFile( srcFile.GetFullPath(), destFile.GetFullPath() ) )
        {
            if( aErrorMsg )
            {
                if( !aErrorMsg->empty() )
                    *aErrorMsg += "\n";

                wxString msg;

                msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
                *aErrorMsg += msg;
            }

            result = false;
        }
    }

    return result;
}


wxString* PROJECT_TEMPLATE::GetTitle()
{
    wxFFileInputStream input( GetHtmlFile().GetFullPath() );
    wxString           separator( wxT( "\x9" ) );
    wxTextInputStream  text( input, separator, wxConvUTF8 );

    /* Open HTML file and get the text between the title tags */
    if( m_title == wxEmptyString )
    {
        int  start = 0;
        int  finish = 0;
        bool done = false;
        bool hasStart = false;

        while( input.IsOk() && !input.Eof() && !done )
        {
            wxString line = text.ReadLine();
            wxString upperline = line.Clone().Upper();

            start = upperline.Find( wxT( "<TITLE>" ) );
            finish = upperline.Find( wxT( "</TITLE>" ) );
            int length = finish - start - 7;

            // find the opening tag
            if( start != wxNOT_FOUND )
            {
                if( finish != wxNOT_FOUND )
                {
                    m_title = line( start + 7, length );
                    done = true;
                }
                else
                {
                    m_title = line.Mid( start + 7 );
                    hasStart = true;
                }
            }
            else
            {
                if( finish != wxNOT_FOUND )
                {
                    m_title += line.SubString( 0, finish - 1 );
                    done = true;
                }
                else if( hasStart )
                    m_title += line;
            }
        }

        // Remove line endings
        m_title.Replace( wxT( "\r" ), wxT( "" ) );
        m_title.Replace( wxT( "\n" ), wxT( "" ) );

        m_title.Trim( false ); // Trim from left
        m_title.Trim();        // Trim from right
    }

    return &m_title;
}
