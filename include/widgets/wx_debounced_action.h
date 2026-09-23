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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>

#include <wx/timer.h>


/**
 * Debounces an action: runs it only after a quiet period following the last trigger.
 *
 * Call Restart() to trigger the debounce timer, and on every (potential) trigger.
 * Call Cancel() to drop a pending action.
 */
class WX_DEBOUNCED_ACTION : public wxTimer
{
public:
    using ACTION_FN = std::function<void()>;

    /**
     * @param aAction the action to run when the delay elapses.
     * @param aDelayMs the period after a trigger when the action runs
     */
    WX_DEBOUNCED_ACTION( ACTION_FN aAction, int aDelayMs );

    /// (Re)start the delay: the action runs by the delay after the call, unless another
    /// one is made within the delay period.
    void Restart();

    /// Stop the timer. Any pending action will be canceled.
    ///
    /// Does nothing when none is pending. You must do this if the object the
    /// action callback will use is about to be destroyed.
    void Cancel();

    /// A typical debounce period for text input events, in milliseconds.
    constexpr static int TEXT_INPUT_DEBOUNCE_MS = 200;
    /// A typical debounce period for hover preview events, in milliseconds.
    constexpr static int HOVER_PREVIEW_DEBOUNCE_MS = 400;

protected:
    void Notify() override;

private:
    ACTION_FN m_action;
    int       m_delayMs;
};
