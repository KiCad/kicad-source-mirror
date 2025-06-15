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

#include <wx/version.h>
#include <widgets/app_progress_dialog.h>


APP_PROGRESS_DIALOG::APP_PROGRESS_DIALOG( const wxString& aTitle, const wxString& aMessage,
                                          int aMaximum, wxWindow* aParent,
                                          bool aIndeterminateTaskBarStatus, int aStyle )
        : APP_PROGRESS_DIALOG_BASE( aTitle,
                            aMessage == wxEmptyString ? wxString( wxT( " " ) ) : aMessage,
                            aMaximum, aParent, aStyle ),
          m_appProgressIndicator( aParent, aMaximum ),
          m_indeterminateTaskBarStatus( aIndeterminateTaskBarStatus )

{
    if( m_indeterminateTaskBarStatus )
    {
        m_appProgressIndicator.Pulse();
    }
}


bool APP_PROGRESS_DIALOG::Update( int aValue, const wxString& aNewMsg, bool* aSkip )
{
    if( !m_indeterminateTaskBarStatus )
    {
        m_appProgressIndicator.SetValue( aValue );
    }

    return APP_PROGRESS_DIALOG_BASE::Update( aValue, aNewMsg, aSkip );
}
