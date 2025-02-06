/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2023 jean-pierre.charras
 * Copyright (C) Kicad Developers, see AUTHORS.txt for contributors.
 *
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_PRINTER_LIST_H
#define PANEL_PRINTER_LIST_H

#include <panel_printer_list_base.h>


class PANEL_PRINTER_LIST : public PANEL_PRINTER_LIST_BASE
{
public:
    PANEL_PRINTER_LIST( wxWindow* aParent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxSize( 378,109 ), long style = wxTAB_TRAVERSAL,
                        const wxString& name = wxEmptyString );

    ~PANEL_PRINTER_LIST();

    /// @return the selected printer name or a empty name
    /// for the default printer
    wxString GetSelectedPrinterName();

    /// @return false if no printer in list
    bool AsPrintersAvailable();

private:
	void onPrinterChoice( wxCommandEvent& event ) override;

    // The printer name selected in this dialog
    // static to store the choice during a session
    static wxString m_selectedPrinterName;
    // The default printer name selected by the OS
    wxString m_defaultPrinterName;
    // The list of available printer names
    wxArrayString m_printer_list;
};

#endif
