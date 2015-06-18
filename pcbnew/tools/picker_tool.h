/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PICKER_TOOL_H
#define PICKER_TOOL_H

#include <tool/tool_interactive.h>
#include <boost/optional/optional.hpp>
#include <boost/function.hpp>

/**
 * @brief Generic tool for picking a point.
 */
class PICKER_TOOL : public TOOL_INTERACTIVE
{
public:
    PICKER_TOOL();
    ~PICKER_TOOL() {}

    ///> Mouse event click handler type.
    typedef boost::function<bool(const VECTOR2D&)> CLICK_HANDLER;

    ///> @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) {}

    ///> Main event loop.
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Function SetSnapping()
     * Sets cursor snapping to grid for the period when the tool is active.
     */
    inline void SetSnapping( bool aEnable ) { m_cursorSnapping = aEnable; }

    /**
     * Function SetCursorVisible()
     * Sets cursor visibility for the period when the tool is active.
     */
    inline void SetCursorVisible( bool aEnable ) { m_cursorVisible = aEnable; }

    /**
     * Function SetAutoPanning()
     * Sets autopanning mode for the period when the tool is active.
     */
    inline void SetAutoPanning( bool aEnable ) { m_autoPanning = aEnable; }

    /**
     * Function GetPoint()
     * Returns picked point.
     */
    inline boost::optional<VECTOR2D> GetPoint() const
    {
        assert( !m_picking );
        return m_picked;
    }

    /**
     * Function IsPicking()
     * Returns information whether the tool is still active.
     */
    bool IsPicking() const { return m_picking; }

    /**
     * Function SetClickHandler()
     * Sets a handler for mouse click event. Handler may decide to receive further click by
     * returning true.
     */
    inline void SetClickHandler( CLICK_HANDLER aHandler )
    {
        assert( !m_clickHandler );
        m_clickHandler = aHandler;
    }

    ///> @copydoc TOOL_INTERACTIVE::SetTransitions();
    void SetTransitions();

private:
    // Tool settings.
    bool m_cursorSnapping;
    bool m_cursorVisible;
    bool m_autoPanning;

    ///> Optional mouse click event handler.
    boost::optional<CLICK_HANDLER> m_clickHandler;

    ///> Picked point (if any).
    boost::optional<VECTOR2D> m_picked;

    ///> Activity status.
    bool m_picking;

    ///> Reinitializes tool to its initial state.
    void reset();
};

#endif /* PICKER_TOOL_H */
