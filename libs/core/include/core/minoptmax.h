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

#ifndef MINOPTMAX_PROTO_H
#define MINOPTMAX_PROTO_H

#include <limits>

template<class T=int>
class MINOPTMAX
{
public:
    T Min() const { return m_hasMin ? m_min : 0; };
    T Max() const { return m_hasMax ? m_max : std::numeric_limits<T>::max(); };
    T Opt() const { return m_hasOpt ? m_opt : Min(); };

    bool HasMin() const { return m_hasMin; }
    bool HasMax() const { return m_hasMax; }
    bool HasOpt() const { return m_hasOpt; }

    void SetMin( T v ) { m_isNull = false; m_min = v; m_hasMin = true; }
    void SetMax( T v ) { m_isNull = false; m_max = v; m_hasMax = true; }
    void SetOpt( T v ) { m_isNull = false; m_opt = v; m_hasOpt = true; }

    bool IsNull() const { return m_isNull; }

private:
    bool m_isNull = true;
    T    m_min{};
    T    m_opt{};
    T    m_max{};
    bool m_hasMin = false;
    bool m_hasOpt = false;
    bool m_hasMax = false;
};

#endif
