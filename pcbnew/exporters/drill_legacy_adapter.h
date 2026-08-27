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

#ifndef DRILL_LEGACY_ADAPTER_H
#define DRILL_LEGACY_ADAPTER_H

#include <vector>

#include <drill/drill_operation.h>
#include <gendrill_writer_base.h>


/**
 * Project canonical operations onto the drill writers' record.
 *
 * One operation yields one HOLE_INFO, so hole counts and tool assignments are unchanged.
 * m_Tool_Reference is left at -1 for the writer to assign after its own sort.
 */
std::vector<HOLE_INFO> ToLegacyHoleList( const std::vector<DRILL_OPERATION>& aOperations );

#endif // DRILL_LEGACY_ADAPTER_H
