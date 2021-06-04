/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_FP_EDITOR_DEFAULTS_H
#define PANEL_FP_EDITOR_DEFAULTS_H

#include <panel_fp_editor_defaults_base.h>
#include <board_design_settings.h>
#include <widgets/unit_binder.h>

class FOOTPRINT_EDIT_FRAME;


class PANEL_FP_EDITOR_DEFAULTS : public PANEL_FP_EDITOR_DEFAULTS_BASE
{
public:
    PANEL_FP_EDITOR_DEFAULTS( FOOTPRINT_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent );
    ~PANEL_FP_EDITOR_DEFAULTS() override;

private:
    virtual void OnAddTextItem( wxCommandEvent& event ) override;
    virtual void OnDeleteTextItem( wxCommandEvent& event ) override;

    bool Show( bool aShow ) override;

    int getGridValue( int aRow, int aCol );

    bool validateData();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    BOARD_DESIGN_SETTINGS   m_brdSettings;
    FOOTPRINT_EDIT_FRAME*   m_frame;
    PAGED_DIALOG*           m_parent;
    bool                    m_firstShow = true;
};




#endif // PANEL_FP_EDITOR_DEFAULTS_H

