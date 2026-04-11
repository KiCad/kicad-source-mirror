/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <advanced_config.h>
#include <pgm_base.h>
#include <thread_pool.h>

static thread_pool* tp = nullptr;
static bool         tp_owned = false;  // True if we created the thread pool ourselves

thread_pool& GetKiCadThreadPool()
{
    if( tp )
        return *tp;

    // If we have a PGM_BASE, use its thread pool
    if( PGM_BASE* pgm = PgmOrNull() )
    {
        tp = &pgm->GetThreadPool();
        tp_owned = false;
        return *tp;
    }

    // Otherwise, we are running in scripting or some other context where we don't have a PGM_BASE
    // so we need to create our own thread pool
    int num_threads = std::max( 0, ADVANCED_CFG::GetCfg().m_MaximumThreads );
    tp = new thread_pool( num_threads );
    tp_owned = true;

    return *tp;
}


void InvalidateKiCadThreadPool()
{
    // If we own the thread pool (scripting context), delete it
    if( tp && tp_owned )
    {
        delete tp;
    }

    tp = nullptr;
    tp_owned = false;
}
