/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_BOARD_CONTEXT_H
#define KICAD_BOARD_CONTEXT_H

class BOARD;
class KIWAY;
class PROJECT;
class TOOL_MANAGER;


/// Base interface for board-level API contexts; shared by PCB editor and footprint editor
class BOARD_CONTEXT
{
public:
    virtual ~BOARD_CONTEXT() = default;

    virtual BOARD* GetBoard() const = 0;

    virtual PROJECT& Prj() const = 0;

    virtual TOOL_MANAGER* GetToolManager() const = 0;

    virtual KIWAY* GetKiway() const = 0;

    virtual bool CanAcceptApiCommands() const = 0;
};

#endif // KICAD_BOARD_CONTEXT_H
