/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file python_console_frame.h
 */

#ifndef PYTHON_CONSOLE_FRAME_H_
#define PYTHON_CONSOLE_FRAME_H_

#if defined(KICAD_SCRIPTING) || defined(KICAD_SCRIPTING_WXPYTHON)
#include <python_scripting.h>
#endif


/**
 * Class PYTHON_CONSOLE_FRAME is a simple derived class from wxMiniFrame
 * to handle the scripting python console
 */
class PYTHON_CONSOLE_FRAME : public wxMiniFrame
{
private:
    static wxSize m_frameSize;   ///< The size of the frame, stored during a session
    static wxPoint m_framePos;   ///< The position of the frame, stored during a session

public:

    PYTHON_CONSOLE_FRAME( wxWindow* aParent, const wxString& aFramenameId )
        : wxMiniFrame( aParent, wxID_ANY, wxT("Python console"), wxDefaultPosition, wxDefaultSize,
                       wxCAPTION|wxCLOSE_BOX|wxRESIZE_BORDER|wxFRAME_FLOAT_ON_PARENT,
                       aFramenameId )
    {
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

#if defined(KICAD_SCRIPTING_WXPYTHON)
        wxWindow * pythonPanel = CreatePythonShellWindow( this );
        sizer->Add( pythonPanel, 1, wxEXPAND | wxALL, 5 );
#endif
        SetSizer( sizer );
        SetMinSize( wxSize( 400, 200 ) );

        if( m_frameSize.x <= 0 || m_frameSize.y <= 0 )
            SetSize( wxSize( 600, 300 ) );
        else
            SetSize( m_frameSize );

        if( m_framePos.x == 0 && m_framePos.y == 0 )
            Centre();
        else
            SetPosition( m_framePos );

        Layout();

        // Connect Events
        this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PYTHON_CONSOLE_FRAME::OnClose ) );
    }

    ~PYTHON_CONSOLE_FRAME()
    {
        // Disconnect Events
        this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PYTHON_CONSOLE_FRAME::OnClose ) );
    }

private:

    void  OnClose( wxCloseEvent& event )
    {
        if( !IsIconized() )
        {
            m_frameSize = GetSize();
            m_framePos = GetPosition();
        }

        event.Skip();
    }
};

#endif    // PYTHON_CONSOLE_FRAME_H_
