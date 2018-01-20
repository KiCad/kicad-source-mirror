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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LIB_MANAGER_ADAPTER_H
#define LIB_MANAGER_ADAPTER_H

#include <cmp_tree_model_adapter_base.h>
#include <map>

class LIB_MANAGER;

class LIB_MANAGER_ADAPTER : public CMP_TREE_MODEL_ADAPTER_BASE
{
public:
    static PTR Create( LIB_MANAGER* aLibs );

    void AddLibrary( const wxString& aLibNickname ) override;

    void AddAliasList( const wxString& aNodeName, const wxArrayString& aAliasNameList ) override;

    bool IsContainer( const wxDataViewItem& aItem ) const override;

    void Sync( bool aForce = false, std::function<void(int, int, const wxString&)> aProgressCallback
            = [](int, int, const wxString&){} );

    int GetLibrariesCount() const override;

protected:
    void updateLibrary( CMP_TREE_NODE_LIB& aLibNode );

    CMP_TREE_NODE::PTR_VECTOR::iterator deleteLibrary(
            CMP_TREE_NODE::PTR_VECTOR::iterator& aLibNodeIt );

    CMP_TREE_NODE* findLibrary( const wxString& aLibNickName );

    void GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
            unsigned int aCol ) const override;
    bool GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
            wxDataViewItemAttr& aAttr ) const override;

    LIB_MANAGER_ADAPTER( LIB_MANAGER* aLibMgr );

    LIB_MANAGER* m_libMgr;

    ///> Hashes to decide whether a library needs an update
    std::map<wxString, int> m_libHashes;

    ///> LIB_MANAGER hash value returned in the last synchronization
    int m_lastSyncHash;
};

#endif /* LIB_MANAGER_ADAPTER_H */
