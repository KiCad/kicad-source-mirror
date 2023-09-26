/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef DIALOG_FOOTPRINT_CHOOSER_H
#define DIALOG_FOOTPRINT_CHOOSER_H

#include <lib_id.h>
#include "dialog_shim.h"


class PCB_BASE_FRAME;
class PANEL_FOOTPRINT_CHOOSER;


class DIALOG_FOOTPRINT_CHOOSER : public DIALOG_SHIM
{
public:
    DIALOG_FOOTPRINT_CHOOSER( PCB_BASE_FRAME* aParent, const LIB_ID& aPreselect,
                              const wxArrayString& aFootprintHistoryList );

    ~DIALOG_FOOTPRINT_CHOOSER() {};

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId() const;

protected:
    PANEL_FOOTPRINT_CHOOSER* m_chooserPanel;
};

#endif /* DIALOG_FOOTPRINT_CHOOSER_H */
