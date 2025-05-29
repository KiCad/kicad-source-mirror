/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHOR.txt for contributors.
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

#ifndef DIALOG_PIN_PROPERTIES_H
#define DIALOG_PIN_PROPERTIES_H

#include <wx/bmpcbox.h>
#include <wx/dcclient.h>

#include <pinshape_combobox.h>
#include <pintype_combobox.h>

#include <dialog_pin_properties_base.h>
#include <widgets/unit_binder.h>
#include <sch_pin.h>
#include <symbol_edit_frame.h>


enum COL_ORDER
{
    COL_NAME,
    COL_TYPE,
    COL_SHAPE,

    COL_COUNT       // keep as last
};


class ALT_PIN_DATA_MODEL;
class SYMBOL_PREVIEW_WIDGET;


class DIALOG_PIN_PROPERTIES : public DIALOG_PIN_PROPERTIES_BASE
{
public:
    DIALOG_PIN_PROPERTIES( SYMBOL_EDIT_FRAME* parent, SCH_PIN* aPin, bool aFocusPinNumber );
    ~DIALOG_PIN_PROPERTIES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnPropertiesChange( wxCommandEvent& event ) override;
    void OnAddAlternate( wxCommandEvent& event ) override;
    void OnDeleteAlternate( wxCommandEvent& event ) override;
    void OnSize( wxSizeEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCollapsiblePaneChange( wxCollapsiblePaneEvent& event ) override;

protected:
    void adjustGridColumns();
    wxString getSyncPinsMessage();

private:
    SYMBOL_EDIT_FRAME*     m_frame;
    SCH_PIN*               m_pin;

    LIB_SYMBOL*            m_dummyParent;
    SCH_PIN*               m_dummyPin;                   // a working copy used to show changes
    SYMBOL_PREVIEW_WIDGET* m_previewWidget;

    UNIT_BINDER            m_posX;
    UNIT_BINDER            m_posY;
    UNIT_BINDER            m_pinLength;
    UNIT_BINDER            m_nameSize;
    UNIT_BINDER            m_numberSize;

    VECTOR2I               m_origPos;

    ALT_PIN_DATA_MODEL*    m_alternatesDataModel;

    int                    m_delayedFocusRow;
    int                    m_delayedFocusColumn;

    std::map<int, int>     m_originalColWidths;          // map col-number : orig-col-width
    wxSize                 m_size;
    bool                   m_initialized;
    inline static bool     s_alternatesTurndownOpen = false;
};

#endif // DIALOG_PIN_PROPERTIES_H
