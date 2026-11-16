/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "git_config_handler.h"
#include "git_backend.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <trace_helpers.h>
#include <wx/log.h>

GIT_CONFIG_HANDLER::GIT_CONFIG_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}


GIT_CONFIG_HANDLER::~GIT_CONFIG_HANDLER()
{}


GitUserConfig GIT_CONFIG_HANDLER::GetUserConfig()
{
    GitUserConfig local;
    GitUserConfig global;

    KIGIT::ReadAuthorConfig( GetRepo(), local, global );
    return KIGIT::ResolveAuthor( Pgm().GetCommonSettings()->m_Git, local, global );
}
