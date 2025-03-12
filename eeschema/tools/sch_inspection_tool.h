/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef SCH_INSPECTION_TOOL_H
#define SCH_INSPECTION_TOOL_H

#include <tools/sch_tool_base.h>
#include <dialogs/dialog_book_reporter.h>
#include <sch_base_frame.h>


class SCH_SELECTION_TOOL;
class SCH_BASE_FRAME;
class DIALOG_ERC;
class SYMBOL_DIFF_WIDGET;


class SCH_INSPECTION_TOOL : public wxEvtHandler, public SCH_TOOL_BASE<SCH_BASE_FRAME>
{
public:
    SCH_INSPECTION_TOOL();
    ~SCH_INSPECTION_TOOL() {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    void Reset( RESET_REASON aReason ) override;

    int RunERC( const TOOL_EVENT& aEvent );
    void ShowERCDialog();

    int PrevMarker( const TOOL_EVENT& aEvent );
    int NextMarker( const TOOL_EVENT& aEvent );

    /// Called when clicking on a item:
    int CrossProbe( const TOOL_EVENT& aEvent );

    void CrossProbe( const SCH_MARKER* aMarker );

    wxString InspectERCErrorMenuText( const std::shared_ptr<RC_ITEM>& aERCItem );
    void InspectERCError( const std::shared_ptr<RC_ITEM>& aERCItem );

    int ExcludeMarker( const TOOL_EVENT& aEvent );

    int ShowBusSyntaxHelp( const TOOL_EVENT& aEvent );

    int CheckSymbol( const TOOL_EVENT& aEvent );
    int DiffSymbol( const TOOL_EVENT& aEvent );
    void DiffSymbol( SCH_SYMBOL* aSymbol );

    int RunSimulation( const TOOL_EVENT& aEvent );

    int ShowDatasheet( const TOOL_EVENT& aEvent );

    /// Display the selected item info (when clicking on a item)
    int UpdateMessagePanel( const TOOL_EVENT& aEvent );

private:
    SYMBOL_DIFF_WIDGET* constructDiffPanel( wxPanel* aParentPanel );

    ///< @copydoc TOOL_INTERACTIVE::setTransitions();
    void setTransitions() override;

private:
    HTML_MESSAGE_BOX* m_busSyntaxHelp;
};

#endif /* SCH_INSPECTION_TOOL_H */
