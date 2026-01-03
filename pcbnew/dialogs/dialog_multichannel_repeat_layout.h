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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __DIALOG_MULTICHANNEL_REPEAT_LAYOUT__
#define __DIALOG_MULTICHANNEL_REPEAT_LAYOUT__

#include <vector>
#include <wx/bmpbndl.h>
#include <board.h>
#include <widgets/unit_binder.h>

#include <dialogs/dialog_multichannel_repeat_layout_base.h>

class PCB_BASE_FRAME;
class MULTICHANNEL_TOOL;
struct RULE_AREA;
class wxGridEvent;

class DIALOG_MULTICHANNEL_REPEAT_LAYOUT : public DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE
{
public:
    DIALOG_MULTICHANNEL_REPEAT_LAYOUT(
        PCB_BASE_FRAME* aFrame,
        MULTICHANNEL_TOOL* aParentTool );
    ~DIALOG_MULTICHANNEL_REPEAT_LAYOUT();

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    struct TABLE_ENTRY {
        RULE_AREA *m_targetRA;
        bool m_isOK;
        bool m_doCopy;
        wxString m_raName;
        wxString m_errMsg;
        std::vector<wxString> m_mismatchReasons;
    };

    void OnGridCellLeftClick( wxGridEvent& aEvent );

    std::vector<TABLE_ENTRY> m_targetRAs;
    MULTICHANNEL_TOOL*       m_parentTool;
    BOARD*                   m_board;
    wxBitmapBundle           m_detailsIcon;
};

#endif
