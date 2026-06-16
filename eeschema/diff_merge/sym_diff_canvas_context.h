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

class LIB_SYMBOL;
class WIDGET_DIFF_CANVAS;


namespace KICAD_DIFF
{

void ConfigureSymDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, LIB_SYMBOL* aBefore, LIB_SYMBOL* aAfter );

} // namespace KICAD_DIFF

#endif // SYM_DIFF_CANVAS_CONTEXT_H
