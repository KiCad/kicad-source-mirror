/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "mdw_overwritten_parts.h"

#include <kiplatform/io.h>
#include <mdw/client/mdw_file_lock.h>
#include <paths.h>
#include <remote_provider_utils.h>

#include <nlohmann/json.hpp>

#include <wx/ffile.h>
#include <wx/filename.h>



MDW_OVERWRITTEN_PARTS& MDW_OVERWRITTEN_PARTS::Get()
{
    static MDW_OVERWRITTEN_PARTS store(
            wxFileName( PATHS::GetUserSettingsPath(), wxS( "mdw_overwritten_parts.json" ) ).GetFullPath() );
    return store;
}


MDW_OVERWRITTEN_PARTS::MDW_OVERWRITTEN_PARTS( const wxString& aFile ) :
        m_file( aFile )
{
}


MDW_OVERWRITTEN_PARTS::PARTS MDW_OVERWRITTEN_PARTS::read() const
{
    PARTS    parts;
    wxFFile  file;
    wxString content;

    if( !wxFileName::FileExists( m_file ) || !file.Open( m_file, wxS( "rb" ) ) || !file.ReadAll( &content ) )
        return parts;

    nlohmann::json root = nlohmann::json::parse( content.utf8_string(), nullptr, false );

    if( !root.is_object() || !root.contains( "parts" ) || !root["parts"].is_array() )
        return parts;

    for( const nlohmann::json& row : root["parts"] )
    {
        if( !row.is_object() )
            continue;

        wxString marker = RemoteProviderJsonString( row, "copy" );
        wxString item = RemoteProviderJsonString( row, "item" );

        if( !marker.IsEmpty() && !item.IsEmpty() )
            parts.emplace( marker, item );
    }

    return parts;
}


bool MDW_OVERWRITTEN_PARTS::update( const std::function<bool( PARTS& )>& aChange )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    bool                        written = true;

    bool locked = MdwWithFileLock(
            m_file,
            [&]()
            {
                PARTS parts = read();

                if( !aChange( parts ) )
                    return;

                nlohmann::json rows = nlohmann::json::array();

                for( const auto& [marker, item] : parts )
                    rows.push_back( { { "copy", marker.utf8_string() }, { "item", item.utf8_string() } } );

                std::string data = nlohmann::json( { { "parts", rows } } ).dump( 2 );
                written = KIPLATFORM::IO::AtomicWriteFile( m_file, data.data(), data.size() );
            } );

    return locked && written;
}


bool MDW_OVERWRITTEN_PARTS::Mark( const wxString& aMarker, const std::vector<wxString>& aItemIds )
{
    return update(
            [&]( PARTS& aParts )
            {
                const size_t before = aParts.size();

                for( const wxString& item : aItemIds )
                    aParts.emplace( aMarker, item );

                return aParts.size() != before;
            } );
}


bool MDW_OVERWRITTEN_PARTS::Clear( const wxString& aMarker, const wxString& aItemId )
{
    return update(
            [&]( PARTS& aParts )
            {
                return aParts.erase( { aMarker, aItemId } ) > 0;
            } );
}


bool MDW_OVERWRITTEN_PARTS::Has( const wxString& aMarker, const wxString& aItemId ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );

    return read().count( { aMarker, aItemId } ) > 0;
}
