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

class PAGED_DIALOG;


class PANEL_FP_EDITOR_DEFAULTS : public PANEL_FP_EDITOR_DEFAULTS_BASE
{
public:
    PANEL_FP_EDITOR_DEFAULTS( wxWindow* aParent, EDA_BASE_FRAME* aUnitsProvider );
    ~PANEL_FP_EDITOR_DEFAULTS() override;

private:
    virtual void OnAddTextItem( wxCommandEvent& event ) override;
    virtual void OnDeleteTextItem( wxCommandEvent& event ) override;

    bool Show( bool aShow ) override;

    int getGridValue( int aRow, int aCol );

    bool validateData();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    EDA_UNITS     m_units = EDA_UNITS::MILLIMETRES;
    PAGED_DIALOG* m_parent;
    bool          m_firstShow = true;
};




#endif // PANEL_FP_EDITOR_DEFAULTS_H

