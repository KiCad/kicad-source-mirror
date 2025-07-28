/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SHEET_SYNCHRONIZATION_MODEL_H
#define SHEET_SYNCHRONIZATION_MODEL_H

#include <sch_sheet_path.h>
#include <memory>
#include <optional>
#include <list>
#include <wx/dataview.h>
#include <wx/string.h>

class SHEET_SYNCHRONIZATION_ITEM;
using SHEET_SYNCHRONIZATION_ITE_PTR = std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM>;
using SHEET_SYNCHRONIZATION_ITEM_LIST = std::vector<SHEET_SYNCHRONIZATION_ITE_PTR>;

class SHEET_SYNCHRONIZATION_NOTIFIER;
class SHEET_SYNCHRONIZATION_AGENT;
class SCH_SHEET;
class SCH_SHEET_PATH;

class SHEET_SYNCHRONIZATION_MODEL : public wxDataViewVirtualListModel
{
public:
    enum SHEET_SYNCHRONIZATION_COL
    {
        NAME,
        SHAPE,
        COL_COUNT
    };

    enum
    {
        HIRE_LABEL,
        SHEET_PIN,
        ASSOCIATED,
        MODEL_COUNT
    };

    static wxString GetColName( int col )
    {
        switch( col )
        {
        case NAME: return _( "Name" );
        case SHAPE: return _( "Shape" );
        default: return {};
        }
    }


    SHEET_SYNCHRONIZATION_MODEL( SHEET_SYNCHRONIZATION_AGENT& aAgent, SCH_SHEET* aSheet,
                                 const SCH_SHEET_PATH& aPath );
    ~SHEET_SYNCHRONIZATION_MODEL() override;

    void GetValueByRow( wxVariant& variant, unsigned row, unsigned col ) const override;

    bool SetValueByRow( const wxVariant& variant, unsigned row, unsigned col ) override;

    bool GetAttrByRow( unsigned row, unsigned int col, wxDataViewItemAttr& attr ) const override;

    void RemoveItems( wxDataViewItemArray const& aItems );

    /**
     * Add a new item, the notifiers are notified.
     */
    bool AppendNewItem( std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM> aItem );

    /**
     * Just append item to the list, the notifiers are not notified.
     */
    bool AppendItem( std::shared_ptr<SHEET_SYNCHRONIZATION_ITEM> aItem );

    SHEET_SYNCHRONIZATION_ITEM_LIST TakeItems( wxDataViewItemArray const& aItems );

    SHEET_SYNCHRONIZATION_ITE_PTR TakeItem( wxDataViewItem const& aItem );

    SHEET_SYNCHRONIZATION_ITE_PTR GetSynchronizationItem( unsigned aIndex ) const;

    SHEET_SYNCHRONIZATION_ITE_PTR GetSynchronizationItem( wxDataViewItem const& aItem ) const;

    void OnRowSelected( std::optional<unsigned> aRow );

    void UpdateItems( SHEET_SYNCHRONIZATION_ITEM_LIST aItems );

    void AddNotifier( std::shared_ptr<SHEET_SYNCHRONIZATION_NOTIFIER> aNotifier );

    void DoNotify();

    bool HasSelectedIndex() const { return m_selectedIndex.has_value(); }

    std::optional<unsigned int> GetSelectedIndex() const { return m_selectedIndex; }

    unsigned int GetCount() const override;


private:
    SHEET_SYNCHRONIZATION_ITEM_LIST                            m_items;
    std::optional<unsigned>                                    m_selectedIndex;
    std::list<std::shared_ptr<SHEET_SYNCHRONIZATION_NOTIFIER>> m_notifiers;
    SHEET_SYNCHRONIZATION_AGENT&                               m_agent;
    SCH_SHEET*                                                 m_sheet;
    SCH_SHEET_PATH                                             m_path;
};

#endif
