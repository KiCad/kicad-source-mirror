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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef IO_MGR_H_
#define IO_MGR_H_

#include <memory>

/**
 * Helper to hold and release an #IO_BASE object when exceptions are thrown.
 *
 * (Wrapper around a std::unique_ptr, but created to allow possible custom deleters
 *  in the future).
 */
template <class T>
using IO_RELEASER = std::unique_ptr<T>;


class IO_MGR
{
public:

};


#endif // IO_MGR_H_
