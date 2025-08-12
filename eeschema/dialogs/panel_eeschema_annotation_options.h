/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mike Williams <mike@mikebwilliams.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_EESCHEMA_ANNOTATION_OPTIONS_H
#define PANEL_EESCHEMA_ANNOTATION_OPTIONS_H

#include <widgets/unit_binder.h>
#include "panel_eeschema_annotation_options_base.h"

class EDA_BASE_FRAME;
class SCHEMATIC_SETTINGS;


class PANEL_EESCHEMA_ANNOTATION_OPTIONS : public PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE
{
public:
    PANEL_EESCHEMA_ANNOTATION_OPTIONS( wxWindow* aWindow, EDA_BASE_FRAME* schSettingsProvider );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

    void ImportSettingsFrom( SCHEMATIC_SETTINGS& aSettings );

private:
    void loadEEschemaSettings( SCHEMATIC_SETTINGS* aCfg );

    EDA_BASE_FRAME* m_schSettingsProvider;
};


#endif //PANEL_EESCHEMA_ANNOTATION_OPTIONS_H
