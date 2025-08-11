/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <dialog_sheet_pin_properties_base.h>
#include <widgets/unit_binder.h>


class SCH_SHEET_PIN;
class HTML_MESSAGE_BOX;


class DIALOG_SHEET_PIN_PROPERTIES : public DIALOG_SHEET_PIN_PROPERTIES_BASE
{
public:
    DIALOG_SHEET_PIN_PROPERTIES( SCH_EDIT_FRAME* parent, SCH_SHEET_PIN* aPin );
    ~DIALOG_SHEET_PIN_PROPERTIES();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
	void onOKButton( wxCommandEvent& event ) override;
    void OnSyntaxHelp( wxHyperlinkEvent& event ) override;
    void onComboBox( wxCommandEvent& event ) override;

    SCH_EDIT_FRAME*   m_frame;
    SCH_SHEET_PIN*    m_sheetPin;

    UNIT_BINDER       m_textSize;

    HTML_MESSAGE_BOX* m_helpWindow;
};
