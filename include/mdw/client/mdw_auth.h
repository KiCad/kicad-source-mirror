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

/**
 * @file mdw_auth.h
 * @brief Backend identity and version.
 */

#ifndef MDW_AUTH_H
#define MDW_AUTH_H

#include <mdw/client/mdw_types.h>

#include <wx/string.h>

#include <cstdint>
#include <map>

/// A protocol or backend version. Major must match to load; minor only adds
struct MDW_VERSION
{
    unsigned major = 0;
    unsigned minor = 0;
    unsigned patch = 0;
    wxString full;
};


/// What a backend advertises about itself at handshake
struct MDW_BACKEND_INFO
{
    wxString          backend_id;
    wxString          display_name;
    wxString          description;
    wxString          vendor;
    MDW_VERSION       version;
    MDW_CHECKIN_MODEL checkin_model = MDW_CHECKIN_MODEL::UPLOAD;
};


/// Who a backend signed the user in as
struct MDW_ACCOUNT
{
    wxString user_id;
    wxString display_name;
    wxString email;

    wxString Name() const { return display_name.IsEmpty() ? user_id : display_name; }

    /// Name(), followed by the user id the server signed in when the name shown is another
    wxString NameAndLogin() const
    {
        if( user_id.IsEmpty() || Name() == user_id )
            return Name();

        return wxString::Format( wxS( "%s (%s)" ), Name(), user_id );
    }
};


/// Credential values as the user typed them, keyed by MDW_AUTH_FIELD::field_id
struct MDW_TYPED_CREDENTIALS
{
    std::map<wxString, wxString> fields;
};


#endif // MDW_AUTH_H
