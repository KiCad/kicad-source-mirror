/**
 * @file kicad_clipboard.h
 * @brief
 * @author Kristoffer Ödmark
 * @version 1.0
 * @date 2017-05-03
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef KICAD_CLIPBOARD_H
#define KICAD_CLIPBOARD_H

#include <plugins/kicad/kicad_plugin.h>
#include <board_item.h>
#include <footprint.h>
#include <plugins/kicad/pcb_parser.h>
#include <memory.h>
#include <tools/pcb_selection.h>


class CLIPBOARD_IO : public PCB_IO
{
public:
    CLIPBOARD_IO();
    ~CLIPBOARD_IO();

    /*
     * Saves the entire board to the clipboard formatted using the PCB_IO formatting
     */
    void Save( const wxString& aFileName, BOARD* aBoard,
               const PROPERTIES* aProperties = nullptr ) override;

    /*
     * Write all the settings of the BOARD* set by setBoard() and then adds all the
     * BOARD_ITEMs found in selection formatted by PCB_IO to clipboard as sexpr text
     */
    void SaveSelection( const PCB_SELECTION& selected, bool isFootprintEditor );

    BOARD_ITEM* Parse();

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
                 const PROPERTIES* aProperties = nullptr, PROJECT* aProject = nullptr,
                 PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    void SetBoard( BOARD* aBoard );

private:
    STRING_FORMATTER m_formatter;
};


#endif /* KICAD_CLIPBOARD_H */
