/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#pragma once

#include <dialog_draw_layers_settings_base.h>
#include <widgets/unit_binder.h>


class DIALOG_DRAW_LAYERS_SETTINGS : public DIALOG_DRAW_LAYERS_SETTINGS_BASE
{
public:
    DIALOG_DRAW_LAYERS_SETTINGS( GERBVIEW_FRAME* aParent );
    ~DIALOG_DRAW_LAYERS_SETTINGS() {};

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    ///< Update layerset basing on the selected layers
    int setLayerSetFromList();

    GERBVIEW_FRAME* m_parent;

    UNIT_BINDER       m_offsetX;
    UNIT_BINDER       m_offsetY;
    UNIT_BINDER       m_rotation;

};
