/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/optional/optional.hpp>
#include <math/vector2d.h>
#include <tool/tool_interactive.h>

class EDA_DRAW_FRAME;


class PICKER_TOOL : public TOOL_INTERACTIVE
{
public:
    PICKER_TOOL();
    ~PICKER_TOOL() {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override { }

    ///> Event handler types.
    typedef std::function<bool(const VECTOR2D&)> CLICK_HANDLER;
    typedef std::function<void(const VECTOR2D&)> MOTION_HANDLER;
    typedef std::function<void(void)> CANCEL_HANDLER;
    typedef std::function<void(const int&)> FINALIZE_HANDLER;

    enum pickerEndState
    {
        WAIT_CANCEL,
        CLICK_CANCEL,
        END_ACTIVATE,
        EVT_CANCEL,
        EXCEPTION_CANCEL
    };

    ///> Main event loop.
    int Main( const TOOL_EVENT& aEvent );

    inline void SetCursor( const wxCursor& aCursor ) { m_cursor = aCursor; }

    /**
     * Function SetClickHandler()
     * Sets a handler for mouse click event. Handler may decide to receive further click by
     * returning true.
     */
    inline void SetClickHandler( CLICK_HANDLER aHandler )
    {
        wxASSERT( !m_clickHandler );
        m_clickHandler = aHandler;
    }

    /**
     * Function SetMotionHandler()
     * Sets a handler for mouse motion.  Used for roll-over highlighting.
     */
    inline void SetMotionHandler( MOTION_HANDLER aHandler )
    {
        wxASSERT( !m_motionHandler );
        m_motionHandler = aHandler;
    }

    /**
     * Function SetCancelHandler()
     * Sets a handler for cancel events (ESC or context-menu Cancel).
     */
    inline void SetCancelHandler( CANCEL_HANDLER aHandler )
    {
        wxASSERT( !m_cancelHandler );
        m_cancelHandler = aHandler;
    }

    /**
     * Function SetFinalizeHandler()
     * Sets a handler for the finalize event. Takes the state of the exit from the Main loop
     */
    inline void SetFinalizeHandler( FINALIZE_HANDLER aHandler )
    {
        wxASSERT( !m_finalizeHandler );
        m_finalizeHandler = aHandler;
    }

private:
    ///> Reinitializes tool to its initial state.
    void resetPicker();

    ///> Applies the requested VIEW_CONTROLS settings.
    void setControls();

    ///> @copydoc TOOL_INTERACTIVE::setTransitions();
    void setTransitions() override;

private:
    EDA_DRAW_FRAME*       m_frame;
    wxCursor              m_cursor;

    OPT<CLICK_HANDLER>    m_clickHandler;
    OPT<MOTION_HANDLER>   m_motionHandler;
    OPT<CANCEL_HANDLER>   m_cancelHandler;
    OPT<FINALIZE_HANDLER> m_finalizeHandler;

    OPT<VECTOR2D>         m_picked;
};

#endif /* PICKER_TOOL_H */
