/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_EDIT_SHEET_PIN_H
#define DIALOG_EDIT_SHEET_PIN_H


#include <dialog_edit_sheet_pin_base.h>
#include <widgets/unit_binder.h>


class SCH_SHEET_PIN;


class DIALOG_EDIT_SHEET_PIN : public DIALOG_EDIT_SHEET_PIN_BASE
{
    SCH_EDIT_FRAME* m_frame;
    SCH_SHEET_PIN*  m_sheetPin;

    UNIT_BINDER     m_textWidth;
    UNIT_BINDER     m_textHeight;

public:
    DIALOG_EDIT_SHEET_PIN( SCH_EDIT_FRAME* parent, SCH_SHEET_PIN* aPin );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
	void onOKButton( wxCommandEvent& event ) override;
};

#endif // DIALOG_EDIT_SHEET_PIN_H
