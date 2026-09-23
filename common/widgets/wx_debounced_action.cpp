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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/wx_debounced_action.h>

#include <utility>


WX_DEBOUNCED_ACTION::WX_DEBOUNCED_ACTION( ACTION_FN aAction, int aDelayMs ) :
        m_action( std::move( aAction ) ),
        m_delayMs( aDelayMs )
{
    wxASSERT_MSG( aDelayMs >= 0, wxT( "A debounce delay cannot be negative" ) );
}


void WX_DEBOUNCED_ACTION::Restart()
{
    // StartOnce() restarts the timer when it is already running, which is what makes the
    // action wait for the caller to stop calling Restart().
    StartOnce( m_delayMs );
}


void WX_DEBOUNCED_ACTION::Cancel()
{
    Stop();
}


void WX_DEBOUNCED_ACTION::Notify()
{
    if( m_action )
        m_action();
}
