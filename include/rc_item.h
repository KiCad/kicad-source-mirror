/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef RC_ITEM_H
#define RC_ITEM_H

#include <wx/dataview.h>
#include <units_provider.h>
#include <kiid.h>
#include <reporter.h>
#include <math/vector2d.h>

class MARKER_BASE;
class EDA_BASE_FRAME;
class RC_ITEM;
class EDA_ITEM;
class EDA_DRAW_FRAME;

namespace RC_JSON
{
struct VIOLATION;
}

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

    virtual int GetSeverities() const = 0;

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

    RC_ITEM( const std::shared_ptr<RC_ITEM>& aItem )
    {
        m_errorCode    = aItem->m_errorCode;
        m_errorMessage = aItem->m_errorMessage;
        m_errorTitle   = aItem->m_errorTitle;
        m_settingsKey  = aItem->m_settingsKey;
        m_parent       = aItem->m_parent;
        m_ids          = aItem->m_ids;
    }

    virtual ~RC_ITEM() { }

    void SetErrorMessage( const wxString& aMessage ) { m_errorMessage = aMessage; }

    void SetErrorDetail( const wxString& aMsg ) { SetErrorMessage( GetErrorText( true ) + wxS( " " ) + aMsg ); }

    void SetItems( const KIIDS& aIds ) { m_ids = aIds; }

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

    virtual KIID GetMainItemID() const { return m_ids.size() > 0 ? m_ids[0] : niluuid; }
    virtual KIID GetAuxItemID() const { return m_ids.size() > 1 ? m_ids[1] : niluuid; }
    virtual KIID GetAuxItem2ID() const { return m_ids.size() > 2 ? m_ids[2] : niluuid; }
    virtual KIID GetAuxItem3ID() const { return m_ids.size() > 3 ? m_ids[3] : niluuid; }

    std::vector<KIID> GetIDs() const { return m_ids; }

    void SetParent( MARKER_BASE* aMarker ) { m_parent = aMarker; }
    MARKER_BASE* GetParent() const { return m_parent; }


    /**
     * Translate this object into a text string suitable for saving to disk in a report.
     *
     * @return wxString - the simple multi-line report text.
     */
    virtual wxString ShowReport( UNITS_PROVIDER* aUnitsProvider, SEVERITY aSeverity,
                                 const std::map<KIID, EDA_ITEM*>& aItemMap ) const;

    /**
     * Translate this object into an RC_JSON::VIOLATION object
     *
     * @param aViolation is the violation to be populated by info from this item
     * @param aUnitsProvider is the units provider that will be used to output coordinates
     * @param aSeverity is the severity of this item
     * @param aItemMap is a map allowing the lookup of items from KIIDs
     *
     * @return None
     */
    virtual void GetJsonViolation( RC_JSON::VIOLATION& aViolation, UNITS_PROVIDER* aUnitsProvider,
                                   SEVERITY aSeverity,
                                   const std::map<KIID, EDA_ITEM*>& aItemMap ) const;

    int GetErrorCode() const { return m_errorCode; }
    void SetErrorCode( int aCode ) { m_errorCode = aCode; }

    /**
     * @return the error message describing the specific details of a RC_ITEM.  For instance,
     * "Clearance violation (netclass '100ohm' clearance 0.4000mm; actual 0.3200mm)"
     */
    virtual wxString GetErrorMessage( bool aTranslate ) const;

    /**
     * @return the error text for the class of error of this RC_ITEM represents.  For instance,
     * "Clearance violation".
     */
    wxString GetErrorText( bool aTranslate ) const
    {
        if( aTranslate )
            return wxGetTranslation( m_errorTitle );
        else
            return m_errorTitle;
    }

    wxString GetSettingsKey() const
    {
        return m_settingsKey;
    }

    virtual wxString GetViolatingRuleDesc( bool aTranslate ) const
    {
        return wxEmptyString;
    }

protected:
    static wxString getSeverityString( SEVERITY aSeverity );

    int           m_errorCode;         ///< The error code's numeric value
    wxString      m_errorMessage;      ///< A message describing the details of this specific error
    wxString      m_errorTitle;        ///< The string describing the type of error
    wxString      m_settingsKey;       ///< The key used to describe this type of error in settings
    MARKER_BASE*  m_parent;            ///< The marker this item belongs to, if any

    KIIDS         m_ids;

};


class RC_TREE_NODE
{
public:
    enum NODE_TYPE
    {
        MARKER,
        MAIN_ITEM,
        AUX_ITEM,
        AUX_ITEM2,
        AUX_ITEM3,
        COMMENT
    };

    RC_TREE_NODE( RC_TREE_NODE* aParent, const std::shared_ptr<RC_ITEM>& aRcItem,
                  NODE_TYPE aType ) :
            m_Type( aType ),
            m_RcItem( aRcItem ),
            m_Parent( aParent )
    {}

    ~RC_TREE_NODE()
    {
        for( RC_TREE_NODE* child : m_Children )
            delete child;
    }

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    RC_TREE_NODE( const RC_TREE_NODE& ) = delete;
    RC_TREE_NODE& operator=( const RC_TREE_NODE& ) = delete;

    NODE_TYPE                  m_Type;
    std::shared_ptr<RC_ITEM>   m_RcItem;

    RC_TREE_NODE*              m_Parent;
    std::vector<RC_TREE_NODE*> m_Children;
};


class RC_TREE_MODEL : public wxDataViewModel, public wxEvtHandler
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

    const wxDataViewCtrl* GetView() const { return m_view; }

    static KIID ToUUID( wxDataViewItem aItem );

    RC_TREE_MODEL( EDA_DRAW_FRAME* aParentFrame, wxDataViewCtrl* aView );

    ~RC_TREE_MODEL();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    RC_TREE_MODEL( const RC_TREE_MODEL& ) = delete;
    RC_TREE_MODEL& operator=( const RC_TREE_MODEL& ) = delete;

    void Update( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities );

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

    bool HasValue( const wxDataViewItem& item, unsigned col ) const override
    {
        if( m_tree.empty() )
            return false;
        else
            return wxDataViewModel::HasValue( item, col );
    }

    /**
     * Called by the wxDataView to fetch an item's value.
     */
    void GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                   unsigned int aCol ) const override;

    /**
     * Called by the wxDataView to edit an item's content.
     */
    bool SetValue( wxVariant const& aVariant, wxDataViewItem const& aItem,
                   unsigned int aCol ) override
    {
        // Editing not supported
        return false;
    }

    /**
     * Called by the wxDataView to fetch an item's formatting.  Return true if the
     * item has non-default attributes.
     */
    bool GetAttr( wxDataViewItem const& aItem, unsigned int aCol,
                  wxDataViewItemAttr& aAttr ) const override;

    void ValueChanged( RC_TREE_NODE* aNode );

    void DeleteCurrentItem( bool aDeep );

    /**
     * Delete the current item or all items.
     *
     * If all, \a aIncludeExclusions determines whether or not exclusions are also deleted.
     */
    void DeleteItems( bool aCurrentOnly, bool aIncludeExclusions, bool aDeep );

protected:
    void     rebuildModel( std::shared_ptr<RC_ITEMS_PROVIDER> aProvider, int aSeverities );

    EDA_DRAW_FRAME*                    m_editFrame;
    wxDataViewCtrl*                    m_view;
    int                                m_severities;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_rcItemsProvider;

    std::vector<RC_TREE_NODE*>         m_tree;              // I own this
};

#endif      // RC_ITEM_H
