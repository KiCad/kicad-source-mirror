/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file vrml1_matbinding.h
 */


#ifndef VRML1_MATBINDING_H
#define VRML1_MATBINDING_H

#include <vector>
#include "vrml1_node.h"

class WRL1BASE;
class SGNODE;

class WRL1MATBINDING : public WRL1NODE
{
public:
    WRL1MATBINDING( NAMEREGISTER* aDictionary );
    WRL1MATBINDING( NAMEREGISTER* aDictionary, WRL1NODE* aParent );
    virtual ~WRL1MATBINDING();

    bool Read( WRLPROC& proc, WRL1BASE* aTopNode ) override;
    bool AddRefNode( WRL1NODE* aNode ) override;
    bool AddChildNode( WRL1NODE* aNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent, WRL1STATUS* sp ) override;

private:
    WRL1_BINDING m_binding;
};

#endif  // VRML1_MATBINDING_H
