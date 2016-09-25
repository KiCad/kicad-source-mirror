/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
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

#ifndef DIALOG_EDIT_MODULE_TEXT_H
#define DIALOG_EDIT_MODULE_TEXT_H

#include <dialog_edit_module_text_base.h>
#include <wx/valnum.h>

/*************** **************/
/* class DialogEditModuleText */
/*************** **************/
class DialogEditModuleText : public DialogEditModuleText_base
{
private:
    PCB_BASE_FRAME* m_parent;
    wxDC* m_dc;
    MODULE* m_module;
    TEXTE_MODULE* m_currentText;

    wxFloatingPointValidator<double> m_OrientValidator;
    double m_OrientValue;

public:
    DialogEditModuleText( PCB_BASE_FRAME* aParent, TEXTE_MODULE* aTextMod, wxDC* aDC );
    ~DialogEditModuleText() {};

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void ModuleOrientEvent( wxCommandEvent& event ) override;

    void OnInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }
};

#endif /* DIALOG_EDIT_MODULE_TEXT_H */
