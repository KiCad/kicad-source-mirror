/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef SYM_DIFF_CANVAS_CONTEXT_H
#define SYM_DIFF_CANVAS_CONTEXT_H

#include <diff_merge/sym_lib_differ.h>

#include <memory>
#include <vector>

class DIALOG_KICAD_DIFF;
class LIB_SYMBOL;
class WIDGET_DIFF_CANVAS;


namespace KICAD_DIFF
{

void ConfigureSymDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, LIB_SYMBOL* aBefore, LIB_SYMBOL* aAfter );

/**
 * Draw the symbol aName as it is in aBefore and aAfter.  The canvas does not own what it draws,
 * so the flattened copies are kept in aShown until the next call replaces them.
 */
void ShowSymbolPair( WIDGET_DIFF_CANVAS& aCanvas, const SYM_LIB_DIFFER::SYMBOL_MAP& aBefore,
                     const SYM_LIB_DIFFER::SYMBOL_MAP& aAfter, const wxString& aName,
                     std::vector<std::unique_ptr<LIB_SYMBOL>>& aShown );

/**
 * Draw each symbol chosen in aDialog as it is in aBefore and aAfter, which aDiff compares. Both
 * maps must outlive the dialog.
 */
void ShowSymbolChanges( DIALOG_KICAD_DIFF& aDialog, const DOCUMENT_DIFF& aDiff,
                        const SYM_LIB_DIFFER::SYMBOL_MAP& aBefore, const SYM_LIB_DIFFER::SYMBOL_MAP& aAfter );

} // namespace KICAD_DIFF

#endif // SYM_DIFF_CANVAS_CONTEXT_H
