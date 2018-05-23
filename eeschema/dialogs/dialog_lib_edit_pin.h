/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __dialog_lib_edit_pin__
#define __dialog_lib_edit_pin__

#include <wx/bmpcbox.h>
#include <pin_shape_combobox.h>
#include <pin_type_combobox.h>

#include <dialog_lib_edit_pin_base.h>
#include <widgets/unit_binder.h>
#include <lib_pin.h>
#include <lib_edit_frame.h>

/** Implementing DIALOG_LIB_EDIT_PIN_BASE */
class DIALOG_LIB_EDIT_PIN : public DIALOG_LIB_EDIT_PIN_BASE
{
    LIB_EDIT_FRAME* m_frame;
    LIB_PIN*        m_pin;
    LIB_PIN*        m_dummyPin;       // a working copy used to show changes

    UNIT_BINDER     m_posX;
    UNIT_BINDER     m_posY;
    UNIT_BINDER     m_pinLength;
    UNIT_BINDER     m_nameSize;
    UNIT_BINDER     m_numberSize;

public:
    /** Constructor */
    DIALOG_LIB_EDIT_PIN( LIB_EDIT_FRAME* parent, LIB_PIN* aPin );
    ~DIALOG_LIB_EDIT_PIN() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnPaintShowPanel( wxPaintEvent& event ) override;
    void OnPropertiesChange( wxCommandEvent& event ) override;
};

#endif // __dialog_lib_edit_pin__
