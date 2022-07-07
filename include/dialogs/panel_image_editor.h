/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2018 jean-pierre.charras
 * Copyright (C) 2018 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_IMAGE_EDITOR_H
#define PANEL_IMAGE_EDITOR_H

#include <bitmap_base.h>
#include <panel_image_editor_base.h>


class PANEL_IMAGE_EDITOR : public PANEL_IMAGE_EDITOR_BASE
{
private:
    BITMAP_BASE* m_workingImage; // The copy of BITMAP_BASE to be edited

public:
    PANEL_IMAGE_EDITOR( wxWindow* aParent, BITMAP_BASE* aItem );
    ~PANEL_IMAGE_EDITOR() { delete m_workingImage; }


public:
    bool TransferDataFromWindow() override;

    /**
     * Function TransferToImage
     * copy edited image to aItem
     */
    void TransferToImage( BITMAP_BASE* aItem );

private:
    void OnGreyScaleConvert( wxCommandEvent& event ) override;
    void OnRedrawPanel( wxPaintEvent& event ) override;

    bool CheckValues();
};

#endif
