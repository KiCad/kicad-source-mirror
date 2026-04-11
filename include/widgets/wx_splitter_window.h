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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_WX_SPLITTER_WINDOW_H
#define KICAD_WX_SPLITTER_WINDOW_H

#include <wx/splitter.h>

class WX_SPLITTER_WINDOW : public wxSplitterWindow
{
public:
    WX_SPLITTER_WINDOW( wxWindow *parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                        long style = wxSP_3D, const wxString& name = wxT( "splitter" ) ) :
        wxSplitterWindow( parent, id, pos, size, style, name ),
        m_minFirstPane( -1 ),
        m_minSecondPane( -1 )
    {
        this->Connect( wxEVT_SIZE, wxSizeEventHandler( WX_SPLITTER_WINDOW::OnSize ) );
    }


    ~WX_SPLITTER_WINDOW() override
    { }

    bool OnSashPositionChange( int newSashPosition ) override;

    void OnSize( wxSizeEvent& aEvent );

    void SetPaneMinimums( int aFirst, int aSecond )
    {
        m_minFirstPane = aFirst;
        m_minSecondPane = aSecond;
    }

private:
    int m_minFirstPane;
    int m_minSecondPane;
};

#endif //KICAD_WX_SPLITTER_WINDOW_H
