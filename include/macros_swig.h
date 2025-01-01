/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file macros_swig.h
 * @brief This file contains macros just for swig binding
 */

#ifndef MACROS_SWIG_H
#define MACROS_SWIG_H

#include <deque>
#include <vector>
#include <map>
#include <set>
#include <memory>       // std::shared_ptr


#ifdef SWIG
/// Declare a std::vector and also the swig %template in unison
#define DECL_VEC_FOR_SWIG(TypeName, MemberType) namespace std { %template(TypeName) vector<MemberType>; } typedef std::vector<MemberType> TypeName;
#define DECL_DEQ_FOR_SWIG(TypeName, MemberType) namespace std { %template(TypeName) deque<MemberType>; } typedef std::deque<MemberType> TypeName;
#define DECL_MAP_FOR_SWIG(TypeName, KeyType, ValueType) namespace std { %template(TypeName) map<KeyType, ValueType>; } typedef std::map<KeyType, ValueType> TypeName;
#define DECL_SPTR_FOR_SWIG(TypeName, MemberType) %shared_ptr(MemberType) namespace std { %template(TypeName) shared_ptr<MemberType>; } typedef std::shared_ptr<MemberType> TypeName;
#define DECL_SET_FOR_SWIG(TypeName, MemberType) namespace std { %template(TypeName) set<MemberType>; } typedef std::set<MemberType> TypeName;
#else
/// Declare a std::vector but no swig %template
#define DECL_VEC_FOR_SWIG(TypeName, MemberType) typedef std::vector<MemberType> TypeName;
#define DECL_DEQ_FOR_SWIG(TypeName, MemberType) typedef std::deque<MemberType> TypeName;
#define DECL_MAP_FOR_SWIG(TypeName, KeyType, ValueType) typedef std::map<KeyType, ValueType> TypeName;
#define DECL_SPTR_FOR_SWIG(TypeName, MemberType) typedef std::shared_ptr<MemberType> TypeName;
#define DECL_SET_FOR_SWIG(TypeName, MemberType) typedef std::set<MemberType> TypeName;
#endif

#endif // MACROS_SWIG_H
