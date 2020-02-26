/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef KICAD_DRC_TREE_MODEL_H
#define KICAD_DRC_TREE_MODEL_H

#include <drc/drc.h>
#include <wx/wupdlock.h>


/**
 * Provide an abstract interface of a DRC_ITEM* list manager.  The details
 * of the actual list architecture are hidden from the caller.  Any class
 * that implements this interface can then be used by a DRC_TREE_MODEL class without
 * it knowing the actual architecture of the list.
 */
class DRC_ITEMS_PROVIDER
{
public:
    virtual int GetCount() = 0;

    /**
     * Function GetItem
     * retrieves a DRC_ITEM by pointer.  The actual item remains owned by the
     * list container.
     * @param aIndex The 0 based index into the list of the desired item.
     * @return const DRC_ITEM* - the desired item or NULL if aIndex is out of range.
     */
    virtual const DRC_ITEM* GetItem( int aIndex ) = 0;

    /**
     * Function DeleteItems
     * removes and deletes desired item from the list.
     * @param aIndex The 0 based index into the list of the desired item which
     *         is to be deleted.
     */
    virtual void DeleteItem( int aIndex ) = 0;

    /**
     * Function DeleteAllItems
     * removes and deletes all the items in the list.
     */
    virtual void DeleteAllItems() = 0;

    virtual ~DRC_ITEMS_PROVIDER() { }
};

/**
 * BOARD_DRC_ITEMS_PROVIDER
 * is an implementation of the interface named DRC_ITEM_LIST which uses a BOARD instance
 * to fulfill the interface.  No ownership is taken of the BOARD.
 */
class BOARD_DRC_ITEMS_PROVIDER : public DRC_ITEMS_PROVIDER
{
    BOARD* m_board;

public:
    BOARD_DRC_ITEMS_PROVIDER( BOARD* aBoard ) :
            m_board( aBoard )
    {
    }

    int  GetCount() override
    {
        return m_board->GetMARKERCount();
    }

    const DRC_ITEM* GetItem( int aIndex ) override
    {
        const MARKER_PCB* marker = m_board->GetMARKER( aIndex );

        return marker ? &marker->GetReporter() : nullptr;
    }

    void DeleteItem( int aIndex ) override
    {
        MARKER_PCB* marker = m_board->GetMARKER( aIndex );

        if( marker )
            m_board->Delete( marker );
    }

    void DeleteAllItems() override
    {
        m_board->DeleteMARKERs();
    }
};


/**
 * VECTOR_DRC_ITEMS_PROVIDER
 * is an implementation of the interface named DRC_ITEMS_PROVIDER which uses a vector
 * of pointers to DRC_ITEMs to fulfill the interface.  No ownership is taken of the
 * vector.
 */
class VECTOR_DRC_ITEMS_PROVIDER : public DRC_ITEMS_PROVIDER
{
    std::vector<DRC_ITEM*>* m_vector;

public:

    VECTOR_DRC_ITEMS_PROVIDER( std::vector<DRC_ITEM*>* aList ) :
            m_vector( aList )
    {
    }

    int  GetCount() override
    {
        return m_vector ? (int) m_vector->size() : 0;
    }

    const DRC_ITEM* GetItem( int aIndex ) override
    {
        return (*m_vector)[aIndex];
    }

    void DeleteItem( int aIndex ) override
    {
        delete (*m_vector)[aIndex];
        m_vector->erase( m_vector->begin() + aIndex );
    }

    void DeleteAllItems() override
    {
        if( m_vector )
        {
            for( DRC_ITEM* item : *m_vector )
                delete item;

            m_vector->clear();
        }
    }
};


class DRC_TREE_NODE
{
public:
    enum NODE_TYPE { MARKER, MAIN_ITEM, AUX_ITEM };

    DRC_TREE_NODE( DRC_TREE_NODE* aParent, const DRC_ITEM* aDrcItem, NODE_TYPE aType ) :
            m_Type( aType ),
            m_Parent( aParent ),
            m_DrcItem( aDrcItem )
    {}

    NODE_TYPE                  m_Type;
    DRC_TREE_NODE*             m_Parent;

    const DRC_ITEM*            m_DrcItem;
    std::vector<DRC_TREE_NODE> m_Children;
};


class DRC_TREE_MODEL : public wxDataViewModel
{
public:
    static wxDataViewItem ToItem( DRC_TREE_NODE const* aNode )
    {
        return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
    }

    static DRC_TREE_NODE* ToNode( wxDataViewItem aItem )
    {
        return static_cast<DRC_TREE_NODE*>( aItem.GetID() );
    }

    static BOARD_ITEM* ToBoardItem( BOARD* aBoard, wxDataViewItem aItem )
    {
        BOARD_ITEM*          board_item = nullptr;
        const DRC_TREE_NODE* node = DRC_TREE_MODEL::ToNode( aItem );

        if( node )
        {
            const DRC_ITEM* drc_item = node->m_DrcItem;

            switch( node->m_Type )
            {
            case DRC_TREE_NODE::MARKER:
                board_item = static_cast<MARKER_PCB*>( drc_item->GetParent() );
                break;
            case DRC_TREE_NODE::MAIN_ITEM:
                board_item = drc_item->GetMainItem( aBoard );
                break;
            case DRC_TREE_NODE::AUX_ITEM:
                board_item = drc_item->GetAuxiliaryItem( aBoard );
                break;
            }
        }

        return board_item;
    };

public:
    DRC_TREE_MODEL( wxDataViewCtrl* aView ) :
        m_view( aView ),
        m_drcItemsProvider( nullptr )
    { }

    ~DRC_TREE_MODEL()
    {
        delete m_drcItemsProvider;
    }

    void SetProvider( DRC_ITEMS_PROVIDER* aProvider )
    {
        Cleared();

        delete m_drcItemsProvider;
        m_drcItemsProvider = aProvider;
        m_tree.clear();

        for( int i = 0; m_drcItemsProvider && i < m_drcItemsProvider->GetCount(); ++i )
        {
            const DRC_ITEM* drcItem = m_drcItemsProvider->GetItem( i );

            m_tree.emplace_back( nullptr, drcItem, DRC_TREE_NODE::MARKER );
            DRC_TREE_NODE& node = m_tree.back();

            node.m_Children.emplace_back( &node, drcItem, DRC_TREE_NODE::MAIN_ITEM );

            if( drcItem->HasSecondItem() )
                node.m_Children.emplace_back( &node, drcItem, DRC_TREE_NODE::AUX_ITEM );

            wxDataViewItemArray childItems;

            for( DRC_TREE_NODE& child : node.m_Children )
                childItems.Add( ToItem( &child ) );

            ItemAdded( ToItem( nullptr ), ToItem( &node ) );
            ItemsAdded( ToItem( &node ), childItems );
        }

        ExpandAll();
    }

    int GetDRCItemCount() const { return m_tree.size(); }

    const DRC_ITEM* GetDRCItem( int i ) const { return m_tree.at( i ).m_DrcItem; }

    void ExpandAll()
    {
        for( DRC_TREE_NODE& markerNode : m_tree )
            m_view->Expand( ToItem( &markerNode ) );
    }

    bool IsContainer( wxDataViewItem const& aItem ) const override
    {
        if( ToNode( aItem ) == nullptr )    // must be tree root...
            return true;
        else
            return ToNode( aItem )->m_Type == DRC_TREE_NODE::MARKER;
    }

    wxDataViewItem GetParent( wxDataViewItem const& aItem ) const override
    {
        return ToItem( ToNode( aItem)->m_Parent );
    }

    unsigned int GetChildren( wxDataViewItem const& aItem,
                              wxDataViewItemArray&  aChildren ) const override
    {
        const DRC_TREE_NODE*              parent = ToNode( aItem );
        const std::vector<DRC_TREE_NODE>& children = parent ? parent->m_Children : m_tree;

        for( const DRC_TREE_NODE& child: children )
            aChildren.Add( ToItem( &child ) );

        return children.size();
    }

    // Simple, single-text-column model
    unsigned int GetColumnCount() const override { return 1; }
    wxString GetColumnType( unsigned int aCol ) const override { return "string"; }
    bool HasContainerColumns( wxDataViewItem const& aItem ) const override { return true; }

    /**
     * Called by the wxDataView to fetch an item's value.
     */
    void GetValue( wxVariant&              aVariant,
                   wxDataViewItem const&   aItem,
                   unsigned int            aCol ) const override
    {
        const DRC_TREE_NODE* node = ToNode( aItem );
        wxASSERT( node );

        switch( node->m_Type )
        {
        case DRC_TREE_NODE::MARKER:    aVariant = node->m_DrcItem->GetErrorText();     break;
        case DRC_TREE_NODE::MAIN_ITEM: aVariant = node->m_DrcItem->GetMainText();      break;
        case DRC_TREE_NODE::AUX_ITEM:  aVariant = node->m_DrcItem->GetAuxiliaryText(); break;
        }
    }

    /**
     * Called by the wxDataView to edit an item's content.
     */
    bool SetValue( wxVariant const& aVariant,
                   wxDataViewItem const&   aItem,
                   unsigned int            aCol ) override
    {
        // Editing not supported
        return false;
    }

    /**
     * Called by the wxDataView to fetch an item's formatting.  Return true iff the
     * item has non-default attributes.
     */
    bool GetAttr( wxDataViewItem const&   aItem,
                  unsigned int            aCol,
                  wxDataViewItemAttr&     aAttr ) const override
    {
        const DRC_TREE_NODE* node = ToNode( aItem );
        wxASSERT( node );

        switch( node->m_Type )
        {
        case DRC_TREE_NODE::MARKER:     aAttr.SetBold( true );  return true;
        case DRC_TREE_NODE::MAIN_ITEM:                          return false;
        case DRC_TREE_NODE::AUX_ITEM:                           return false;
        }

        return false;
    }

    void DeleteCurrentItem()
    {
        wxDataViewItem  dataViewItem = m_view->GetCurrentItem();
        DRC_TREE_NODE*  tree_node = ToNode( dataViewItem );
        const DRC_ITEM* drc_item = tree_node ? tree_node->m_DrcItem : nullptr;

        if( !drc_item )
        {
            wxBell();
            return;
        }

        for( int i = 0; i < m_drcItemsProvider->GetCount(); ++i )
        {
            if( m_drcItemsProvider->GetItem( i ) == drc_item )
            {
                m_drcItemsProvider->DeleteItem( i );
                m_tree.erase( m_tree.begin() + i );

                ItemDeleted( ToItem( nullptr ), dataViewItem );
                break;
            }
        }
    }

    void DeleteAllItems()
    {
        if( m_drcItemsProvider )
        {
            m_drcItemsProvider->DeleteAllItems();

            m_tree.clear();
            Cleared();
        }
    }

private:
    wxDataViewCtrl*             m_view;
    DRC_ITEMS_PROVIDER*         m_drcItemsProvider; // I own this, but not its contents
    std::vector<DRC_TREE_NODE>  m_tree;             // I own this
};

#endif //KICAD_DRC_TREE_MODEL_H
