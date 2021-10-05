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
#include <kiid.h>
#include <reporter.h>

class MARKER_BASE;
class EDA_BASE_FRAME;
class RC_ITEM;
class EDA_ITEM;
class EDA_DRAW_FRAME;


/**
 * Provide an abstract interface of a RC_ITEM* list manager.
 *
 * The details of the actual list architecture are hidden from the caller.  Any class that
 * implements this interface can then be used by a RC_TREE_MODEL class without it knowing
 * the actual architecture of the list.
 */
class RC_ITEMS_PROVIDER
{
public:
    virtual void SetSeverities( int aSeverities ) = 0;

    virtual int GetCount( int aSeverity = -1 ) const = 0;

    /**
     * Retrieve a RC_ITEM by index.
     */
    virtual std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const = 0;

    /**
     * Remove (and optionally deletes) the indexed item from the list.
     * @param aDeep If true, the source item should be deleted as well as its entry in the list.
     */
    virtual void DeleteItem( int aIndex, bool aDeep ) = 0;

    virtual void DeleteAllItems( bool aIncludeExclusions, bool aDeep ) = 0;

    virtual ~RC_ITEMS_PROVIDER() { }
};


/**
 * A holder for a rule check item, DRC in Pcbnew or ERC in Eeschema.
 *
 * RC_ITEMs can have zero, one, or two related EDA_ITEMs.
 */
class RC_ITEM
{
public:
    typedef std::vector<KIID> KIIDS;

    RC_ITEM() :
        m_errorCode( 0 ),
        m_parent( nullptr )
    {
    }

    RC_ITEM( std::shared_ptr<RC_ITEM> aItem )
    {
        m_errorCode    = aItem->m_errorCode;
        m_errorMessage = aItem->m_errorMessage;
        m_errorTitle   = aItem->m_errorTitle;
        m_settingsKey  = aItem->m_settingsKey;
        m_parent       = aItem->m_parent;
        m_ids = aItem->m_ids;
    }

    virtual ~RC_ITEM() { }

    void SetErrorMessage( const wxString& aMessage ) { m_errorMessage = aMessage; }

    void SetItems( const KIIDS& aIds )
    {
        m_ids = aIds;
    }

    void AddItem( EDA_ITEM* aItem );

    void SetItems( const EDA_ITEM* aItem, const EDA_ITEM* bItem = nullptr,
                   const EDA_ITEM* cItem = nullptr, const EDA_ITEM* dItem = nullptr );

    void SetItems( const KIID& aItem, const KIID& bItem = niluuid, const KIID& cItem = niluuid,
                   const KIID& dItem = niluuid )
    {
        m_ids.clear();

        m_ids.push_back( aItem );
        m_ids.push_back( bItem );
        m_ids.push_back( cItem );
        m_ids.push_back( dItem );
    }

    KIID GetMainItemID() const { return m_ids.size() > 0 ? m_ids[0] : niluuid; }
    KIID GetAuxItemID() const { return m_ids.size() > 1 ? m_ids[1] : niluuid;; }
    KIID GetAuxItem2ID() const { return m_ids.size() > 2 ? m_ids[2] : niluuid;; }
    KIID GetAuxItem3ID() const { return m_ids.size() > 3 ? m_ids[3] : niluuid;; }

    void SetParent( MARKER_BASE* aMarker ) { m_parent = aMarker; }
    MARKER_BASE* GetParent() const { return m_parent; }


    /**
     * Translate this object into a text string suitable for saving to disk in a report.
     *
     * @return wxString - the simple multi-line report text.
     */
    virtual wxString ShowReport( EDA_UNITS aUnits, SEVERITY aSeverity,
                                 const std::map<KIID, EDA_ITEM*>& aItemMap ) const;

    int GetErrorCode() const { return m_errorCode; }
    void SetErrorCode( int aCode ) { m_errorCode = aCode; }

    /**
     * Return the error message of a RC_ITEM.
     */
    virtual wxString GetErrorMessage() const;

    wxString GetErrorText() const
    {
        return wxGetTranslation( m_errorTitle );
    }

    wxString GetSettingsKey() const
    {
        return m_settingsKey;
    }

    virtual wxString GetViolatingRuleDesc() const
    {
        return wxEmptyString;
    }

    /**
     * Format a coordinate or position to text.
     */
    static wxString ShowCoord( EDA_UNITS aUnits, const wxPoint& aPos );

protected:
    int           m_errorCode;         ///< The error code's numeric value
    wxString      m_errorMessage;      ///< A message describing the details of this specific error
    wxString      m_errorTitle;        ///< The string describing the type of error
    wxString      m_settingsKey;       ///< The key used to describe this type of error in settings
    MARKER_BASE*  m_parent;            ///< The marker this item belongs to, if any

    KIIDS m_ids;

};


class RC_TREE_NODE
{
public:
    enum NODE_TYPE { MARKER, MAIN_ITEM, AUX_ITEM, AUX_ITEM2, AUX_ITEM3 };

    RC_TREE_NODE( RC_TREE_NODE* aParent, std::shared_ptr<RC_ITEM> aRcItem, NODE_TYPE aType ) :
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
    std::shared_ptr<RC_ITEM>   m_RcItem;

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

    RC_TREE_MODEL( EDA_DRAW_FRAME* aParentFrame, wxDataViewCtrl* aView );

    ~RC_TREE_MODEL();

    void SetProvider( RC_ITEMS_PROVIDER* aProvider );
    void SetSeverities( int aSeverities );

    int GetDRCItemCount() const { return m_tree.size(); }

    void ExpandAll();

    void PrevMarker();
    void NextMarker();
    void SelectMarker( const MARKER_BASE* aMarker );
    void CenterMarker( const MARKER_BASE* aMarker );

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
     * Called by the wxDataView to fetch an item's formatting.  Return true if the
     * item has non-default attributes.
     */
    bool GetAttr( wxDataViewItem const&   aItem,
                  unsigned int            aCol,
                  wxDataViewItemAttr&     aAttr ) const override;

    void ValueChanged( const RC_TREE_NODE* aNode );

    void DeleteCurrentItem( bool aDeep );

    /**
     * Deletes the current item or all items.  If all, \a aIncludeExclusions determines
     * whether or not exclusions are also deleted.
     */
    void DeleteItems( bool aCurrentOnly, bool aIncludeExclusions, bool aDeep );

private:
    void rebuildModel( RC_ITEMS_PROVIDER* aProvider, int aSeverities );
    void onSizeView( wxSizeEvent& aEvent );

    EDA_DRAW_FRAME*            m_editFrame;
    wxDataViewCtrl*            m_view;
    int                        m_severities;
    RC_ITEMS_PROVIDER*         m_rcItemsProvider;   // I own this, but not its contents

    std::vector<RC_TREE_NODE*> m_tree;              // I own this
};

#endif      // RC_ITEM_H
