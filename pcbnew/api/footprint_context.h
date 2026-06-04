/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_FOOTPRINT_CONTEXT_H
#define KICAD_FOOTPRINT_CONTEXT_H

#include <memory>
#include <wx/string.h>

#include <api/board_context.h>

class FOOTPRINT;
class FOOTPRINT_EDIT_FRAME;
class LIB_ID;


class FOOTPRINT_CONTEXT : public BOARD_CONTEXT
{
public:
    virtual LIB_ID GetLoadedFPID() const = 0;

    virtual bool SaveFootprint( FOOTPRINT* aFootprint ) = 0;

    virtual bool SaveFootprintInLibrary( FOOTPRINT* aFootprint, const wxString& aLibraryName ) = 0;
};


std::shared_ptr<FOOTPRINT_CONTEXT> CreateFootprintFrameContext( FOOTPRINT_EDIT_FRAME* aFrame );

#endif
