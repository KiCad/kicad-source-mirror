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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MDW_CONNECTION_PROMPT_H
#define MDW_CONNECTION_PROMPT_H

#include <mdw/client/mdw_client_types.h>

#include <functional>

class wxWindow;


/**
 * Ask the user for what aStatus needs: trust in the host key of TRUST_NEEDED, or credentials for
 * SIGN_IN_NEEDED in the form the backend sent. aRetry runs with the answer, possibly after this returns while
 * the keychain asks to be unlocked, and never when the user declines or the phase needs nothing from them. The
 * parent may close while a prompt is up.
 */
void MdwResolveConnection( wxWindow* aParent, const MDW_CONNECTION_STATUS& aStatus,
                           std::function<void( const MDW_CONNECTION_ANSWER& )> aRetry );

#endif // MDW_CONNECTION_PROMPT_H
