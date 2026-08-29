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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "http_lib_fake_server.h"

#include <ranges>


void HTTP_LIB_FAKE_SERVER::SetResponse( const std::string& aUrl, RESPONSE aResponse )
{
    std::lock_guard lock( m_mutex );
    m_responses[aUrl] = std::move( aResponse );
}


int HTTP_LIB_FAKE_SERVER::RequestCount( const std::string& aUrl ) const
{
    std::lock_guard lock( m_mutex );
    return static_cast<int>( std::ranges::count( m_requests, aUrl ) );
}


HTTP_LIB_CONNECTION::RETRIEVER HTTP_LIB_FAKE_SERVER::MakeRetriever()
{
    return  [this]( const std::string& aUrl, int& aStatusCode, std::string& aBody, std::string& )
            {
                std::lock_guard lock( m_mutex );
                m_requests.push_back( aUrl );

                if( auto it = m_responses.find( aUrl ); it == m_responses.end() )
                {
                    aStatusCode = 404;
                    aBody.clear();
                }
                else
                {
                    aStatusCode = it->second.statusCode;
                    aBody = it->second.body;
                }

                return true;
            };
}


void HTTP_LIB_FAKE_SERVER::InstallStandardLibrary( const std::string& aRootUrl )
{
    SetResponse( aRootUrl, { 200, R"({"categories": "categories.json", "parts": "parts"})" } );

    SetResponse( aRootUrl + "categories.json", { 200, R"([
  { "id": "1", "name": "Resistors",  "description": "Resistors" },
  { "id": "2", "name": "Capacitors", "description": "Capacitors" }
])" } );

    SetResponse( aRootUrl + "parts/category/1.json", { 200, R"([
  { "id": "res-10k",  "name": "R_10K",  "description": "10 kOhms resistor" },
  { "id": "res-100k", "name": "R_100K", "description": "100 kOhms resistor" }
])" } );

    SetResponse( aRootUrl + "parts/category/2.json", { 200, R"([
  { "id": "cap-100n", "name": "C_100n", "description": "100 nF capacitor" }
])" } );

    SetResponse( aRootUrl + "parts/res-10k.json", { 200, R"({
  "id": "res-10k",
  "name": "R_10K",
  "exclude_from_bom": "False",
  "exclude_from_board": "False",
  "exclude_from_sim": "False",
  "description": "10 kOhms resistor",
  "keywords": "resistor",
  "footprint_filters": ["Resistor_SMD:R_0603_1608Metric"],
  "fields": {
    "footprint": { "value": "Resistor_SMD:R_0603_1608Metric", "visible": "False" },
    "value":     { "value": "10k" },
    "reference": { "value": "R" }
  }
})" } );

    SetResponse( aRootUrl + "parts/res-100k.json", { 200, R"({
  "id": "res-100k",
  "name": "R_100K",
  "exclude_from_bom": "False",
  "exclude_from_board": "False",
  "exclude_from_sim": "False",
  "description": "100 kOhms resistor",
  "fields": {
    "footprint": { "value": "Resistor_SMD:R_0603_1608Metric", "visible": "False" },
    "value":     { "value": "100k" },
    "reference": { "value": "R" }
  }
})" } );

    SetResponse( aRootUrl + "parts/cap-100n.json", { 200, R"({
  "id": "cap-100n",
  "name": "C_100n",
  "description": "100 nF capacitor",
  "fields": {
    "footprint": { "value": "Capacitor_SMD:C_0603_1608Metric", "visible": "False" },
    "value":     { "value": "100n" },
    "reference": { "value": "C" }
  }
})" } );
}
