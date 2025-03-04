/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <unordered_set>

#include <io/io_base.h>
#include <progress_reporter.h>
#include <ki_exception.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>
#include <wx/translation.h>
#include <wx/dir.h>

#define FMT_UNIMPLEMENTED wxT( "IO interface \"%s\" does not implement the \"%s\" function." )
#define NOT_IMPLEMENTED( aCaller )                                       \
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,                 \
                                      GetName(),                         \
                                      wxString::FromUTF8( aCaller ) ) );


wxString IO_BASE::IO_FILE_DESC::FileFilter() const
{
    return wxGetTranslation( m_Description ) + AddFileExtListToFilter( m_FileExtensions );
}


void IO_BASE::CreateLibrary( const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool IO_BASE::DeleteLibrary( const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool IO_BASE::IsLibraryWritable( const wxString& aLibraryPath )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}

void IO_BASE::GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const
{
    // No global options to append
}


bool IO_BASE::CanReadLibrary( const wxString& aFileName ) const
{
    const IO_BASE::IO_FILE_DESC& desc = GetLibraryDesc();

    if( desc.m_IsFile )
    {
        const std::vector<std::string>& exts = desc.m_FileExtensions;

        wxString fileExt = wxFileName( aFileName ).GetExt().Lower();

        for( const std::string& ext : exts )
        {
            if( fileExt == wxString( ext ).Lower() )
                return true;
        }
    }
    else
    {
        wxDir dir( aFileName );

        if( !dir.IsOpened() )
            return false;

        std::vector<std::string>     exts = desc.m_ExtensionsInDir;
        std::unordered_set<wxString> lowerExts;

        for( const std::string& ext : exts )
            lowerExts.emplace( wxString( ext ).MakeLower() );

        wxString filenameStr;

        bool cont = dir.GetFirst( &filenameStr, wxEmptyString, wxDIR_FILES | wxDIR_HIDDEN );

        while( cont )
        {
            wxString ext = wxS( "" );

            int idx = filenameStr.Find( '.', true );

            if( idx != -1 )
                ext = filenameStr.Mid( idx + 1 ).MakeLower();

            if( lowerExts.count( ext ) )
                return true;

            cont = dir.GetNext( &filenameStr );
        }
    }

    return false;
}


void IO_BASE::Report( const wxString& aText, SEVERITY aSeverity )
{
    if( !m_reporter )
        return;

    m_reporter->Report( aText, aSeverity );
}


void IO_BASE::AdvanceProgressPhase()
{
    if( !m_progressReporter )
        return;

    if( !m_progressReporter->KeepRefreshing() )
        THROW_IO_ERROR( _( "Loading file canceled by user." ) );

    m_progressReporter->AdvancePhase();
}
