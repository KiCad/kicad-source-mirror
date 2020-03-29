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

#ifndef RC_ITEM_H
#define RC_ITEM_H

#include <wx/dataview.h>
#include <macros.h>
#include <base_struct.h>

class MARKER_BASE;
class EDA_BASE_FRAME;
class RC_ITEM;


/**
 * Provide an abstract interface of a RC_ITEM* list manager.
 * The details of the actual list architecture are hidden from the caller.  Any class that
 * implements this interface can then be used by a RC_TREE_MODEL class without it knowing
 * the actual architecture of the list.
 */
class RC_ITEMS_PROVIDER
{
public:
    virtual void SetSeverities( int aSeverities ) = 0;

    virtual int GetCount( int aSeverity = -1 ) = 0;

    /**
     * Function GetItem
     * retrieves a RC_ITEM by index.
     */
    virtual RC_ITEM* GetItem( int aIndex ) = 0;

    /**
     * Function DeleteItem
     * removes (and optionally deletes) the indexed item from the list.
     * @param aDeep If true, the source item should be deleted as well as its entry in the list.
     */
    virtual void DeleteItem( int aIndex, bool aDeep ) = 0;

    /**
     * Function DeleteAllItems
     * removes and deletes all the items in the list.
     */
    virtual void DeleteAllItems() = 0;

    virtual ~RC_ITEMS_PROVIDER() { }
};


/**
 * RC_ITEM
 * is a holder for a DRC (in Pcbnew) or ERC (in Eeschema) error item.
 * There are holders for information on two EDA_ITEMs.  Some errors involve only one item
 * (item with an incorrect param) so m_hasSecondItem is set to false in this case.
 */
class RC_ITEM
{
protected:
    int           m_ErrorCode;         // the error code's numeric value
    wxString      m_MainText;          // text for the first EDA_ITEM
    wxString      m_AuxText;           // text for the second EDA_ITEM
    wxPoint       m_MainPosition;      // the location of the first EDA_ITEM
    wxPoint       m_AuxPosition;       // the location of the second EDA_ITEM
    bool          m_hasSecondItem;     // true when 2 items create a DRC/ERC error
    MARKER_BASE*  m_parent;            // The marker this item belongs to, if any
    KIID          m_mainItemUuid;
    KIID          m_auxItemUuid;

public:

    RC_ITEM()
    {
        m_ErrorCode     = 0;
        m_hasSecondItem = false;
        m_parent        = nullptr;
        m_mainItemUuid  = niluuid;
        m_auxItemUuid   = niluuid;
    }

    RC_ITEM( RC_ITEM* aItem )
    {
        m_ErrorCode = aItem->m_ErrorCode;
        m_MainText = aItem->m_MainText;
        m_AuxText = aItem->m_AuxText;
        m_MainPosition = aItem->m_MainPosition;
        m_AuxPosition = aItem->m_AuxPosition;
        m_hasSecondItem = aItem->m_hasSecondItem;
        m_parent = aItem->m_parent;
        m_mainItemUuid = aItem->m_mainItemUuid;
        m_auxItemUuid = aItem->m_auxItemUuid;
    }

    virtual ~RC_ITEM() { }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainItem = the first (main) schematic or board item
     * @param bAuxItem = the second schematic or board item
     */
    void SetData( EDA_UNITS aUnits, int aErrorCode,
                  EDA_ITEM* aMainItem,
                  EDA_ITEM* bAuxItem = nullptr )
    {
        m_ErrorCode       = aErrorCode;
        m_MainText        = aMainItem->GetSelectMenuText( aUnits );
        m_AuxText         = wxEmptyString;
        m_hasSecondItem   = bAuxItem != nullptr;
        m_parent          = nullptr;
        m_mainItemUuid    = aMainItem->m_Uuid;

        if( m_hasSecondItem )
        {
            m_AuxText     = bAuxItem->GetSelectMenuText( aUnits );
            m_auxItemUuid = bAuxItem->m_Uuid;
        }
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainItem = the first (main) schematic or board item
     * @param bAuxItem = the second schematic or board item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxPos = position the second item
     */
    void SetData( EDA_UNITS aUnits, int aErrorCode,
                  EDA_ITEM* aMainItem, const wxPoint& aMainPos,
                  EDA_ITEM* bAuxItem = nullptr, const wxPoint& bAuxPos = wxPoint() )
    {
        m_ErrorCode       = aErrorCode;
        m_MainText        = aMainItem->GetSelectMenuText( aUnits );
        m_AuxText         = wxEmptyString;
        m_MainPosition    = aMainPos;
        m_AuxPosition     = bAuxPos;
        m_hasSecondItem   = bAuxItem != nullptr;
        m_parent          = nullptr;
        m_mainItemUuid    = aMainItem->m_Uuid;

        if( m_hasSecondItem )
        {
            m_AuxText     = bAuxItem->GetSelectMenuText( aUnits );
            m_auxItemUuid = bAuxItem->m_Uuid;
        }
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = a description of the first (main) item
     * @param bAuxText = a description of the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText,
                  const wxString& bAuxText = wxEmptyString )
    {
        m_ErrorCode     = aErrorCode;
        m_MainText      = aMainText;
        m_AuxText       = bAuxText;
        m_hasSecondItem = !bAuxText.IsEmpty();
        m_parent        = nullptr;
        m_mainItemUuid  = niluuid;
        m_auxItemUuid   = niluuid;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = a description of the first (main) item
     * @param aMainPos = position the first item and therefore of this issue
     * @param bAuxText = a description of the second item
     * @param bAuxPos = position the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText, const wxPoint& aMainPos,
                  const wxString& bAuxText = wxEmptyString, const wxPoint& bAuxPos = wxPoint() )
    {
        m_ErrorCode     = aErrorCode;
        m_MainText      = aMainText;
        m_AuxText       = bAuxText;
        m_MainPosition  = aMainPos;
        m_AuxPosition   = bAuxPos;
        m_hasSecondItem = !bAuxText.IsEmpty();
        m_parent        = nullptr;
        m_mainItemUuid  = niluuid;
        m_auxItemUuid   = niluuid;
    }

    /**
     * Function SetData
     * initialize all data in item
     * @param aErrorCode = error code
     * @param aMainText = a description of the first (main) item
     * @param aMainID = UUID of the main item
     * @param bAuxText = a description of the second item
     * @param bAuxID = UUID of the second item
     */
    void SetData( int aErrorCode,
                  const wxString& aMainText, const KIID& aMainID,
                  const wxString& bAuxText, const KIID& bAuxID )
    {
        m_ErrorCode     = aErrorCode;
        m_MainText      = aMainText;
        m_AuxText       = bAuxText;
        m_hasSecondItem = !bAuxText.IsEmpty() || bAuxID != niluuid;
        m_parent        = nullptr;
        m_mainItemUuid  = aMainID;
        m_auxItemUuid   = bAuxID;
    }

    /**
     * Function SetAuxiliaryData
     * initialize data for the second (auxiliary) item
     * @param aAuxiliaryText = the second text (main text) concerning the second schematic
     *                         or board item
     * @param aAuxiliaryPos = position the second item
     */
    void SetAuxiliaryData( const wxString& aAuxiliaryText, const wxPoint& aAuxiliaryPos )
    {
        m_AuxText       = aAuxiliaryText;
        m_AuxPosition   = aAuxiliaryPos;
        m_hasSecondItem = true;
        m_auxItemUuid   = niluuid;
    }

    void SetParent( MARKER_BASE* aMarker ) { m_parent = aMarker; }
    MARKER_BASE* GetParent() const { return m_parent; }

    bool HasSecondItem() const { return m_hasSecondItem; }

    wxString GetMainText() const { return m_MainText; }
    wxString GetAuxText() const { return m_AuxText; }

    KIID GetMainItemID() const { return m_mainItemUuid; }
    KIID GetAuxItemID() const { return m_auxItemUuid; }

    /**
     * Function ShowReport
     * translates this object into a text string suitable for saving to disk in a report.
     * @return wxString - the simple multi-line report text.
     */
    virtual wxString ShowReport( EDA_UNITS aUnits ) const;

    int GetErrorCode() const { return m_ErrorCode; }

    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    virtual wxString GetErrorText() const = 0;

    /**
     * Function ShowCoord
     * formats a coordinate or position to text.
     */
    static wxString ShowCoord( EDA_UNITS aUnits, const wxPoint& aPos );
};


class RC_TREE_NODE
{
public:
    enum NODE_TYPE { MARKER, MAIN_ITEM, AUX_ITEM };

    RC_TREE_NODE( RC_TREE_NODE* aParent, RC_ITEM* aRcItem, NODE_TYPE aType ) :
            m_Type( aType ),
            m_RcItem( aRcItem ),
            m_Parent( aParent )
    {}

    ~RC_TREE_NODE()
    {
        for( RC_TREE_NODE* child : m_Children )
            delete child;
    }

    NODE_TYPE                  m_Type;
    RC_ITEM*                   m_RcItem;

    RC_TREE_NODE*              m_Parent;
    std::vector<RC_TREE_NODE*> m_Children;
};


class RC_TREE_MODEL : public wxDataViewModel, wxEvtHandler
{
public:
    static wxDataViewItem ToItem( RC_TREE_NODE const* aNode )
    {
        return wxDataViewItem( const_cast<void*>( static_cast<void const*>( aNode ) ) );
    }

    static RC_TREE_NODE* ToNode( wxDataViewItem aItem )
    {
        return static_cast<RC_TREE_NODE*>( aItem.GetID() );
    }

    static KIID ToUUID( wxDataViewItem aItem );


public:
    RC_TREE_MODEL( EDA_BASE_FRAME* aParentFrame, wxDataViewCtrl* aView );

    ~RC_TREE_MODEL();

    void SetProvider( RC_ITEMS_PROVIDER* aProvider );
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

    void ValueChanged( RC_TREE_NODE* aNode );

    void DeleteCurrentItem( bool aDeep );
    void DeleteAllItems();

private:
    void rebuildModel( RC_ITEMS_PROVIDER* aProvider, int aSeverities );
    void onSizeView( wxSizeEvent& aEvent );

private:
    EDA_BASE_FRAME*            m_editFrame;
    wxDataViewCtrl*            m_view;
    int                        m_severities;
    RC_ITEMS_PROVIDER*         m_rcItemsProvider;   // I own this, but not its contents

    std::vector<RC_TREE_NODE*> m_tree;              // I own this
};



#endif      // RC_ITEM_H
