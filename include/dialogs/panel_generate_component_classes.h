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


#ifndef PANEL_GENERATE_COMPONENT_CLASSES_H
#define PANEL_GENERATE_COMPONENT_CLASSES_H

#include <panel_generate_component_classes_base.h>

#include <eda_draw_frame.h>
#include <project/component_class_settings.h>
#include <vector>


class PANEL_GENERATE_COMPONENT_CLASSES : public PANEL_GENERATE_COMPONENT_CLASSES_BASE
{
public:
    PANEL_GENERATE_COMPONENT_CLASSES( wxWindow* aParentWindow, EDA_DRAW_FRAME* aFrame,
                                      std::shared_ptr<COMPONENT_CLASS_SETTINGS> aSettings,
                                      std::vector<wxString>&                    aSelectionRefs );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    //void ImportSettingsFrom( const std::shared_ptr<NET_SETTINGS>& aNetSettings );

protected:
    EDA_DRAW_FRAME*                           m_frame;
    std::shared_ptr<COMPONENT_CLASS_SETTINGS> m_componentClassSettings;

    std::vector<wxString> m_selectionRefs;
};

#endif //PANEL_GENERATE_COMPONENT_CLASSES
