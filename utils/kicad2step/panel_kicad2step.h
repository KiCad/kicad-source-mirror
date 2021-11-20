/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file panel_kicad2step.h
 * Declare the main PCB object.
 */

#ifndef PANEL_KICAD2STEP_H
#define PANEL_KICAD2STEP_H

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

#include "kicad2step.h"

class PANEL_KICAD2STEP: public wxPanel
{
public:
    PANEL_KICAD2STEP( wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxSize( 500,300 ),
                      long style = wxTAB_TRAVERSAL );

    /**
     * Run the KiCad to STEP converter.
     */
    int RunConverter();

    /**
     * Add a message to m_tcMessages.
     */
    void AppendMessage( const wxString& aMessage );

    KICAD2MCAD_PRMS m_params;

    bool m_error;
    bool m_fail;

private:
    wxTextCtrl* m_tcMessages;
};

#endif      // #ifndef PANEL_KICAD2STEP_H
