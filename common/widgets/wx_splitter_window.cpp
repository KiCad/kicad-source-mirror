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
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/wx_splitter_window.h>


bool WX_SPLITTER_WINDOW::OnSashPositionChange( int newSashPosition )
{
    if( newSashPosition < m_minFirstPane )
        return false;

    int totalSize;

    if( GetSplitMode() == wxSPLIT_HORIZONTAL )
        totalSize = GetSize().GetHeight();
    else
        totalSize = GetSize().GetWidth();

    if( totalSize - newSashPosition < m_minSecondPane )
        return false;

    return true;
}


void WX_SPLITTER_WINDOW::OnSize( wxSizeEvent& aEvent )
{
    wxSplitterWindow::OnSize( aEvent );

    if( GetSashPosition() < m_minFirstPane )
        SetSashPosition( m_minFirstPane );

    int totalSize;

    if( GetSplitMode() == wxSPLIT_HORIZONTAL )
        totalSize = GetSize().GetHeight();
    else
        totalSize = GetSize().GetWidth();

    if( totalSize - GetSashPosition() < m_minSecondPane )
        SetSashPosition( totalSize - m_minSecondPane );
}
