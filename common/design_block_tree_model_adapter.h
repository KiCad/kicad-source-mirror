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

#ifndef DESIGN_BLOCK_TREE_MODEL_ADAPTER_H
#define DESIGN_BLOCK_TREE_MODEL_ADAPTER_H

#include <lib_tree_model_adapter.h>

class DESIGN_BLOCK_LIBRARY_ADAPTER;

class DESIGN_BLOCK_TREE_MODEL_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    /**
     * Factory function: create a model adapter in a reference-counting container.
     *
     * @param aLibs library set from which parts will be loaded
     */
    static wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> Create( EDA_BASE_FRAME* aParent, DESIGN_BLOCK_LIBRARY_ADAPTER* aLibs,
                                                           APP_SETTINGS_BASE::LIB_TREE& aSettings,
                                                           TOOL_INTERACTIVE*            aContextMenuTool );

    void AddLibraries( EDA_BASE_FRAME* aParent );
    void ClearLibraries();

    wxString GenerateInfo( LIB_ID const& aLibId, int aUnit ) override;

    TOOL_INTERACTIVE* GetContextMenuTool() override;

protected:
    /**
     * Constructor; takes a set of libraries to be included in the search.
     */
    DESIGN_BLOCK_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, DESIGN_BLOCK_LIBRARY_ADAPTER* aLibs,
                                     APP_SETTINGS_BASE::LIB_TREE& aSettings,
                                     TOOL_INTERACTIVE* aContextMenuTool );

    std::vector<LIB_TREE_ITEM*> getDesignBlocks( EDA_BASE_FRAME* aParent, const wxString& aLibName );

    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::LIB_TYPE_T::DESIGN_BLOCK_LIB; }

protected:
    DESIGN_BLOCK_LIBRARY_ADAPTER* m_libs;
    EDA_BASE_FRAME*         m_frame;
    TOOL_INTERACTIVE*       m_contextMenuTool;
};

#endif // DESIGN_BLOCK_TREE_MODEL_ADAPTER_H
