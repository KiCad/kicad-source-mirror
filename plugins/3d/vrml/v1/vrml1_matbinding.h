/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file vrml1_matbinding.h
 */


#ifndef VRML1_MATBINDING_H
#define VRML1_MATBINDING_H

#include <vector>
#include "vrml1_node.h"

class WRL1BASE;
class SGNODE;

/**
 * Class WRL1MATBINDING
 */
class WRL1MATBINDING : public WRL1NODE
{
private:
    WRL1_BINDING m_binding;

public:
    WRL1MATBINDING( NAMEREGISTER* aDictionary );
    WRL1MATBINDING( NAMEREGISTER* aDictionary, WRL1NODE* aParent );
    virtual ~WRL1MATBINDING();

    // functions inherited from WRL1NODE
    bool Read( WRLPROC& proc, WRL1BASE* aTopNode ) override;
    bool AddRefNode( WRL1NODE* aNode ) override;
    bool AddChildNode( WRL1NODE* aNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent, WRL1STATUS* sp ) override;
};

#endif  // VRML1_MATBINDING_H
