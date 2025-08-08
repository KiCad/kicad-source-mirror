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

#pragma once

#include <dialog_push_pad_properties_base.h>

class PAD;
class PCB_BASE_FRAME;


class DIALOG_PUSH_PAD_PROPERTIES : public DIALOG_PUSH_PAD_PROPERTIES_BASE
{
public:
    DIALOG_PUSH_PAD_PROPERTIES( PCB_BASE_FRAME* aParent );

    bool GetPadShapeFilter() const { return m_Pad_Shape_Filter_CB->GetValue(); }
    bool GetPadLayerFilter() const { return m_Pad_Layer_Filter_CB->GetValue(); }
    bool GetPadOrientFilter() const { return m_Pad_Orient_Filter_CB->GetValue(); }
    bool GetPadTypeFilter() const { return m_Pad_Type_Filter_CB->GetValue(); }

private:
    void PadPropertiesAccept( wxCommandEvent& event ) override;

private:
    PCB_BASE_FRAME* m_parent;
};


