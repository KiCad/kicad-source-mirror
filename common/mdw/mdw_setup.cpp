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

#include <mdw/client/mdw_library_sources.h>

#include <wx/arrstr.h>



namespace
{

/// aLocator with every query item of key aKey replaced by aItem, or dropped when aItem is empty
wxString replaceLocatorItem( const wxString& aLocator, const wxString& aKey, const wxString& aItem )
{
    wxString base = aLocator.BeforeFirst( '#' );
    wxString fragment = aLocator.Mid( base.length() );
    wxString query = base.Contains( '?' ) ? base.AfterFirst( '?' ) : wxString();

    wxString result = base.BeforeFirst( '?' );
    wxString sep = wxS( "?" );
    bool     replaced = false;

    for( const wxString& pair : wxSplit( query, '&' ) )
    {
        if( pair.IsEmpty() )
            continue;

        const bool ours = pair.BeforeFirst( '=' ) == aKey;
        replaced |= ours;

        if( ours && aItem.IsEmpty() )
            continue;

        result << sep << ( ours ? aItem : pair );
        sep = wxS( "&" );
    }

    // The text is the source's identity, so it is rewritten only when it had the item
    if( !replaced && aItem.IsEmpty() )
        return aLocator;

    if( !replaced )
        result << sep << aItem;

    return result + fragment;
}

} // namespace


wxString MDW_SETUP::LocatorWithItem( const wxString& aLocator, const wxString& aItem )
{
    return replaceLocatorItem( aLocator, aItem.BeforeFirst( '=' ), aItem );
}


wxString MDW_SETUP::LocatorWithoutItem( const wxString& aLocator, const wxString& aKey )
{
    if( aKey.IsEmpty() )
        return aLocator;

    return replaceLocatorItem( aLocator, aKey, wxEmptyString );
}

