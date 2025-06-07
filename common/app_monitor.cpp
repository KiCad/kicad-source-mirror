/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <app_monitor.h>
#include <pgm_base.h>
#include <magic_enum.hpp>

#ifdef KICAD_USE_SENTRY
#include <sentry.h>
#endif

using namespace APP_MONITOR;

static std::string GetSentryBreadCrumbType( BREADCRUMB_TYPE aType )
{
    // the only special case due to collisions with defines
    if( aType ==  BREADCRUMB_TYPE::DBG )
        return "debug";

    if( aType == BREADCRUMB_TYPE::ERR )
        return "error";

    std::string ret( magic_enum::enum_name( aType ) );

    std::transform( ret.begin(), ret.end(), ret.begin(),
                    []( unsigned char c )
                    {
                        return std::tolower( c );
                    } );

    return ret;
}


static std::string GetSentryBreadCrumbLevel( BREADCRUMB_LEVEL aLevel )
{
    // the only special case due to collisions with defines
    if( aLevel == BREADCRUMB_LEVEL::DBG )
        return "debug";

    if( aLevel == BREADCRUMB_LEVEL::ERR )
        return "error";

    std::string ret( magic_enum::enum_name( aLevel ) );

    std::transform( ret.begin(), ret.end(), ret.begin(),
                    []( unsigned char c )
                    {
                        return std::tolower( c );
                    } );

    return ret;
}


void APP_MONITOR::AddBreadcrumb( BREADCRUMB_TYPE aType, const wxString& aMsg, const wxString& aCategory,
                               BREADCRUMB_LEVEL aLevel )
{
#ifdef KICAD_USE_SENTRY
    if( !Pgm().IsSentryOptedIn() )
    {
        return;
    }

    std::string    type = GetSentryBreadCrumbType( aType );
    sentry_value_t crumb = sentry_value_new_breadcrumb( type.c_str(), aMsg.c_str() );

    sentry_value_set_by_key( crumb, "category", sentry_value_new_string( aCategory.c_str() ) );

    if( aType == BREADCRUMB_TYPE::ERR )
    {
        std::string level = GetSentryBreadCrumbLevel( aLevel );
        sentry_value_set_by_key( crumb, "level", sentry_value_new_string( level.c_str() ) );
    }

    sentry_add_breadcrumb( crumb );
#endif
}


void APP_MONITOR::AddNavigationBreadcrumb( const wxString& aMsg, const wxString& aCategory )
{
#ifdef KICAD_USE_SENTRY
    AddBreadcrumb( BREADCRUMB_TYPE::NAVIGATION, aMsg, aCategory, BREADCRUMB_LEVEL::INFO );
#endif
}


void APP_MONITOR::AddTransactionBreadcrumb( const wxString& aMsg, const wxString& aCategory )
{
#ifdef KICAD_USE_SENTRY
    AddBreadcrumb( BREADCRUMB_TYPE::TRANSACTION, aMsg, aCategory, BREADCRUMB_LEVEL::INFO );
#endif
}