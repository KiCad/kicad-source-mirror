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

#ifndef EDITOR_TAB_CONTEXT_H
#define EDITOR_TAB_CONTEXT_H

#include <memory>
#include <vector>

#include <wx/string.h>

#include <kiid.h>
#include <math/vector2d.h>
#include <undo_redo_container.h>


/// One open document in a tabbed editor. Owns the undo/redo and view/selection snapshot. The
/// subclass owns the document object itself.
class EDITOR_TAB_CONTEXT
{
public:
    EDITOR_TAB_CONTEXT() :
            m_undo( std::make_unique<UNDO_REDO_CONTAINER>() ),
            m_redo( std::make_unique<UNDO_REDO_CONTAINER>() )
    {
    }

    virtual ~EDITOR_TAB_CONTEXT() = default;

    /**
     * Stable identity for persistence and de-duplication.
     */
    virtual wxString GetTabKey() const = 0;

    /**
     * Short label shown on the tab.
     */
    virtual wxString GetDisplayName() const = 0;

    virtual bool IsModified() const = 0;

    bool IsPreview() const          { return m_preview; }
    void SetPreview( bool aPreview ){ m_preview = aPreview; }

    UNDO_REDO_CONTAINER& UndoList() { return *m_undo; }
    UNDO_REDO_CONTAINER& RedoList() { return *m_redo; }

    /// View snapshot captured on detach, restored on activate.
    struct VIEW_SNAPSHOT
    {
        double   scale = 0.0;
        VECTOR2D center;
        bool     valid = false;
    };

    VIEW_SNAPSHOT& ViewSnapshot() { return m_viewSnapshot; }

    /**
     * Selection saved as resolved KIIDs, restored after the view is rebuilt.
     */
    std::vector<KIID>& SavedSelection() { return m_savedSelection; }

protected:
    bool                                 m_preview = false;
    std::unique_ptr<UNDO_REDO_CONTAINER> m_undo;
    std::unique_ptr<UNDO_REDO_CONTAINER> m_redo;
    VIEW_SNAPSHOT                        m_viewSnapshot;
    std::vector<KIID>                    m_savedSelection;
};

#endif // EDITOR_TAB_CONTEXT_H
