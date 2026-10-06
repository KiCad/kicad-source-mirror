/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VIA_PROTECTION_RULES_H
#define VIA_PROTECTION_RULES_H

/**
 * Which IPC-4761 via protection features can be built together. Board Setup greys out what
 * these rule out and the cross section preview does not draw it, so the two agree.
 */
namespace VIA_PROTECTION_RULES
{

/// A cap is copper plated over a filled hole, so it needs the fill.
inline bool CapAllowed( bool aFilled )
{
    return aFilled;
}


/// A plug is a partial fill from one side. A filled hole has nothing left to plug.
inline bool PlugAllowed( bool aFilled )
{
    return !aFilled;
}


/// A cover is a second coat over a tent, a plug or a fill. IPC-4761 has no cover over an open
/// hole, because a coat cannot float.
inline bool CoverAllowed( bool aFilled, bool aTented, bool aPlugged )
{
    return aFilled || aTented || aPlugged;
}

} // namespace VIA_PROTECTION_RULES

#endif
