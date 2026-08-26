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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MDW_PART_ACTIONS_H
#define MDW_PART_ACTIONS_H

#include <lib_id.h>
#include <widgets/mdw_part_state.h>

#include <wx/arrstr.h>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

class CONDITIONAL_MENU;
class MDW_PART_EDITOR;
class TOOL_ACTION;
class TOOL_EVENT;
class TOOL_INTERACTIVE;
class TOOL_MENU;
class WX_INFOBAR;


/// The library tree's Lifecycle menu, each tool action with the part action it runs
const std::vector<std::pair<const TOOL_ACTION*, MDW_PART_ACTION::ID>>& MdwPartToolActions();

/// The part action aEvent runs, if it is one of MdwPartToolActions()
std::optional<MDW_PART_ACTION::ID> MdwPartActionOf( const TOOL_EVENT& aEvent );


/// Bring the source copy of library aNickname, which aEditor's library table holds, up to date
void MdwUpdatePartLibrary( MDW_PART_EDITOR& aEditor, const wxString& aNickname );

/**
 * Check in library aNickname of aEditor's library table after offering to save its unsaved changes, which aModified
 * says exist. aSave writes them and returns false on failure, which abandons the check-in.
 */
void MdwCheckInPartLibrary( MDW_PART_EDITOR& aEditor, const wxString& aNickname, bool aModified,
                            const std::function<bool()>& aSave );

/// Run aLoad unless aIsOpen already holds, and say whether the part is open then
bool MdwOpenPartOnce( const std::function<bool()>& aIsOpen, const std::function<void()>& aLoad );


/// aId as one of aParts offers it, preferring one that allows it; nothing when none does
std::optional<MDW_PART_ACTION> MdwOfferedPartAction( MDW_PART_EDITOR& aEditor, const std::vector<LIB_ID>& aParts,
                                                     MDW_PART_ACTION::ID aId );

/**
 * Run aId on aParts. Release and Check In take every part; the other actions open the first part and act on it.
 * aState is the state a TRANSITION moves to, empty to choose one.
 */
void MdwRunPartAction( MDW_PART_EDITOR& aEditor, MDW_PART_ACTION::ID aId, const std::vector<LIB_ID>& aParts,
                       const wxString& aState = wxEmptyString );

/// Run the Lifecycle action aEvent names on the parts of aSelected, which may also name libraries
void MdwRunPartToolAction( MDW_PART_EDITOR& aEditor, const TOOL_EVENT& aEvent, const std::vector<LIB_ID>& aSelected );

/**
 * Add the Lifecycle submenu to aContextMenu, registered with aTool's aToolMenu. Each action shows for the parts of
 * aSelected that aEditor offers it for; aEditor answers nothing while the tool has no library editor.
 */
void MdwAddLifecycleMenu( TOOL_INTERACTIVE* aTool, TOOL_MENU& aToolMenu, CONDITIONAL_MENU& aContextMenu,
                          std::function<MDW_PART_EDITOR*()> aEditor, std::function<std::vector<LIB_ID>()> aSelected );

/**
 * Add the open part's line to aMessages and to aInfoBar a link for each action aEditor's part allows, with why the
 * others are denied. A link runs aRun, and Get Latest after a release refused over a newer version asks aEditor.
 */
void MdwAddPartLinks( WX_INFOBAR& aInfoBar, wxArrayString& aMessages, MDW_PART_EDITOR& aEditor,
                      const std::function<void( const MDW_PART_ACTION& )>& aRun );

#endif // MDW_PART_ACTIONS_H
