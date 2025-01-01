/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef DIALOG_IMAGE_PROPERTIES_H
#define DIALOG_IMAGE_PROPERTIES_H

#include <dialogs/dialog_image_properties_base.h>
#include <widgets/unit_binder.h>


class SCH_EDIT_FRAME;
class SCH_BITMAP;
class PANEL_IMAGE_EDITOR;


class DIALOG_IMAGE_PROPERTIES : public DIALOG_IMAGE_PROPERTIES_BASE
{
public:
    DIALOG_IMAGE_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_BITMAP& aBitmap );
    ~DIALOG_IMAGE_PROPERTIES() override {}

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    SCH_EDIT_FRAME*     m_frame;
    SCH_BITMAP&         m_bitmap;
    PANEL_IMAGE_EDITOR* m_imageEditor;

    UNIT_BINDER m_posX;
    UNIT_BINDER m_posY;
};

#endif
