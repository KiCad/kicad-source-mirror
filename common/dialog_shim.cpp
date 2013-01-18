
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <dialog_shim.h>


DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style, const wxString& name ) :
    wxDialog( aParent, id, title, pos, size, style, name )
{
#if DLGSHIM_USE_SETFOCUS
    Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SHIM::onInit ) );
#endif
}


// our hashtable is an implementation secret, don't need or want it in a header file
#include <hashtables.h>
#include <base_struct.h>        // EDA_RECT
#include <typeinfo>

static RECT_MAP class_map;

bool DIALOG_SHIM::Show( bool show )
{
    bool        ret;
    const char* classname = typeid(*this).name();

    // Show or hide the window.  If hiding, save current position and size.
    // If showing, use previous position and size.
    if( show )
    {
        ret = wxDialog::Show( show );

        // classname is key, returns a zeroed out default EDA_RECT if none existed before.
        EDA_RECT r = class_map[ classname ];

        if( r.GetSize().x != 0 && r.GetSize().y != 0 )
            SetSize( r.GetPosition().x, r.GetPosition().y, r.GetSize().x, r.GetSize().y, 0 );
    }
    else
    {
        // Save the dialog's position & size before hiding, using classname as key
        EDA_RECT  r( wxDialog::GetPosition(), wxDialog::GetSize() );
        class_map[ classname ] = r;

        ret = wxDialog::Show( show );
    }
    return ret;
}


#if DLGSHIM_USE_SETFOCUS

static bool findWindowRecursively( const wxWindowList& children, const wxWindow* wanted )
{
    for( wxWindowList::const_iterator it = children.begin();  it != children.end();  ++it )
    {
        const wxWindow* child = *it;

        if( wanted == child )
            return true;
        else
        {
            if( findWindowRecursively( child->GetChildren(), wanted ) )
                return true;
        }
    }

    return false;
}


static bool findWindowRecursively( const wxWindow* topmost, const wxWindow* wanted )
{
    // wanted may be NULL and that is ok.

    if( wanted == topmost )
        return true;

    return findWindowRecursively( topmost->GetChildren(), wanted );
}


/// Set the focus if it is not already set in a derived constructor to a specific control.
void DIALOG_SHIM::onInit( wxInitDialogEvent& aEvent )
{
    wxWindow* focusWnd = wxWindow::FindFocus();

    // If focusWnd is not already this window or a child of it, then SetFocus().
    // Otherwise the derived class's constructor SetFocus() already to a specific
    // child control.

    if( !findWindowRecursively( this, focusWnd ) )
    {
        // Linux wxGTK needs this to allow the ESCAPE key to close a wxDialog window.
        SetFocus();
    }

    aEvent.Skip();     // derived class's handler should be called too
}
#endif
