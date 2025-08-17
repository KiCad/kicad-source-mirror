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

#ifndef SYMBOL_TREE_MODEL_ADAPTER_H
#define SYMBOL_TREE_MODEL_ADAPTER_H

#include <lib_tree_model_adapter.h>

class SYMBOL_LIBRARY_ADAPTER;
class SCH_BASE_FRAME;

class SYMBOL_TREE_MODEL_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    /**
     * Destructor.
     *
     * @warning Do **not** delete this class manually.  It is reference-counted by wxObject.
     */
    ~SYMBOL_TREE_MODEL_ADAPTER();

    /**
     * Factory function: create a model adapter in a reference-counting container.
     *
     * @param aLibs library set from which parts will be loaded
     */
    static wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> Create( SCH_BASE_FRAME* aParent,
                                                           SYMBOL_LIBRARY_ADAPTER* aLibs );

    /**
     * Add all the libraries in a SYMBOL_LIB_TABLE to the model.
     *
     * @param aFrame is the parent window to display the progress dialog
     */
    void AddLibraries( SCH_BASE_FRAME* aFrame );

    void AddLibrary( wxString const& aLibNickname, bool pinned );

    wxString GenerateInfo( LIB_ID const& aLibId, int aUnit ) override;

protected:
    /**
     * Constructor; takes a set of libraries to be included in the search.
     */
    SYMBOL_TREE_MODEL_ADAPTER( SCH_BASE_FRAME* aParent, SYMBOL_LIBRARY_ADAPTER* aManager );

    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::LIB_TYPE_T::SYMBOL_LIB; }

private:
    /**
     * Flag to only show the symbol library table load progress dialog the first time.
     */
    static bool        m_show_progress;

    SYMBOL_LIBRARY_ADAPTER*  m_adapter;

    std::set<wxString> m_pending_load_libraries;
    std::unique_ptr<wxTimer> m_check_pending_libraries_timer;
};

#endif // SYMBOL_TREE_MODEL_ADAPTER_H
