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
#include <widgets/ui_common.h>


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
    virtual void SetSeverities( int aSeverities ) = 0;

    virtual int GetCount( int aSeverity = -1 ) = 0;

    /**
     * Function GetItem
     * retrieves a DRC_ITEM by pointer.  The actual item remains owned by the
     * list container.
     * @param aIndex The 0 based index into the list of the desired item.
     * @return const DRC_ITEM* - the desired item or NULL if aIndex is out of range.
     */
    virtual DRC_ITEM* GetItem( int aIndex ) = 0;

    /**
     * Function DeleteItems
     * removes and deletes desired item from the list.
     * @param aIndex The 0 based index into the list of the desired item which is to be deleted.
     * @param aDeep If true, the source item should be deleted as well as the filtered item.
     */
    virtual void DeleteItem( int aIndex, bool aDeep ) = 0;

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
private:
    BOARD*                   m_board;

    int                      m_severities;
    std::vector<MARKER_PCB*> m_filteredMarkers;

public:
    BOARD_DRC_ITEMS_PROVIDER( BOARD* aBoard ) :
            m_board( aBoard ),
            m_severities( 0 )
    {
    }

    void SetSeverities( int aSeverities ) override
    {
        m_severities = aSeverities;

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        m_filteredMarkers.clear();

        for( MARKER_PCB* marker : m_board->Markers() )
        {
            int markerSeverity;

            if( marker->IsExcluded() )
                markerSeverity = RPT_SEVERITY_EXCLUSION;
            else
                markerSeverity = bds.GetSeverity( marker->GetReporter().GetErrorCode() );

            if( markerSeverity & m_severities )
                m_filteredMarkers.push_back( marker );
        }
    }

    int GetCount( int aSeverity = -1 ) override
    {
        if( aSeverity < 0 )
            return m_filteredMarkers.size();

        int count = 0;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        for( MARKER_PCB* marker : m_board->Markers() )
        {
            int markerSeverity;

            if( marker->IsExcluded() )
                markerSeverity = RPT_SEVERITY_EXCLUSION;
            else
                markerSeverity = bds.GetSeverity( marker->GetReporter().GetErrorCode() );

            if( markerSeverity == aSeverity )
                count++;
        }

        return count;
    }

    DRC_ITEM* GetItem( int aIndex ) override
    {
        MARKER_PCB* marker = m_filteredMarkers[ aIndex ];

        return marker ? &marker->GetReporter() : nullptr;
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        MARKER_PCB* marker = m_filteredMarkers[ aIndex ];
        m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

        if( aDeep )
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
    PCB_BASE_FRAME*         m_frame;
    std::vector<DRC_ITEM*>* m_sourceVector;     // owns its DRC_ITEMs

    int                     m_severities;
    std::vector<DRC_ITEM*>  m_filteredVector;   // does not own its DRC_ITEMs

public:

    VECTOR_DRC_ITEMS_PROVIDER( PCB_BASE_FRAME* aFrame, std::vector<DRC_ITEM*>* aList ) :
            m_frame( aFrame ),
            m_sourceVector( aList ),
            m_severities( 0 )
    {
    }

    void SetSeverities( int aSeverities ) override
    {
        m_severities = aSeverities;

        BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

        m_filteredVector.clear();

        if( m_sourceVector )
        {
            for( DRC_ITEM* item : *m_sourceVector )
            {
                if( bds.GetSeverity( item->GetErrorCode() ) & aSeverities )
                    m_filteredVector.push_back( item );
            }
        }
    }

    int  GetCount( int aSeverity = -1 ) override
    {
        if( aSeverity < 0 )
            return m_filteredVector.size();

        int count = 0;
        BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

        if( m_sourceVector )
        {
            for( DRC_ITEM* item : *m_sourceVector )
            {
                if( bds.GetSeverity( item->GetErrorCode() ) == aSeverity )
                    count++;
            }
        }

        return count;
    }

    DRC_ITEM* GetItem( int aIndex ) override
    {
        return (m_filteredVector)[aIndex];
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        DRC_ITEM* item = m_filteredVector[aIndex];
        m_filteredVector.erase( m_filteredVector.begin() + aIndex );

        if( aDeep )
        {
            for( size_t i = 0; i < m_sourceVector->size(); ++i )
            {
                if( m_sourceVector->at( i ) == item )
                {
                    delete item;
                    m_sourceVector->erase( m_sourceVector->begin() + i );
                    break;
                }
            }
        }
    }

    void DeleteAllItems() override
    {
        if( m_sourceVector )
        {
            for( DRC_ITEM* item : *m_sourceVector )
                delete item;

            m_sourceVector->clear();
        }

        m_filteredVector.clear();   // no ownership of DRC_ITEM pointers
    }
};


/**
 * RATSNEST_DRC_ITEMS_PROVIDER
 */
class RATSNEST_DRC_ITEMS_PROVIDER : public VECTOR_DRC_ITEMS_PROVIDER
{
    // TODO: for now this is just a vector, but we need to map it to some board-level
    // data-structure so that deleting/excluding things can do a deep delete/exclusion
    // which will be reflected in the ratsnest....
public:
    RATSNEST_DRC_ITEMS_PROVIDER( PCB_BASE_FRAME* aFrame, std::vector<DRC_ITEM*>* aList ) :
            VECTOR_DRC_ITEMS_PROVIDER( aFrame, aList )
    { }
};


class DRC_TREE_NODE
{
public:
    enum NODE_TYPE { MARKER, MAIN_ITEM, AUX_ITEM };

    DRC_TREE_NODE( DRC_TREE_NODE* aParent, DRC_ITEM* aDrcItem, NODE_TYPE aType ) :
            m_Type( aType ),
            m_Parent( aParent ),
            m_DrcItem( aDrcItem )
    {}

    NODE_TYPE      m_Type;
    DRC_TREE_NODE* m_Parent;

    DRC_ITEM*      m_DrcItem;

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
    DRC_TREE_MODEL( PCB_BASE_FRAME* aParentFrame, wxDataViewCtrl* aView );

    ~DRC_TREE_MODEL();

    void SetProvider( DRC_ITEMS_PROVIDER* aProvider );
    void SetSeverities( int aSeverities );

    int GetDRCItemCount() const { return m_tree.size(); }

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

    void ValueChanged( DRC_TREE_NODE* aNode );

    void DeleteCurrentItem( bool aDeep );
    void DeleteAllItems();

private:
    void rebuildModel( DRC_ITEMS_PROVIDER* aProvider, int aSeverities );
    void onSizeView( wxSizeEvent& aEvent );

private:
    PCB_BASE_FRAME*      m_parentFrame;
    wxDataViewCtrl*      m_view;
    int                  m_severities;
    DRC_ITEMS_PROVIDER*  m_drcItemsProvider;   // I own this, but not its contents

    std::vector<std::unique_ptr<DRC_TREE_NODE>> m_tree;  // I own this
};


#endif //KICAD_DRC_TREE_MODEL_H
