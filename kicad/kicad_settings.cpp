/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "kicad_settings.h"
#include "nlohmann/json.hpp"
#include <settings/parameters.h>


///! Update the schema version whenever a migration is required
const int kicadSchemaVersion = 0;


KICAD_SETTINGS::KICAD_SETTINGS() :
        APP_SETTINGS_BASE( "kicad", kicadSchemaVersion ), m_LeftWinWidth( 200 )
{
    m_params.emplace_back( new PARAM<int>( "appearance.left_frame_width", &m_LeftWinWidth, 200 ) );

    m_params.emplace_back(
            new PARAM_LIST<wxString>( "system.open_projects", &m_OpenProjects, {} ) );

#ifdef PCM
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "pcm.repositories",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const auto& pair : m_PcmRepositories )
                {
                    js.push_back( nlohmann::json( { { "name", pair.first.ToUTF8() },
                                                    { "url", pair.second.ToUTF8() } } ) );

                return js;
            },
            [&]( const nlohmann::json aObj )
            {
                m_PcmRepositories.clear();

                if( !aObj.is_array() )
                    return;

                for( const auto& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    m_PcmRepositories.emplace_back(
                            std::make_pair( wxString( entry["name"].get<std::string>() ),
                                            wxString( entry["url"].get<std::string>() ) ) );
                }
            },
            R"([
                {
                    "name": "KiCad official repository",
                    "url": "https://repository.kicad.org/repository.json"
                }
            ])"_json ) );

    m_params.emplace_back(
            new PARAM<wxString>( "pcm.last_download_dir", &m_PcmLastDownloadDir, "" ) );
#endif
}


bool KICAD_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<int>( aCfg, "LeftWinWidth", "appearance.left_frame_width" );

    return ret;
}
