#include "io_mgr.h"
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <unordered_set>
#include <io_mgr.h>
#include <ki_exception.h>
#include <string_utf8_map.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/translation.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>


#define FMT_UNIMPLEMENTED wxT( "Plugin \"%s\" does not implement the \"%s\" function." )
#define NOT_IMPLEMENTED( aCaller )                                          \
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,                    \
                                      PluginName(),                         \
                                      wxString::FromUTF8( aCaller ) ) );


PLUGIN_FILE_DESC PLUGIN::GetBoardFileDesc() const
{
    return PLUGIN_FILE_DESC( wxEmptyString, {} );
}


PLUGIN_FILE_DESC PLUGIN::GetFootprintFileDesc() const
{
    return PLUGIN_FILE_DESC( wxEmptyString, {} );
}


PLUGIN_FILE_DESC PLUGIN::GetFootprintLibDesc() const
{
    return PLUGIN_FILE_DESC( wxEmptyString, {} );
}


bool PLUGIN::CanReadBoard( const wxString& aFileName ) const
{
    const std::vector<std::string>& exts = GetBoardFileDesc().m_FileExtensions;

    wxString fileExt = wxFileName( aFileName ).GetExt().MakeLower();

    for( const std::string& ext : exts )
    {
        if( fileExt == wxString( ext ).Lower() )
            return true;
    }

    return false;
}


bool PLUGIN::CanReadFootprint( const wxString& aFileName ) const
{
    const std::vector<std::string>& exts = GetFootprintFileDesc().m_FileExtensions;

    wxString fileExt = wxFileName( aFileName ).GetExt().MakeLower();

    for( const std::string& ext : exts )
    {
        if( fileExt == wxString( ext ).Lower() )
            return true;
    }

    return false;
}


bool PLUGIN::CanReadFootprintLib( const wxString& aFileName ) const
{
    const PLUGIN_FILE_DESC& desc = GetFootprintLibDesc();

    if( desc.m_IsFile )
    {
        const std::vector<std::string>& exts = desc.m_FileExtensions;

        wxString fileExt = wxFileName( aFileName ).GetExt().MakeLower();

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


BOARD* PLUGIN::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                          const STRING_UTF8_MAP* aProperties, PROJECT* aProject,
                          PROGRESS_REPORTER* aProgressReporter )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


std::vector<FOOTPRINT*> PLUGIN::GetImportedCachedLibraryFootprints()
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::SaveBoard( const wxString& aFileName, BOARD* aBoard, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                 bool aBestEfforts, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::PrefetchLib( const wxString&, const STRING_UTF8_MAP* )
{
}


FOOTPRINT* PLUGIN::ImportFootprint( const wxString& aFootprintPath, wxString& aFootprintNameOut,
                                    const STRING_UTF8_MAP* aProperties )
{
    wxArrayString footprintNames;

    FootprintEnumerate( footprintNames, aFootprintPath, true, aProperties );

    if( footprintNames.empty() )
        return nullptr;

    if( footprintNames.size() > 1 )
    {
        wxLogWarning( _( "Selected file contains multiple footprints. Only the first one will be "
                         "imported." ) );
    }

    aFootprintNameOut = footprintNames.front();

    return FootprintLoad( aFootprintPath, aFootprintNameOut, false, aProperties );
}


const FOOTPRINT* PLUGIN::GetEnumeratedFootprint( const wxString& aLibraryPath,
                                                 const wxString& aFootprintName,
                                                 const STRING_UTF8_MAP* aProperties )
{
    // default implementation
    return FootprintLoad( aLibraryPath, aFootprintName, false, aProperties );
}


bool PLUGIN::FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                              const STRING_UTF8_MAP* aProperties )
{
    // default implementation
    return FootprintLoad( aLibraryPath, aFootprintName, true, aProperties ) != nullptr;
}


FOOTPRINT* PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                  bool  aKeepUUID, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                            const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                              const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const STRING_UTF8_MAP* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


bool PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PLUGIN::FootprintLibOptions( STRING_UTF8_MAP* aListToAppendTo ) const
{
    // disable all these in another couple of months, after everyone has seen them:
#if 1
    (*aListToAppendTo)["debug_level"] = UTF8( _( "Enable <b>debug</b> logging for Footprint*() "
                                                 "functions in this PLUGIN." ) );

    (*aListToAppendTo)["read_filter_regex"] = UTF8( _( "Regular expression <b>footprint name</b> "
                                                       "filter." ) );

    (*aListToAppendTo)["enable_transaction_logging"] = UTF8( _( "Enable transaction logging. The "
                                                                "mere presence of this option "
                                                                "turns on the logging, no need to "
                                                                "set a Value." ) );

    (*aListToAppendTo)["username"] = UTF8( _( "User name for <b>login</b> to some special library "
                                              "server." ) );

    (*aListToAppendTo)["password"] = UTF8( _( "Password for <b>login</b> to some special library "
                                              "server." ) );
#endif

#if 1
    // Suitable for a C++ to python PLUGIN::Footprint*() adapter, move it to the adapter
    // if and when implemented.
    (*aListToAppendTo)["python_footprint_plugin"] = UTF8( _( "Enter the python module which "
                                                             "implements the PLUGIN::Footprint*() "
                                                             "functions." ) );
#endif
}


bool PLUGIN::fileStartsWithPrefix( const wxString& aFilePath, const wxString& aPrefix,
                                   bool aIgnoreWhitespace )
{
    wxFileInputStream input( aFilePath );

    if( input.IsOk() && !input.Eof() )
    {
        // Find first non-empty line
        wxTextInputStream text( input );
        wxString          line = text.ReadLine();

        if( aIgnoreWhitespace )
        {
            while( line.IsEmpty() )
                line = text.ReadLine().Trim( false /*trim from left*/ );
        }

        if( line.StartsWith( aPrefix ) )
            return true;
    }

    return false;
}


bool PLUGIN::fileStartsWithBinaryHeader( const wxString& aFilePath, const std::vector<uint8_t>& aHeader )
{
    wxFileInputStream input( aFilePath );

    if( input.IsOk() && !input.Eof() )
    {
        if( input.GetLength() < aHeader.size() )
            return false;

        std::vector<uint8_t> parsedHeader(aHeader.size());

        if (!input.ReadAll(parsedHeader.data(), parsedHeader.size()))
            return false;

        return parsedHeader == aHeader;
    }

    return false;
}

