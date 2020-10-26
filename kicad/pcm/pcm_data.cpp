/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcm_data.h"


template <typename T>
void to_optional( const json& j, const char* key, boost::optional<T>& dest )
{
    if( j.contains( key ) )
    {
        T tmp;
        j.at( key ).get_to( tmp );
        dest.emplace( tmp );
    }
}


void to_json( json& j, const PACKAGE_VERSION& v )
{
    j = json{ { "version", v.version },
              { "status", v.status },
              { "kicad_version", v.kicad_version } };

    if( v.version_epoch )
        j["version_epoch"] = v.version_epoch.get();

    if( v.download_url )
        j["download_url"] = v.download_url.get();

    if( v.download_sha256 )
        j["download_sha256"] = v.download_sha256.get();

    if( v.download_size )
        j["download_size"] = v.download_size.get();

    if( v.install_size )
        j["install_size"] = v.install_size.get();

    if( v.platforms.size() > 0 )
        nlohmann::to_json( j["platforms"], v.platforms );

    if( v.kicad_version_max )
        j["kicad_version_max"] = v.kicad_version_max.get();
}


void from_json( const json& j, PACKAGE_VERSION& v )
{
    j.at( "version" ).get_to( v.version );
    j.at( "status" ).get_to( v.status );
    j.at( "kicad_version" ).get_to( v.kicad_version );

    to_optional( j, "version_epoch", v.version_epoch );
    to_optional( j, "download_url", v.download_url );
    to_optional( j, "download_sha256", v.download_sha256 );
    to_optional( j, "download_size", v.download_size );
    to_optional( j, "install_size", v.install_size );
    to_optional( j, "kicad_version_max", v.kicad_version_max );

    if( j.contains( "platforms" ) )
        j.at( "platforms" ).get_to( v.platforms );
}


void to_json( json& j, const PCM_PACKAGE& p )
{
    j = json{ { "name", p.name },
              { "description", p.description },
              { "description_full", p.description_full },
              { "identifier", p.identifier },
              { "type", p.type },
              { "author", p.author },
              { "license", p.license },
              { "resources", p.resources },
              { "versions", p.versions } };

    if( p.maintainer )
        j["maintainer"] = p.maintainer.get();

    if( p.tags.size() > 0 )
        j["tags"] = p.tags;
}


void from_json( const json& j, PCM_PACKAGE& p )
{
    j.at( "name" ).get_to( p.name );
    j.at( "description" ).get_to( p.description );
    j.at( "description_full" ).get_to( p.description_full );
    j.at( "identifier" ).get_to( p.identifier );
    j.at( "type" ).get_to( p.type );
    j.at( "author" ).get_to( p.author );
    j.at( "license" ).get_to( p.license );
    j.at( "resources" ).get_to( p.resources );
    j.at( "versions" ).get_to( p.versions );

    to_optional( j, "maintainer", p.maintainer );

    if( j.contains( "tags" ) )
        j.at( "tags" ).get_to( p.tags );
}


void to_json( json& j, const PCM_RESOURCE_REFERENCE& r )
{
    j = json{ { "url", r.url }, { "update_timestamp", r.update_timestamp } };

    if( r.sha256 )
        j["sha256"] = r.sha256.get();
}


void from_json( const json& j, PCM_RESOURCE_REFERENCE& r )
{
    j.at( "url" ).get_to( r.url );
    j.at( "update_timestamp" ).get_to( r.update_timestamp );

    to_optional(j, "sha256", r.sha256 );
}


void to_json( json& j, const PCM_REPOSITORY& r )
{
    j = json{ { "name", r.name }, { "packages", r.packages } };

    if( r.resources )
        j["resources"] = r.resources.get();

    if( r.manifests )
        j["manifests"] = r.manifests.get();

    if( r.maintainer )
        j["maintainer"] = r.maintainer.get();
}


void from_json( const json& j, PCM_REPOSITORY& r )
{
    j.at( "name" ).get_to( r.name );
    j.at( "packages" ).get_to( r.packages );

    to_optional(j, "resources", r.resources );
    to_optional(j, "manifests", r.manifests );
    to_optional(j, "maintainer", r.maintainer );
}
