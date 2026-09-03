/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KIPLATFORM_TOUCHPAD_H_
#define KIPLATFORM_TOUCHPAD_H_

#include <functional>
#include <memory>

#include <wx/gdicmn.h>

class wxWindow;

namespace KIPLATFORM
{
namespace UI
{
/**
 * A pan and zoom update produced by a native touchpad gesture recognizer.
 */
struct TOUCHPAD_GESTURE
{
    double  panX;
    double  panY;
    double  zoomFactor;
    wxPoint zoomAnchor;
};


/**
 * Owns a platform touchpad input registration for the lifetime of an input window.
 */
class TOUCHPAD_GESTURE_HANDLER
{
public:
    virtual ~TOUCHPAD_GESTURE_HANDLER() = default;
};


using TOUCHPAD_GESTURE_CALLBACK = std::function<void( const TOUCHPAD_GESTURE& )>;

/**
 * Check whether this port provides the native touchpad gesture APIs.
 */
bool IsNativeTouchpadGestureAvailable();

/**
 * Register a window for native touchpad pan and pinch gestures when the port supports it.
 *
 * @param aInputWindow is the window that receives native pointer input.
 * @param aCallback receives gesture deltas in the input window's pixel coordinate space.
 * @return an owning registration handle, or nullptr when native touchpad input is unavailable.
 */
std::unique_ptr<TOUCHPAD_GESTURE_HANDLER> CreateTouchpadGestureHandler(
        wxWindow* aInputWindow, TOUCHPAD_GESTURE_CALLBACK aCallback );
}
}

#endif // KIPLATFORM_TOUCHPAD_H_
