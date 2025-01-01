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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/event.h>
#include <wx/gdicmn.h>
#include <textentry_tricks.h>
#include <dialog_shim.h>


void TEXTENTRY_TRICKS::OnCharHook( wxTextEntry* aTextEntry, wxKeyEvent& aEvent )
{
    if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'X' )
    {
        aTextEntry->Cut();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'C' )
    {
        aTextEntry->Copy();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'V' )
    {
        aTextEntry->Paste();
    }
    else if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'A' )
    {
        aTextEntry->SelectAll();
    }
    else if( aEvent.GetKeyCode() == WXK_BACK )
    {
        long start, end;
        aTextEntry->GetSelection( &start, &end );

        if( end > start )
        {
            aTextEntry->Remove( start, end );
            aTextEntry->SetInsertionPoint( start );
        }
        else if ( start == end && start > 0 )
        {
            aTextEntry->Remove( start-1, start );
            aTextEntry->SetInsertionPoint( start-1 );
        }
    }
    else if( aEvent.GetKeyCode() == WXK_DELETE )
    {
        long start, end;
        aTextEntry->GetSelection( &start, &end );

        if( end > start )
        {
            aTextEntry->Remove( start, end );
            aTextEntry->SetInsertionPoint( start );
        }
        else if( start == end && start < aTextEntry->GetLastPosition() )
        {
            aTextEntry->Remove( start, start+1 );
        }
    }
    else
    {
        aEvent.Skip();
    }
}


