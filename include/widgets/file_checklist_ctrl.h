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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FILE_CHECKLIST_CTRL_H
#define FILE_CHECKLIST_CTRL_H

#include <map>
#include <vector>

#include <wx/string.h>

#include <bitmaps/bitmaps_list.h>
#include <widgets/wx_dataviewctrl.h>
#include <wx/colour.h>
#include <wx/event.h>
#include <kicommon.h>


/**
 * One file row in a FILE_CHECKLIST_CTRL. The types are deliberately generic so the
 * control has no dependency on git. A caller maps its own status to a word and an icon.
 */
struct KICOMMON_API FILE_CHECKLIST_ENTRY
{
    wxString section;
    wxString path;    ///< Row identity.
    wxString label;   ///< Shown in place of the path.
    wxString status;  ///< Free text, e.g. "Modified".
    wxColour statusColour; ///< Text colour for the status, or the default when not set.
    BITMAPS  icon = BITMAPS::INVALID_BITMAP;
    bool     checked = false;
};


/**
 * The rows and tick states behind FILE_CHECKLIST_CTRL. It owns no window, so the rules
 * that decide what the control shows can be tested headlessly.
 *
 * Sections and the rows within them keep the order in which they were added.
 */
class KICOMMON_API FILE_CHECKLIST_DATA
{
public:
    void SetEntries( const std::vector<FILE_CHECKLIST_ENTRY>& aEntries );

    /// Section headings in the order they were added.
    const std::vector<wxString>& Sections() const { return m_sections; }

    /// Rows under a section, in the order they were added. Empty for an unknown section.
    const std::vector<FILE_CHECKLIST_ENTRY>& Rows( const wxString& aSection ) const;

    void SetChecked( const wxString& aPath, bool aChecked );
    void SetSectionChecked( const wxString& aSection, bool aChecked );

    /// True when the section has rows and every one of them is ticked. Drives its header
    /// checkbox, which has no third state for a partly ticked section.
    bool IsSectionChecked( const wxString& aSection ) const;

    /// Paths of every ticked row, in the order they were added.
    std::vector<wxString> CheckedPaths() const;

private:
    std::vector<wxString>                                 m_sections;
    std::map<wxString, std::vector<FILE_CHECKLIST_ENTRY>> m_rowsBySection;
};


class FILE_CHECKLIST_MODEL;


/**
 * A checkable list of files, grouped into sections. Each row has a checkbox, a status
 * icon, a name, and a status word. Section header rows carry a checkbox that ticks or
 * clears every row in the section.
 *
 * The control is git-agnostic. Callers describe rows with FILE_CHECKLIST_ENTRY and ask
 * back for the checked paths.
 */
class KICOMMON_API FILE_CHECKLIST_CTRL : public WX_DATAVIEWCTRL
{
public:
    FILE_CHECKLIST_CTRL( wxWindow* aParent, wxWindowID aId = wxID_ANY );
    ~FILE_CHECKLIST_CTRL() override;

    void SetEntries( const std::vector<FILE_CHECKLIST_ENTRY>& aEntries );

    void SetAllChecked( bool aChecked );

    std::vector<wxString> GetCheckedPaths() const;

    /// Path of the file row for aItem, or empty for a section header or invalid item.
    wxString GetItemPath( const wxDataViewItem& aItem ) const;

    /// WX_DATAVIEWCTRL drops tooltips for every data view it is the base of. This one shows
    /// them, and remembers what the owner asked for so the row tooltip can give way to it.
    void DoSetToolTipText( const wxString& aTip ) override;

private:
    /// wxDataViewCtrl sends no mouse moves, so the hovered row is read on idle. LIB_TREE
    /// tracks its preview the same way.
    void onIdle( wxIdleEvent& aEvent );

    wxObjectDataPtr<FILE_CHECKLIST_MODEL> m_model;

    wxString m_ownerTooltip; ///< What the owner set, shown when no row has more to say.
    wxString m_shownTooltip; ///< What is on screen, so an unchanged hover is left alone.
};

#endif // FILE_CHECKLIST_CTRL_H
