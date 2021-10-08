/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef HTML_WINDOW_H
#define HTML_WINDOW_H

#include <wx/html/htmlwin.h>

/**
 * Add dark theme support to wxHtmlWindow.
 */
class HTML_WINDOW : public wxHtmlWindow
{
public:
    HTML_WINDOW( wxWindow* aParent, wxWindowID aId=wxID_ANY, const wxPoint& aPos=wxDefaultPosition,
                 const wxSize& aSize=wxDefaultSize, long aStyle=wxHW_DEFAULT_STYLE,
                 const wxString& aName="htmlWindow" );

    bool SetPage( const wxString& aSource ) override;

    /*
     * Notify the HTML window the theme has changed.
     */
    void ThemeChanged();

private:
    void onThemeChanged( wxSysColourChangedEvent &aEvent );

    wxString m_pageSource;
};

#endif /* HTML_WINDOW_H */
