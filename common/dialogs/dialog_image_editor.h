/**
 * @file  dialog_image_editor.h
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _DIALOG_IMAGE_EDITOR_H_
#define _DIALOG_IMAGE_EDITOR_H_

#include <dialog_image_editor_base.h>


class DIALOG_IMAGE_EDITOR : public DIALOG_IMAGE_EDITOR_BASE
{
private:
    BITMAP_BASE*     m_workingImage;        // The copy of BITMAP_BASE to be edited
    BITMAP_BASE*     m_lastImage;           // the saved BITMAP_BASE before a new change.
                                            // Used to undo the last change

public:
    DIALOG_IMAGE_EDITOR( wxWindow* aParent, BITMAP_BASE* aItem );
    ~DIALOG_IMAGE_EDITOR(){ delete m_workingImage; }


public:
    /**
     * Function TransfertToImage
     * copy edited image to aItem
     * @param aItem = the target
     */
    void TransfertToImage( BITMAP_BASE* aItem );

private:
    void OnUndoLastChange( wxCommandEvent& event ) override;
    void OnGreyScaleConvert( wxCommandEvent& event ) override;
    void OnHalfSize( wxCommandEvent& event ) override;
    void OnMirrorX_click( wxCommandEvent& event ) override;
    void OnMirrorY_click( wxCommandEvent& event ) override;
    void OnRotateClick( wxCommandEvent& event ) override;
    void OnOK_Button( wxCommandEvent& aEvent ) override;
    void OnCancel_Button( wxCommandEvent& aEvent ) override;
    void OnRedrawPanel( wxPaintEvent& event ) override;
    bool CheckValues();
};


#endif    // _DIALOG_IMAGE_EDITOR_H_
