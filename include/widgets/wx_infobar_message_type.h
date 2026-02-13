/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
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

#ifndef WX_INFOBAR_MESSAGE_TYPE_H
#define WX_INFOBAR_MESSAGE_TYPE_H

/**
 * Sets the type of message for special handling if needed
 */
enum class INFOBAR_MESSAGE_TYPE
{
    GENERIC,          ///< GENERIC Are messages that do not have special handling
    OUTDATED_SAVE,    ///< OUTDATED_SAVE Messages that should be cleared on save
    DRC_RULES_ERROR,
    DRC_VIOLATION
};

#endif // WX_INFOBAR_MESSAGE_TYPE_H
