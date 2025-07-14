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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef RAII_H
#define RAII_H

#include <wx/window.h>


/*
 * Exception-safe (and 'return' safe) scoped handlers following the "resource allocation is
 * initialization" pattern.
 */


// Exception-safe method for nulling a pointer
class NULLER
{
public:
    NULLER( void*& aPtr ) :
            m_what( aPtr )
    {}

    ~NULLER()
    {
        m_what = nullptr;
    }

private:
    void*&  m_what;
};


// Temporarily un-freeze a window, and then re-freeze on destruction
class WINDOW_THAWER
{
public:
    WINDOW_THAWER( wxWindow* aWindow )
    {
        m_window = aWindow;
        m_freezeCount = 0;

        while( m_window->IsFrozen() )
        {
            m_window->Thaw();
            m_freezeCount++;
        }
    }

    ~WINDOW_THAWER()
    {
        while( m_freezeCount > 0 )
        {
            m_window->Freeze();
            m_freezeCount--;
        }
    }

protected:
    wxWindow* m_window;
    int       m_freezeCount;
};


/// Temporarily disable a window, and then re-enable on destruction.
class WINDOW_DISABLER
{
public:
    WINDOW_DISABLER( wxWindow* aWindow ) :
            m_win( aWindow )
    {
        if( m_win )
            m_win->Disable();
    }

    ~WINDOW_DISABLER()
    {
        if( m_win )
        {
            m_win->Enable();
            m_win->Raise(); // let's focus back on the parent window
        }
    }

    void SuspendForTrueModal()
    {
        if( m_win )
            m_win->Enable();
    }

    void ResumeAfterTrueModal()
    {
        if( m_win )
            m_win->Disable();
    }

private:
    wxWindow* m_win;
};


#endif  // RAII_H