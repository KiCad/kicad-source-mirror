/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <unordered_set>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <ki_exception.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/translation.h>
#include <wx/dir.h>


#define FMT_UNIMPLEMENTED wxT( "Plugin \"%s\" does not implement the \"%s\" function." )
#define NOT_IMPLEMENTED( aCaller )                                          \
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,                    \
                                      GetName(),                         \
                                      wxString::FromUTF8( aCaller ) ) );


bool PCB_IO::CanReadBoard( const wxString& aFileName ) const
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


bool PCB_IO::CanReadFootprint( const wxString& aFileName ) const
{
    const std::vector<std::string>& exts = GetLibraryFileDesc().m_FileExtensions;

    wxString fileExt = wxFileName( aFileName ).GetExt().MakeLower();

    for( const std::string& ext : exts )
    {
        if( fileExt == wxString( ext ).Lower() )
            return true;
    }

    return false;
}


BOARD* PCB_IO::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                          const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


std::vector<FOOTPRINT*> PCB_IO::GetImportedCachedLibraryFootprints()
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PCB_IO::SaveBoard( const wxString& aFileName, BOARD* aBoard,
                        const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PCB_IO::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                 bool aBestEfforts, const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


FOOTPRINT* PCB_IO::ImportFootprint( const wxString& aFootprintPath, wxString& aFootprintNameOut,
                                    const std::map<std::string, UTF8>* aProperties )
{
    wxArrayString footprintNames;

    FootprintEnumerate( footprintNames, aFootprintPath, true, aProperties );

    if( footprintNames.empty() )
        return nullptr;

    if( footprintNames.size() > 1 )
    {
        wxLogWarning( _( "Selected file contains multiple footprints. Only the first one will be "
                         "imported.\nTo load all footprints, add it as a library using Preferences "
                         "-> Manage Footprint "
                         "Libraries..." ) );
    }

    aFootprintNameOut = footprintNames.front();

    return FootprintLoad( aFootprintPath, aFootprintNameOut, false, aProperties );
}


const FOOTPRINT* PCB_IO::GetEnumeratedFootprint( const wxString& aLibraryPath,
                                                 const wxString& aFootprintName,
                                                 const std::map<std::string, UTF8>* aProperties )
{
    // default implementation
    return FootprintLoad( aLibraryPath, aFootprintName, false, aProperties );
}


bool PCB_IO::FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                              const std::map<std::string, UTF8>* aProperties )
{
    // default implementation
    return FootprintLoad( aLibraryPath, aFootprintName, true, aProperties ) != nullptr;
}


FOOTPRINT* PCB_IO::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                  bool  aKeepUUID, const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PCB_IO::FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                            const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PCB_IO::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                              const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void PCB_IO::GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const
{
    // Get base options first
    IO_BASE::GetLibraryOptions( aListToAppendTo );

    // disable all these in another couple of months, after everyone has seen them:
#if 1
    (*aListToAppendTo)["debug_level"] = UTF8( _( "Enable <b>debug</b> logging for Footprint*() "
                                                 "functions in this PCB_IO." ) );

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
    // Suitable for a C++ to python PCB_IO::Footprint*() adapter, move it to the adapter
    // if and when implemented.
    (*aListToAppendTo)["python_footprint_plugin"] = UTF8( _( "Enter the python module which "
                                                             "implements the PCB_IO::Footprint*() "
                                                             "functions." ) );
#endif
}
