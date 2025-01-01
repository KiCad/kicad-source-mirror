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

#include <widgets/wx_listbox.h>
#include <lib_tree_model_adapter.h>

/*
 * A specialization of wxListBox with support for pinned items.
 */


wxString WX_LISTBOX::GetStringSelection() const
{
    wxString str = wxListBox::GetStringSelection();

    if( str.StartsWith( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() ) )
        str = str.substr( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol().length() );

    return str;
}


bool WX_LISTBOX::SetStringSelection( const wxString& s )
{
    if( wxListBox::SetStringSelection( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + s ) )
        return true;

    return wxListBox::SetStringSelection( s );
}


bool WX_LISTBOX::SetStringSelection( const wxString& s, bool select )
{
    if( wxListBox::SetStringSelection( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + s, select ) )
        return true;

    return wxListBox::SetStringSelection( s, select );
}


wxString WX_LISTBOX::GetBaseString( int n ) const
{
    wxString str = wxListBox::GetString( n );

    if( str.StartsWith( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() ) )
        str = str.substr( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol().length() );

    return str;
}


int WX_LISTBOX::FindString( const wxString& s, bool bCase ) const
{
    int retVal = wxListBox::FindString( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + s, bCase );

    if( retVal == wxNOT_FOUND )
        retVal = wxListBox::FindString( s, bCase );

    return retVal;
}