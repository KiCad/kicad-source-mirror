/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Seth Hillbrand <hillbrand@ucdavis.edu>
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

#ifndef DIALOG_LINE_PROPERTIES_H
#define DIALOG_LINE_PROPERTIES_H

#include <dialog_line_properties_base.h>
#include <widgets/unit_binder.h>


class SCH_EDIT_FRAME;
class SCH_LINE;


class DIALOG_LINE_PROPERTIES : public DIALOG_LINE_PROPERTIES_BASE
{
public:
    DIALOG_LINE_PROPERTIES( SCH_EDIT_FRAME* aParent, std::deque<SCH_LINE*>& aLines );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    SCH_EDIT_FRAME*       m_frame;
    std::deque<SCH_LINE*> m_lines;

    UNIT_BINDER           m_width;

    void resetDefaults( wxCommandEvent& event ) override;
};

#endif // DIALOG_LINE_PROPERTIES_H
