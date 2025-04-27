/**
 * @file properties_frame.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

    UNIT_BINDER m_textSizeX;
    UNIT_BINDER m_textSizeY;

    UNIT_BINDER m_constraintX;
    UNIT_BINDER m_constraintY;

    UNIT_BINDER m_textPosX;
    UNIT_BINDER m_textPosY;

    UNIT_BINDER m_textEndX;
    UNIT_BINDER m_textEndY;

    UNIT_BINDER m_textStepX;
    UNIT_BINDER m_textStepY;

    UNIT_BINDER m_defaultTextSizeX;
    UNIT_BINDER m_defaultTextSizeY;

    UNIT_BINDER m_defaultLineWidth;
    UNIT_BINDER m_defaultTextThickness;

    UNIT_BINDER m_textLeftMargin;
    UNIT_BINDER m_textRightMargin;

    UNIT_BINDER m_textTopMargin;
    UNIT_BINDER m_textBottomMargin;

    UNIT_BINDER m_lineWidth;

    bool        m_propertiesDirty;

public:
    PROPERTIES_FRAME( PL_EDITOR_FRAME* aParent );
    ~PROPERTIES_FRAME();

    void OnAcceptPrms();

    // Event functions
    void OnUpdateUI( wxUpdateUIEvent& aEvent ) override;
    void onModify( wxCommandEvent& aEvent ) override;
    void onTextFocusLost( wxFocusEvent& aEvent ) override;
    void OnSetDefaultValues( wxCommandEvent& event ) override;
    void onScintillaCharAdded( wxStyledTextEvent &aEvent );
    void onScintillaFocusLost( wxFocusEvent& aEvent ) override;
	void onHelp( wxHyperlinkEvent& aEvent ) override;
    void onHAlignButton( wxCommandEvent &aEvent );
    void onVAlignButton( wxCommandEvent &aEvent );

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
