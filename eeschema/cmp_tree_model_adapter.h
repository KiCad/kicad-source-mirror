/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _CMP_TREE_MODEL_ADAPTER_H
#define _CMP_TREE_MODEL_ADAPTER_H

#include <cmp_tree_model_adapter_base.h>

class SYMBOL_LIB_TABLE;

class CMP_TREE_MODEL_ADAPTER: public CMP_TREE_MODEL_ADAPTER_BASE
{
public:

    /**
     * Reference-counting container for a pointer to CMP_TREE_MODEL_ADAPTER.
     */
    //typedef wxObjectDataPtr<CMP_TREE_MODEL_ADAPTER> PTR;

    /**
     * Destructor. Do NOT delete this class manually; it is reference-counted
     * by wxObject.
     */
    ~CMP_TREE_MODEL_ADAPTER();

    /**
     * Factory function: create a model adapter in a reference-counting
     * container.
     *
     * @param aLibs library set from which parts will be loaded
     */
    static PTR Create( SYMBOL_LIB_TABLE* aLibs );

    /**
     * Add all the components and their aliases in this library. To be called
     * in the setup phase.
     *
     * @param aLibNickname reference to a symbol library nickname
     */
    void AddLibrary( wxString const& aLibNickname ) override;

    /**
     * Add the given list of components, by name. To be called in the setup
     * phase.
     *
     * @param aNodeName         the parent node the components will appear under
     * @param aAliasNameList    list of alias names
     */
    void AddAliasList(
            wxString const&      aNodeName,
            wxArrayString const& aAliasNameList ) override;

    using CMP_TREE_MODEL_ADAPTER_BASE::AddAliasList;
protected:

    /**
     * Constructor; takes a set of libraries to be included in the search.
     */
    CMP_TREE_MODEL_ADAPTER( SYMBOL_LIB_TABLE* aLibs );

private:
    SYMBOL_LIB_TABLE*   m_libs;
};

#endif // _CMP_TREE_MODEL_ADAPTER_H
