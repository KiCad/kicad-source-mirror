/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <git/kigit_driver_registry.h>
#include <git/kicad_git_common.h>

#include <git2.h>
#include <git2/sys/merge.h>

#include <trace_helpers.h>

#include <wx/log.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>


namespace KIGIT
{

namespace
{

struct KICAD_DRIVER
{
    git_merge_driver base;
    std::string      name;        // owns the c-string libgit2 borrows
    MERGE_APPLY_FN   apply;
};


std::mutex                                              g_registryMutex;
std::map<std::string, MERGE_APPLY_FN>                   g_appliers;
std::map<std::string, std::unique_ptr<KICAD_DRIVER>>    g_drivers;

int trampolineApply( git_merge_driver* aSelf, const char** aPathOut, uint32_t* aModeOut,
                     git_buf* aMergedOut, const char* /*aFilterName*/,
                     const git_merge_driver_source* aSrc )
{
    KICAD_DRIVER* driver = reinterpret_cast<KICAD_DRIVER*>( aSelf );

    if( !driver || !driver->apply )
        return -1;

    unsigned int mode = 0;
    int          rc   = driver->apply( aSrc, aPathOut, &mode, aMergedOut );

    if( aModeOut )
        *aModeOut = mode;

    return rc;
}

} // namespace


bool RegisterMergeDriver( const char* aName, MERGE_APPLY_FN aApply )
{
    if( !aName || !aApply )
        return false;

    std::lock_guard<std::mutex> lock( g_registryMutex );

    std::string name( aName );

    if( g_appliers.count( name ) )
        return true;   // already registered — idempotent

    auto driver = std::make_unique<KICAD_DRIVER>();
    driver->base.version    = GIT_MERGE_DRIVER_VERSION;
    driver->base.initialize = nullptr;
    driver->base.shutdown   = nullptr;
    driver->base.apply      = trampolineApply;
    driver->name            = name;
    driver->apply           = aApply;

    int rc = git_merge_driver_register( driver->name.c_str(),
                                        reinterpret_cast<git_merge_driver*>( driver.get() ) );

    if( rc != 0 )
    {
        wxLogTrace( traceGit, "git_merge_driver_register('%s') failed: %s",
                    name, KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    g_appliers[name] = aApply;
    g_drivers[name]  = std::move( driver );   // owns the full KICAD_DRIVER

    return true;
}

} // namespace KIGIT
