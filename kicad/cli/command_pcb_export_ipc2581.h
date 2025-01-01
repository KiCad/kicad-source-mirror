/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef COMMAND_EXPORT_PCB_IPC2581_H
#define COMMAND_EXPORT_PCB_IPC2581_H

#include "command_pcb_export_base.h"


namespace CLI
{
#define ARG_NO_X2 "--no-x2"
#define ARG_NO_NETLIST "--no-netlist"
#define ARG_SUBTRACT_SOLDERMASK "--subtract-soldermask"
#define ARG_DISABLE_APERTURE_MACROS "--disable-aperture-macros"
#define ARG_USE_DRILL_FILE_ORIGIN "--use-drill-file-origin"
#define ARG_PRECISION "--precision"
#define ARG_NO_PROTEL_EXTENSION "--no-protel-ext"

class PCB_EXPORT_IPC2581_COMMAND : public PCB_EXPORT_BASE_COMMAND
{
public:
    PCB_EXPORT_IPC2581_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};
} // namespace CLI

#endif
