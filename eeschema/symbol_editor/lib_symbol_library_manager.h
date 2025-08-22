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

#ifndef KICAD_LIB_SYMBOL_LIBRARY_MANAGER_H
#define KICAD_LIB_SYMBOL_LIBRARY_MANAGER_H

#include <symbol_library_manager.h>
#include <symbol_tree_synchronizing_adapter.h>


struct NEW_SYMBOL_PROPERTIES
{
    wxString name;
    wxString parentSymbolName;
    wxString reference;
    int      unitCount;
    bool     pinNameInside;
    int      pinTextPosition;
    bool     powerSymbol;
    bool     showPinNumber;
    bool     showPinName;
    bool     unitsInterchangeable;
    bool     includeInBom;
    bool     includeOnBoard;
    bool     alternateBodyStyle;
    bool     keepFootprint;
    bool     keepDatasheet;
    bool     transferUserFields;
    bool     keepContentUserFields;
};

/**
 * Symbol library management helper that is specific to the symbol library editor frame
 *
 * The base class handles library manipulation; this one also handles synchronizing the LIB_TREE.
 */
class LIB_SYMBOL_LIBRARY_MANAGER : public SYMBOL_LIBRARY_MANAGER
{
public:
    LIB_SYMBOL_LIBRARY_MANAGER( SYMBOL_EDIT_FRAME& aFrame );

    /**
     * Updates the #SYMBOL_LIBRARY_MANAGER data to synchronize with Symbol Library Table.
     */
    void Sync( const wxString& aForceRefresh,
               std::function<void( int, int, const wxString& )> aProgressCallback );

    static std::unique_ptr<LIB_SYMBOL> CreateSymbol( const NEW_SYMBOL_PROPERTIES& aProps,
                                                     LIB_SYMBOL* aParent );

    bool CreateNewSymbol( const wxString& aLibrary, const NEW_SYMBOL_PROPERTIES& aProps );

    /**
     * Return the adapter object that provides the stored data.
     */
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& GetAdapter() { return m_adapter; }

protected:
    void OnDataChanged() const override;

private:
    SYMBOL_TREE_SYNCHRONIZING_ADAPTER* getAdapter()
    {
        return static_cast<SYMBOL_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );
    }

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    int m_syncHash;     ///< Symbol lib table hash value from last synchronization
};


#endif
