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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef DIALOG_FOOTPRINT_ASSOCIATIONS_H
#define DIALOG_FOOTPRINT_ASSOCIATIONS_H


#include <dialog_footprint_associations_base.h>

class PCB_BASE_FRAME;
class FOOTPRINT;


/**
 * Dialog to show footprint library and symbol links.
 */
class DIALOG_FOOTPRINT_ASSOCIATIONS : public DIALOG_FOOTPRINT_ASSOCIATIONS_BASE
{
public:
    DIALOG_FOOTPRINT_ASSOCIATIONS( PCB_BASE_FRAME* aFrame, FOOTPRINT* aFootprint );
    ~DIALOG_FOOTPRINT_ASSOCIATIONS() { }

    ///< Get data from the PCB board and print it to dialog
    bool TransferDataToWindow() override;

private:
    PCB_BASE_FRAME* m_frame;
    FOOTPRINT*      m_footprint;
};

#endif // _DIALOG_FOOTPRINT_ASSOCIATIONS_H
