/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef KICAD_SPACENAV_DRIVER_H
#define KICAD_SPACENAV_DRIVER_H

class SPACEMOUSE_HANDLER
{
public:
    virtual ~SPACEMOUSE_HANDLER() = default;

    /** Handle translation (pan) events. Units are driver dependent. */
    virtual void OnPan(double x, double y, double z) = 0;

    /** Handle rotational events. Units are driver dependent. */
    virtual void OnRotate(double rx, double ry, double rz) = 0;

    /** Handle button press/release events. */
    virtual void OnButton(int button, bool pressed) = 0;
};

class SPACENAV_DRIVER
{
public:
    virtual ~SPACENAV_DRIVER() = default;

    /** Connect to the device. Returns true on success. */
    virtual bool Connect() = 0;

    /** Disconnect from the device. */
    virtual void Disconnect() = 0;

    /** Poll for pending events and dispatch them to the handler. */
    virtual void Poll() = 0;

    void SetHandler( SPACEMOUSE_HANDLER* aHandler ) { m_handler = aHandler; }

protected:
    SPACEMOUSE_HANDLER* m_handler = nullptr;
};

#endif // KICAD_SPACENAV_DRIVER_H