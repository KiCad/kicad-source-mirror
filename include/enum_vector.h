/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEFINE_ENUM_VECTOR

/**
 * Macro to create const vectors containing enum values to enable easy iteration.
 *
 * Usage:
 * [header]
 * class A {
 *     DEFINE_ENUM_VECTOR( COLORS, { RED, GREEN, BLUE } );
 * };
 *
 * [source]
 * for( auto color : A::COLORS_vector ) {
 *     // do sth with color
 * }
 *
 * DECLARE_ENUM_VECTOR( COLORS );
 */
#define DEFINE_ENUM_VECTOR(enum_name, ...) \
    enum enum_name __VA_ARGS__; \
    static constexpr enum_name enum_name##_vector[] = __VA_ARGS__;

#define DECLARE_ENUM_VECTOR(class_name, enum_name) \
    constexpr class_name::enum_name class_name::enum_name##_vector[];

#endif
