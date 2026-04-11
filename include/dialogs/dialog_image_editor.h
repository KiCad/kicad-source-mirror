/**
 * @file  dialog_image_editor.h
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2018 jean-pierre.charras
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _DIALOG_IMAGE_EDITOR_H_
#define _DIALOG_IMAGE_EDITOR_H_

#include <dialog_image_editor_base.h>


class DIALOG_IMAGE_EDITOR : public DIALOG_IMAGE_EDITOR_BASE
{
private:
    BITMAP_BASE*     m_workingImage;        // The copy of BITMAP_BASE to be edited

public:
    DIALOG_IMAGE_EDITOR( wxWindow* aParent, BITMAP_BASE* aItem );
    ~DIALOG_IMAGE_EDITOR(){ delete m_workingImage; }


public:
    /**
     * Copy edited image to \a aItem.
     */
    void TransferToImage( BITMAP_BASE* aItem );

private:
    void OnGreyScaleConvert( wxCommandEvent& event ) override;
    void OnRedrawPanel( wxPaintEvent& event ) override;
    bool TransferDataFromWindow() override;

    bool CheckValues();
};


#endif    // _DIALOG_IMAGE_EDITOR_H_
