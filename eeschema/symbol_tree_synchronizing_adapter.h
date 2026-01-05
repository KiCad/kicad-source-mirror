/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <lib_tree_model_adapter.h>
#include <map>

class SYMBOL_EDIT_FRAME;
class SYMBOL_LIBRARY_MANAGER;

class SYMBOL_TREE_SYNCHRONIZING_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    static wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> Create( SYMBOL_EDIT_FRAME* aParent, SYMBOL_LIBRARY_MANAGER* aLibs );

    bool IsContainer( const wxDataViewItem& aItem ) const override;

    void Sync( const wxString& aForceRefresh, std::function<void( int, int, const wxString&)> aProgressCallback );

    int GetLibrariesCount() const override;

    TOOL_INTERACTIVE* GetContextMenuTool() override;

    wxDataViewItem GetCurrentDataViewItem() override;

    bool HasPreview( const wxDataViewItem& aItem ) override;
    void ShowPreview( wxWindow* aParent, const wxDataViewItem& aItem ) override;
    void ShutdownPreview( wxWindow* aParent ) override;

protected:
    void updateLibrary( LIB_TREE_NODE_LIBRARY& aLibNode );

    LIB_TREE_NODE::PTR_VECTOR::iterator deleteLibrary( LIB_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt );

    void GetValue( wxVariant& aVariant, wxDataViewItem const& aItem, unsigned int aCol ) const override;
    bool GetAttr( wxDataViewItem const& aItem, unsigned int aCol, wxDataViewItemAttr& aAttr ) const override;

    SYMBOL_TREE_SYNCHRONIZING_ADAPTER( SYMBOL_EDIT_FRAME* aParent, SYMBOL_LIBRARY_MANAGER* aLibMgr );

    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::LIB_TYPE_T::SYMBOL_LIB; }

protected:
    SYMBOL_EDIT_FRAME*      m_frame;
    SYMBOL_LIBRARY_MANAGER* m_libMgr;

    /// Hashes to decide whether a library needs an update.
    std::map<wxString, int> m_libHashes;

    /// #SYMBOL_LIBRARY_MANAGER hash value returned in the last synchronization.
    int                     m_lastSyncHash;
};
