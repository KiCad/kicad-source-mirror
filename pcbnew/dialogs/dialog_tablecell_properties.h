/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_TABLECELL_PROPERTIES_H
#define DIALOG_TABLECELL_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <wx/valnum.h>

#include "dialog_tablecell_properties_base.h"


class PCB_BASE_EDIT_FRAME;
class PCB_TABLE;
class PCB_TABLECELL;
class SCINTILLA_TRICKS;


class DIALOG_TABLECELL_PROPERTIES : public DIALOG_TABLECELL_PROPERTIES_BASE
{
public:
    DIALOG_TABLECELL_PROPERTIES( PCB_BASE_EDIT_FRAME* aParentFrame, PCB_TABLECELL* aCell );
    ~DIALOG_TABLECELL_PROPERTIES();

protected:
    void OnCharHook( wxKeyEvent& aEvt ) override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onHAlignButton( wxCommandEvent &aEvent );
    void onVAlignButton( wxCommandEvent &aEvent );
    void onBorderChecked( wxCommandEvent& aEvent ) override;
    void OnApply( wxCommandEvent& aEvent ) override;

private:
    PCB_BASE_EDIT_FRAME* m_frame;
    PCB_TABLE*           m_table;
    PCB_TABLECELL*       m_cell;

    UNIT_BINDER          m_borderWidth;
    UNIT_BINDER          m_separatorsWidth;
    UNIT_BINDER          m_textHeight;
    UNIT_BINDER          m_textWidth;
    UNIT_BINDER          m_textThickness;

    SCINTILLA_TRICKS*    m_scintillaTricks;
};


#endif //DIALOG_TABLECELL_PROPERTIES_H
