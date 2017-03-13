/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef WIDGETS_GAL_OPTIONS_PANEL__H_
#define WIDGETS_GAL_OPTIONS_PANEL__H_

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include <gal/gal_display_options.h>

class INCREMENTAL_TEXT_CTRL;

class GAL_OPTIONS_PANEL: public wxPanel
{
public:

    GAL_OPTIONS_PANEL( wxWindow* aParent, KIGFX::GAL_DISPLAY_OPTIONS& aGalOpts );

    /**
     * Load the panel controls from the given opt
     */
    bool TransferDataToWindow() override;

    /**
     * Read the options set in the UI into the given options object
     */
    bool TransferDataFromWindow() override;

private:

    wxBoxSizer* m_mainSizer;

    wxChoice* m_choiceAntialiasing;
    wxRadioBox* m_gridStyle;
    wxStaticText* l_gridLineWidth;
    wxTextCtrl* m_gridLineWidth;
    wxSpinButton* m_gridLineWidthSpinBtn;
    wxStaticText* l_gridLineWidthUnits;
    wxStaticText* l_gridMinSpacing;
    wxTextCtrl* m_gridMinSpacing;
    wxSpinButton* m_gridMinSpacingSpinBtn;
    wxStaticText* l_gridMinSpacingUnits;

    ///> The GAL options to read/write
    KIGFX::GAL_DISPLAY_OPTIONS& m_galOptions;

    std::unique_ptr<INCREMENTAL_TEXT_CTRL> m_gridSizeIncrementer;
    std::unique_ptr<INCREMENTAL_TEXT_CTRL> m_gridMinSpacingIncrementer;
};


#endif // WIDGETS_GAL_OPTIONS_PANEL__H_
