/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <textentry_tricks.h>
#include <dialog_shim.h>

bool TEXTENTRY_TRICKS::isCtrl( int aChar, const wxKeyEvent& e )
{
    return e.GetKeyCode() == aChar && e.ControlDown() && !e.AltDown() &&
            !e.ShiftDown() && !e.MetaDown();
}


bool TEXTENTRY_TRICKS::isShiftCtrl( int aChar, const wxKeyEvent& e )
{
    return e.GetKeyCode() == aChar && e.ControlDown() && !e.AltDown() &&
            e.ShiftDown() && !e.MetaDown();
}


void TEXTENTRY_TRICKS::OnCharHook( wxTextEntry* aTextEntry, wxKeyEvent& aEvent )
{
    if( isCtrl( 'X', aEvent ) )
    {
        aTextEntry->Cut();
    }
    else if( isCtrl( 'C', aEvent ) )
    {
        aTextEntry->Copy();
    }
    else if( isCtrl( 'V', aEvent ) )
    {
        aTextEntry->Paste();
    }
    else if( aEvent.GetKeyCode() == WXK_BACK )
    {
        long start, end;
        aTextEntry->GetSelection( &start, &end );

        if( start > end )
        {
            aTextEntry->Remove( start, end );
            aTextEntry->SetInsertionPoint( start );
        }
        else if (start == end && start > 0 )
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
    }
    else
    {
        aEvent.Skip();
    }
}


