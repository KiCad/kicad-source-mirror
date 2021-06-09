/**
 * @file properties_frame.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef  _PROPERTIES_FRAME_H
#define  _PROPERTIES_FRAME_H

#include "dialogs/properties_frame_base.h"
#include <widgets/unit_binder.h>

class DS_DATA_ITEM;
class PL_EDITOR_FRAME;
class SCINTILLA_TRICKS;

/**
 * PROPERTIES_FRAME display properties of the current item.
 */

class PROPERTIES_FRAME : public PANEL_PROPERTIES_BASE
{
    PL_EDITOR_FRAME*  m_parent;
    SCINTILLA_TRICKS* m_scintillaTricks;

    UNIT_BINDER m_textCtrlTextSizeXBinder;
    UNIT_BINDER m_textCtrlTextSizeYBinder;

    UNIT_BINDER m_textCtrlConstraintXBinder;
    UNIT_BINDER m_textCtrlConstraintYBinder;

    UNIT_BINDER m_textCtrlPosXBinder;
    UNIT_BINDER m_textCtrlPosYBinder;

    UNIT_BINDER m_textCtrlEndXBinder;
    UNIT_BINDER m_textCtrlEndYBinder;

    UNIT_BINDER m_textCtrlStepXBinder;
    UNIT_BINDER m_textCtrlStepYBinder;

    UNIT_BINDER m_textCtrlDefaultTextSizeXBinder;
    UNIT_BINDER m_textCtrlDefaultTextSizeYBinder;

    UNIT_BINDER m_textCtrlDefaultLineWidthBinder;
    UNIT_BINDER m_textCtrlDefaultTextThicknessBinder;

    UNIT_BINDER m_textCtrlLeftMarginBinder;
    UNIT_BINDER m_textCtrlRightMarginBinder;

    UNIT_BINDER m_textCtrlTopMarginBinder;
    UNIT_BINDER m_textCtrlBottomMarginBinder;

    UNIT_BINDER m_textCtrlThicknessBinder;

public:
    PROPERTIES_FRAME( PL_EDITOR_FRAME* aParent );
    ~PROPERTIES_FRAME();

    // Event functions
    void OnPageChanged( wxNotebookEvent& event ) override;
    void OnAcceptPrms( wxCommandEvent& event ) override;
    void OnSetDefaultValues( wxCommandEvent& event ) override;
    void onScintillaCharAdded( wxStyledTextEvent &aEvent );

    // Data transfer from general properties to widgets
    void CopyPrmsFromGeneralToPanel();

    // Data transfer from widgets to general properties
    bool CopyPrmsFromPanelToGeneral();

    // Data transfer from item to widgets in properties frame
    void CopyPrmsFromItemToPanel( DS_DATA_ITEM* aItem );

    // Data transfer from widgets in properties frame to item
    bool CopyPrmsFromPanelToItem( DS_DATA_ITEM* aItem );

    wxSize GetMinSize() const override;
};

#endif /* _PROPERTIES_FRAME_H */
