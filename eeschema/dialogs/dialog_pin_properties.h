/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHOR.txt for contributors.
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
#include <wx/dcclient.h>

#include <pin_shape_combobox.h>
#include <pin_type_combobox.h>

#include <dialog_pin_properties_base.h>
#include <widgets/unit_binder.h>
#include <lib_pin.h>
#include <symbol_edit_frame.h>


enum COL_ORDER
{
    COL_NAME,
    COL_TYPE,
    COL_SHAPE,

    COL_COUNT       // keep as last
};


class ALT_PIN_DATA_MODEL;


class DIALOG_PIN_PROPERTIES : public DIALOG_PIN_PROPERTIES_BASE
{
public:
    DIALOG_PIN_PROPERTIES( SYMBOL_EDIT_FRAME* parent, LIB_PIN* aPin );
    ~DIALOG_PIN_PROPERTIES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnPaintShowPanel( wxPaintEvent& event ) override;
    void OnPropertiesChange( wxCommandEvent& event ) override;
    void OnAddAlternate( wxCommandEvent& event ) override;
    void OnDeleteAlternate( wxCommandEvent& event ) override;
    void OnSize( wxSizeEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

protected:
    void adjustGridColumns( int aWidth );

private:
    SYMBOL_EDIT_FRAME*  m_frame;
    LIB_PIN*            m_pin;
    LIB_PIN*            m_dummyPin;       // a working copy used to show changes

    UNIT_BINDER         m_posX;
    UNIT_BINDER         m_posY;
    UNIT_BINDER         m_pinLength;
    UNIT_BINDER         m_nameSize;
    UNIT_BINDER         m_numberSize;

    wxPoint             m_origPos;

    ALT_PIN_DATA_MODEL* m_alternatesDataModel;

    int                 m_delayedFocusRow;
    int                 m_delayedFocusColumn;

    int                 m_originalColWidths[ COL_COUNT ];
    int                 m_width;
    bool                m_initialized;
};

#endif // __dialog_lib_edit_pin__
