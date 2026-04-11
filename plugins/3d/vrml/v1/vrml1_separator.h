/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

/**
 * @file vrml1_separator.h
 */


#ifndef VRML1_SEPARATOR_H
#define VRML1_SEPARATOR_H

#include "vrml1_node.h"

class WRL1BASE;
class SGNODE;

class WRL1SEPARATOR : public WRL1NODE
{
public:
    WRL1SEPARATOR( NAMEREGISTER* aDictionary );
    WRL1SEPARATOR( NAMEREGISTER* aDictionary, WRL1NODE* aNode );
    virtual ~WRL1SEPARATOR();

    bool Read( WRLPROC& proc, WRL1BASE* aTopNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent, WRL1STATUS* sp ) override;
};

#endif  // VRML1_SEPARATOR_H
