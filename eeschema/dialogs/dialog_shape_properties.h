/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef DIALOG_SHAPE_PROPERTIES_H
#define DIALOG_SHAPE_PROPERTIES_H


class SCH_SHAPE;
class SCH_EDIT_FRAME;


#include <dialog_shape_properties_base.h>
#include <widgets/unit_binder.h>


class DIALOG_SHAPE_PROPERTIES : public DIALOG_SHAPE_PROPERTIES_BASE
{
public:
    DIALOG_SHAPE_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_SHAPE* aShape );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void onBorderChecked( wxCommandEvent& aEvent) override;
    void onFillChecked( wxCommandEvent& aEvent ) override;

private:
    SCH_EDIT_FRAME* m_frame;
    SCH_SHAPE*      m_shape;
    UNIT_BINDER     m_borderWidth;
};

#endif // DIALOG_SHAPE_PROPERTIES_H
