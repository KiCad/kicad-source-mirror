/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#pragma once

#include <wx/print.h>

class SCH_EDIT_FRAME;
class SCH_SCREEN;

namespace KIGFX
{
class GAL;
class SCH_VIEW;
class PAINTER;
};

/**
 * Custom print out for printing schematics.
 */
class SCH_PRINTOUT : public wxPrintout
{
public:
    SCH_PRINTOUT( SCH_EDIT_FRAME* aParent, const wxString& aTitle );

    bool OnPrintPage( int page ) override;
    bool HasPage( int page ) override;
    bool OnBeginDocument( int startPage, int endPage ) override;
    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo ) override;

    /**
     * Print the current SCH_SCREEN using a given wxDC
     * @param aScreen is the screen corresponding to the sheet to print
     * @param aDC is the drawing context to use. It can be only
     *  a wxDC returned by wxPrintout::GetDC() (a wxPrinterDC, a wxPostScriptDC or a wxMemoryDC )
     *  a wxMemoryDC (to print to the clipboard)
     * @param aForPrinting = true to draw to a printer, false to a wxMeoryDC used to
     * draw to the Clipboard
     * @return true if OK
     */
    bool PrintPage( SCH_SCREEN* aScreen, wxDC* aDC, bool aForPrinting );

private:
    SCH_EDIT_FRAME*        m_parent;
    ///< Source VIEW object (note that actual printing only refers to this object)
    const KIGFX::SCH_VIEW* m_view;

    int milsToIU( int aMils );
};
