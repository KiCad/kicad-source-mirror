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


#define WX_DATAVIEW_WINDOW_PADDING 6


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

    NODE_TYPE       m_Type;
    DRC_TREE_NODE*  m_Parent;

    const DRC_ITEM* m_DrcItem;

    std::vector<std::unique_ptr<DRC_TREE_NODE>> m_Children;
};


class DRC_TREE_MODEL : public wxDataViewModel, wxEvtHandler
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

    static BOARD_ITEM* ToBoardItem( BOARD* aBoard, wxDataViewItem aItem );

public:
    DRC_TREE_MODEL( wxDataViewCtrl* aView );

    ~DRC_TREE_MODEL();

    void SetProvider( DRC_ITEMS_PROVIDER* aProvider );

    int GetDRCItemCount() const { return m_tree.size(); }

    const DRC_ITEM* GetDRCItem( int i ) const { return m_tree.at( i )->m_DrcItem; }

    void ExpandAll();

    bool IsContainer( wxDataViewItem const& aItem ) const override;

    wxDataViewItem GetParent( wxDataViewItem const& aItem ) const override;

    unsigned int GetChildren( wxDataViewItem const& aItem,
                              wxDataViewItemArray&  aChildren ) const override;

    // Simple, single-text-column model
    unsigned int GetColumnCount() const override { return 1; }
    wxString GetColumnType( unsigned int aCol ) const override { return "string"; }
    bool HasContainerColumns( wxDataViewItem const& aItem ) const override { return true; }

    /**
     * Called by the wxDataView to fetch an item's value.
     */
    void GetValue( wxVariant&              aVariant,
                   wxDataViewItem const&   aItem,
                   unsigned int            aCol ) const override;

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
                  wxDataViewItemAttr&     aAttr ) const override;

    void DeleteCurrentItem();
    void DeleteAllItems();

    void onSizeView( wxSizeEvent& aEvent );

private:
    wxDataViewCtrl*      m_view;
    DRC_ITEMS_PROVIDER*  m_drcItemsProvider;   // I own this, but not its contents

    std::vector<std::unique_ptr<DRC_TREE_NODE>> m_tree;  // I own this
};

#endif //KICAD_DRC_TREE_MODEL_H
