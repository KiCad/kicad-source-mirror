/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

#include <dialogs/dialog_unit_entry_base.h>
#include <widgets/unit_binder.h>


/**
 * An extension of WX_TEXT_ENTRY_DIALOG that uses UNIT_BINDER to request a dimension
 * (e.g. mm, inches, etc) from the user according to the selected units
 */
class WX_UNIT_ENTRY_DIALOG : public WX_UNIT_ENTRY_DIALOG_BASE
{
public:
    WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, const wxString& aLabel,
                          long long int aDefaultValue );

    /**
     * Return the value in internal units.
     */
    int GetValue();

private:
    UNIT_BINDER m_unit_binder;
};


class WX_PT_ENTRY_DIALOG : public WX_PT_ENTRY_DIALOG_BASE
{
public:
    WX_PT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, const wxString& aLabelX,
                        const wxString& aLabelY, const VECTOR2I& aDefaultValue, bool aShowResetButt );

    /**
     * Return the value in internal units.
     */
    VECTOR2I GetValue();

	void ResetValues( wxCommandEvent& event ) override;

private:
    UNIT_BINDER m_unit_binder_x;
    UNIT_BINDER m_unit_binder_y;
};
