/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
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

#ifndef FP_TREE_MODEL_ADAPTER_H
#define FP_TREE_MODEL_ADAPTER_H

#include <lib_tree_model_adapter.h>
#include <footprint_info.h>

class FOOTPRINT_LIBRARY_ADAPTER;
class PCB_BASE_FRAME;

class FP_TREE_MODEL_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    /**
     * Factory function: create a model adapter in a reference-counting container.
     *
     * @param aLibs library set from which parts will be loaded
     */
    static wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> Create( PCB_BASE_FRAME* aParent,
                                                           FOOTPRINT_LIBRARY_ADAPTER* aLibs );

    void AddLibraries( EDA_BASE_FRAME* aParent );

    wxString GenerateInfo( LIB_ID const& aLibId, int aUnit ) override;

protected:
    /**
     * Constructor; takes a set of libraries to be included in the search.
     */
    FP_TREE_MODEL_ADAPTER( PCB_BASE_FRAME* aParent, FOOTPRINT_LIBRARY_ADAPTER* aLibs );

    std::vector<LIB_TREE_ITEM*> getFootprints( const wxString& aLibName );

    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::LIB_TYPE_T::FOOTPRINT_LIB; }

protected:
    FOOTPRINT_LIBRARY_ADAPTER* m_libs;
};

#endif // FP_TREE_MODEL_ADAPTER_H
