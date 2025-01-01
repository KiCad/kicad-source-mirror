/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "bom_plugins.h"
#include <config.h>
#include <paths.h>
#include <wx/ffile.h>
#include <wx/log.h>


const wxChar BOM_TRACE[] = wxT( "BOM_GENERATORS" );


BOM_GENERATOR_HANDLER::BOM_GENERATOR_HANDLER( const wxString& aFile )
    : m_storedPath( aFile )
{
    m_isOk = false;
    m_file = wxFileName( aFile );

    if( !wxFile::Exists( m_file.GetFullPath() ) )
        m_file = FindFilePath();

    if( !wxFile::Exists( m_file.GetFullPath() ) )
    {
        m_info.Printf( _("Script file:\n%s\nnot found. Script not available."), aFile );
        return;
    }

    m_isOk = true;

    m_name = m_file.GetName();
    wxString extension = m_file.GetExt().Lower();

    // Important note:
    // On Windows the right command command to run a python script is:
    // python <script_path>/script.py
    // and *not* python <script_path>\script.py
    // Otherwise the script does not find some auxiliary pythons scripts needed by this script
    if( extension == wxS( "xsl" ) )
    {
        m_info = readHeader( wxS( "-->" ) );
        m_cmd = wxString::Format( wxS( "xsltproc -o \"%%O%s\" \"%s\" \"%%I\"" ),
                                  getOutputExtension( m_info ),
                                  m_file.GetFullPath() );
    }
    else if( extension == wxS( "py" ) )
    {
        m_info = readHeader( wxS( "\"\"\"" ) );
#ifdef __WINDOWS__
        m_cmd = wxString::Format( "python \"%s/%s\" \"%%I\" \"%%O%s\"",
                                  m_file.GetPath(),
                                  m_file.GetFullName(),
                                  getOutputExtension( m_info ) );
#else
// For macOS, we want to use the Python we bundle along, rather than just PYTHON_EXECUTABLE.
// For non-Windows, non-macOS, we can call out to PYTHON_EXECUTABLE.
#ifdef __APPLE__
        // python is at Contents/Frameworks/Python.framework/Versions/Current/bin/python3

        // Of course, for macOS, it's not quite that simple, since the relative path
        // will depend on if we are in standalone mode or not.

        // (If we're going to call out to python like this in other places, we probably want to
        // think about pulling this into PATHS.)

        wxFileName python( PATHS::GetOSXKicadDataDir(), wxEmptyString );
        python.RemoveLastDir();
        python.AppendDir( wxT( "Frameworks" ) );
        python.AppendDir( wxT( "Python.framework" ) );
        python.AppendDir( wxT( "Versions" ) );
        python.AppendDir( wxT( "Current" ) );
        python.AppendDir( wxT( "bin" ) );
        python.SetFullName(wxT( "python3" ) );

        wxString interpreter = python.GetFullPath();
#else
        wxString interpreter = wxString::FromUTF8Unchecked( PYTHON_EXECUTABLE );
#endif
        if( interpreter.IsEmpty() )
            interpreter = wxT( "python" ); // For macOS, should we log here? Error here?

        m_cmd = wxString::Format( "%s \"%s\" \"%%I\" \"%%O%s\"",
                                  interpreter,
                                  m_file.GetFullPath(),
                                  getOutputExtension( m_info ) );
#endif
    }
#ifdef __WINDOWS__
    else if( extension == wxS( "pyw" ) )
    {
        m_info = readHeader( wxS( "\"\"\"" ) );
        m_cmd = wxString::Format( wxS( "pythonw \"%s/%s\" \"%%I\" \"%%O%s\"" ),
                                  m_file.GetPath(),
                                  m_file.GetFullName(),
                                  getOutputExtension( m_info ) );
    }
#endif /* __WINDOWS__ */
    else // fallback
    {
        m_cmd = m_file.GetFullPath();
    }

    wxLogTrace( BOM_TRACE, wxS( "%s: extracted command line %s" ), m_name, m_cmd );
}


bool BOM_GENERATOR_HANDLER::IsValidGenerator( const wxString& aFile )
{
    wxFileName fn( aFile );
    wxString ext = fn.GetExt().Lower();

    for( const auto& pluginExt : { wxS( "xsl" ), wxS( "py" ), wxS( "pyw" ) } )
    {
        if( pluginExt == ext )
            return true;
    }

    return false;
}


wxString BOM_GENERATOR_HANDLER::readHeader( const wxString& aEndSection )
{
    if( aEndSection.IsEmpty() )
        return wxEmptyString;

    wxFFile fdata( m_file.GetFullPath(), wxS( "rb" ) );        // dtor will close the file
    wxString data;

    if( !fdata.ReadAll( &data ) )
        return wxEmptyString;

    const wxString header( wxS( "@package" ) );

    // Extract substring between @package and endsection
    size_t strstart = data.find( header );

    if( strstart == wxString::npos )
        return wxEmptyString;

    strstart += header.Length();
    size_t strend = data.find( aEndSection, strstart );

    if( strend == wxString::npos )
        return wxEmptyString;

    // Remove empty line if any
    while( data[strstart] < ' ' )
            strstart++;

    return data.SubString( strstart, strend - 1 );
}


wxString BOM_GENERATOR_HANDLER::getOutputExtension( const wxString& aHeader )
{
    // search header for extension after %O (extension includes '.')
    // looks for output argument of the form `"%O.extension"`
    const wxString outputarg( wxS( "\"%O" ) );

    size_t strstart = aHeader.find( outputarg );

    if( strstart == wxString::npos )
        return wxEmptyString;

    strstart += outputarg.Length();
    size_t strend = aHeader.find( wxS( "\"" ), strstart );

    if( strend == wxString::npos )
        return wxEmptyString;

    return aHeader.SubString( strstart, strend - 1 );
}


wxFileName BOM_GENERATOR_HANDLER::FindFilePath() const
{
    if( m_file.IsAbsolute() && m_file.Exists( wxFILE_EXISTS_REGULAR ) )
    {
        wxLogTrace( BOM_TRACE, wxS( "%s found directly" ), m_file.GetFullPath() );
        return m_file;
    }

    wxFileName test( PATHS::GetUserPluginsPath(), m_file.GetName(), m_file.GetExt() );

    if( test.Exists( wxFILE_EXISTS_REGULAR ) )
    {
        wxLogTrace( BOM_TRACE, wxS( "%s found in user plugins path %s" ), m_file.GetFullName(),
                    PATHS::GetUserPluginsPath() );
        return test;
    }

    test = wxFileName( PATHS::GetStockPluginsPath(), m_file.GetName(), m_file.GetExt() );

    if( test.Exists( wxFILE_EXISTS_REGULAR ) )
    {
        wxLogTrace( BOM_TRACE, wxS( "%s found in stock plugins path %s" ), m_file.GetFullName(),
                    PATHS::GetStockPluginsPath() );
        return test;
    }

    wxLogTrace( BOM_TRACE, wxS( "Could not find %s (checked %s, %s)" ), m_file.GetFullName(),
                PATHS::GetUserPluginsPath(), PATHS::GetStockPluginsPath() );

    return m_file;
}
