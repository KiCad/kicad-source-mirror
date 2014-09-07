/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "project_template.h"

#include <vector>

#include <wx/wx.h>
#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include <macros.h>


#define SEP()   wxFileName::GetPathSeparator()


PROJECT_TEMPLATE::PROJECT_TEMPLATE( const wxString& aPath )
{
    templateBasePath = wxFileName::DirName( aPath );
    templateMetaPath = wxFileName::DirName( aPath + SEP() + METADIR );
    templateMetaHtmlFile = wxFileName::FileName( aPath + SEP() + METADIR + SEP() +
                                                 METAFILE_INFO_HTML );
    templateMetaIconFile = wxFileName::FileName( aPath + SEP() + METADIR + SEP() + METAFILE_ICON );

    title = wxEmptyString;

    // Test the project template requirements to make sure aPath is a valid
    // template structure
    if( !wxFileName::DirExists( templateBasePath.GetPath() ) )
    {
        // Error, the path doesn't exist!
        title = wxT( "Could open the template path! " + aPath );
    }
    else if( !wxFileName::DirExists( templateMetaPath.GetPath() ) )
    {
        // Error, the meta information directory doesn't exist!
        title = wxT( "Couldn't open the meta information directory for this template! " +
                     templateMetaPath.GetPath() );
    }
    else if( !wxFileName::FileExists( templateMetaHtmlFile.GetFullPath() ) )
    {
        // Error, the meta information directory doesn't contain the informational html file!
        title = wxT( "Cound't find the meta html information file for this template!" );
    }

    // Try to load an icon
    metaIcon = new wxBitmap( templateMetaIconFile.GetFullPath(), wxBITMAP_TYPE_PNG );
}

std::vector<wxFileName> PROJECT_TEMPLATE::GetFileList()
{
    std::vector<wxFileName> files;
    wxString f = templateBasePath.GetPath();
    wxArrayString allfiles;
    wxFileName p;

    wxDir::GetAllFiles( f, &allfiles );

    // Create the vector and ignore all of the meta data files!
    for( size_t i=0; i < allfiles.size(); i++ )
    {
        p = allfiles[i];

        // Files that are in the meta directory must not be included
        if( !p.GetPath().StartsWith( templateMetaPath.GetPath() ) )
            files.push_back(allfiles[i]);
    }

    return files;
}


wxString PROJECT_TEMPLATE::GetPrjDirName()
{
    return templateBasePath.GetDirs()[ templateBasePath.GetDirCount()-1 ];
}


PROJECT_TEMPLATE::~PROJECT_TEMPLATE()
{

}


wxFileName PROJECT_TEMPLATE::GetHtmlFile()
{
    return templateMetaHtmlFile;
}


wxBitmap* PROJECT_TEMPLATE::GetIcon()
{
    return metaIcon;
}


bool PROJECT_TEMPLATE::CreateProject( wxFileName& aNewProjectPath )
{
    // CreateProject copy the files from template to the new project folder
    // and rename files which have the same name as the template .pro file
    bool result = true;

    std::vector<wxFileName> srcFiles = GetFileList();

    // Find the template file name base. this is the name of the .pro templte file
    wxString basename;
    for( size_t i=0; i < srcFiles.size(); i++ )
    {
        if( srcFiles[i].GetExt() == wxT( "pro" ) )
        {
            basename = srcFiles[i].GetName();
            break;
        }
    }

    for( size_t i=0; i < srcFiles.size(); i++ )
    {
        // Replace the template path
        wxFileName destination = srcFiles[i];

        // Replace the template filename with the project filename for the new project creation
        wxString currname = destination.GetName();
        currname.Replace( basename, aNewProjectPath.GetName() );
        destination.SetName( currname );

        // Replace the template path with the project path for the new project creation
        // but keep the sub directory name, if exists
        wxString destpath = destination.GetPathWithSep();
        destpath.Replace( templateBasePath.GetPathWithSep(), aNewProjectPath.GetPathWithSep() );

        // Check to see if the path already exists, if not attempt to create it here. Don't worry
        // about error checking, if the path isn't created the file copy will fail anyway

        if( !wxFileName::DirExists( destpath ) )
            wxFileName::Mkdir( destpath, 0777, wxPATH_MKDIR_FULL );

        destination.SetPath( destpath );

        wxString srcFile = srcFiles[i].GetFullPath();
        wxString dstFile = destination.GetFullPath();

        if( !wxCopyFile( srcFile, dstFile ) )
        {
            result = false;
            break;
        }
    }

    return result;
}


wxString* PROJECT_TEMPLATE::GetTitle(void)
{
    wxFileInputStream input( GetHtmlFile().GetFullPath() );
    wxString separator( wxT( "\x9" ) );
    wxTextInputStream text( input, separator, wxConvUTF8 );

    /* Open HTML file and get the text between the title tags */
    if( title == wxEmptyString )
    {
        int start = 0;
        int finish = 0;
        bool done = false;

        while( input.IsOk() && !input.Eof() && !done )
        {
            wxString line = text.ReadLine();

            start = line.Find( wxT( "<title>" ) );
            if( start == wxNOT_FOUND )
                start = line.Find( wxT( "<TITLE>" ) );

            finish = line.Find( wxT( "</title>" ) );
            if( finish == wxNOT_FOUND )
                finish = line.Find( wxT( "</TITLE>" ) );

            // find the opening tag
            if( start != wxNOT_FOUND )
            {
                if( finish != wxNOT_FOUND )
                {
                    title = line.SubString( start + 7, finish );
                }
                else
                {
                    title = line.SubString( start + 7, line.Len() - 1 );
                    done = true;
                }
            }
            else
            {
                if( finish != wxNOT_FOUND )
                {
                    title += line.SubString( 0, finish );
                    done = true;
                }
                else
                {
                    title += line;
                }
            }

            // Remove line endings
            title.Replace( wxT( "\r" ), wxT( "" ) );
            title.Replace( wxT( "\n" ), wxT( "" ) );
        }
    }

    return &title;
}
