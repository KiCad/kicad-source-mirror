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

#ifndef __APP_PROGRESS_REPORTER
#define __APP_PROGRESS_REPORTER

#include <wx/progdlg.h>
#include <wx/appprogress.h>

#if defined( _WIN32 ) && wxCHECK_VERSION(3,3,0)
// In order to get a dark mode compatible progress dialog on Windows with wx 3.3
#define APP_PROGRESS_DIALOG_BASE wxGenericProgressDialog
#else
#define APP_PROGRESS_DIALOG_BASE wxProgressDialog
#endif

/**
 * wxProgressDialog with the option to also update the application progress on the taskbar
 */
class APP_PROGRESS_DIALOG : public APP_PROGRESS_DIALOG_BASE
{
public:
    APP_PROGRESS_DIALOG( const wxString& aTitle, const wxString& aMessage, int aMaximum = 100,
                         wxWindow* aParent = nullptr, bool aIndeterminateTaskBarStatus = false,
                         int aStyle = wxPD_APP_MODAL | wxPD_AUTO_HIDE );

    virtual bool Update( int aValue, const wxString& aNewMsg = wxEmptyString,
                         bool* aSkip = nullptr ) override;


private:
    wxAppProgressIndicator m_appProgressIndicator;
    bool m_indeterminateTaskBarStatus;
};

#endif
