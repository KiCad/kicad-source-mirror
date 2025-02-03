/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include "dialog_print_base.h"


class SCH_EDIT_FRAME;
class SCH_SCREEN;


class DIALOG_PRINT : public DIALOG_PRINT_BASE
{
public:
    DIALOG_PRINT( SCH_EDIT_FRAME* aParent );
    ~DIALOG_PRINT() override;

protected:
    void OnOutputChoice( wxCommandEvent& event ) override;
    void OnUseColorThemeChecked( wxCommandEvent& event ) override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnPageSetup( wxCommandEvent& event ) override;
    void OnPrintPreview( wxCommandEvent& event ) override;

    void SavePrintOptions();

private:
    SCH_EDIT_FRAME* m_parent;
};


