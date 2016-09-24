/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  status_text_reporter.h
 * @brief
 */

#ifndef STATUS_TEXT_REPORTER_H_
#define STATUS_TEXT_REPORTER_H_

#include <reporter.h>
 
/**
 * Class STATUS_TEXT_REPORTER
 * is a wrapper for reporting to a wxString in a wxFrame status text.
 */
class STATUS_TEXT_REPORTER : public REPORTER
{
private:
    wxStatusBar *m_parentStatusBar;
    int          m_position;
    bool         m_hasMessage;

public:
    STATUS_TEXT_REPORTER( wxStatusBar* aParentStatusBar, int aPosition = 0 ) :
        REPORTER(),
        m_parentStatusBar( aParentStatusBar ), m_position( aPosition )
    {
        m_hasMessage = false;
    }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_UNDEFINED ) override
    {
        if( !aText.IsEmpty() )
            m_hasMessage = true;

        if( m_parentStatusBar )
            m_parentStatusBar->SetStatusText( aText, m_position );

        return *this;
    }

    bool HasMessage() const { return m_hasMessage; }
};


#endif // STATUS_TEXT_REPORTER_H_
