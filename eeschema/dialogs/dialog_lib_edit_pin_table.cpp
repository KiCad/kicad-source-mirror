/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_lib_edit_pin_table.h"
#include "lib_pin.h"
#include "pin_number.h"

#include <boost/algorithm/string/join.hpp>
#include <queue>
#include <list>
#include <map>

/* Avoid wxWidgets bug #16906 -- http://trac.wxwidgets.org/ticket/16906
 *
 * If multiple elements live in the root of a wxDataViewCtrl, using
 * ItemsAdded() can run into an assertion failure. To avoid this, we avoid
 * notifying the widget of changes, but rather reinitialize it.
 *
 * When a fix for this exists in wxWidgets, this is the place to turn it
 * off.
 */
#define REASSOCIATE_HACK

class DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel :
    public wxDataViewModel
{
public:
    DataViewModel( LIB_PART& aPart );

    // wxDataViewModel
    virtual unsigned int    GetColumnCount() const override;
    virtual wxString        GetColumnType( unsigned int col ) const override;
    virtual void            GetValue( wxVariant&, const wxDataViewItem&, unsigned int ) const override;
    virtual bool            SetValue( const wxVariant&, const wxDataViewItem&, unsigned int ) override;
    virtual wxDataViewItem  GetParent( const wxDataViewItem& ) const override;
    virtual bool            IsContainer( const wxDataViewItem& ) const override;
    virtual bool            HasContainerColumns( const wxDataViewItem& ) const override;
    virtual unsigned int    GetChildren( const wxDataViewItem&, wxDataViewItemArray& ) const override;

    virtual int Compare( const wxDataViewItem& lhs,
            const wxDataViewItem& rhs,
            unsigned int col,
            bool ascending ) const override;

    void    SetGroupingColumn( int aCol );
    void    CalculateGrouping();
    void    Refresh();

    PinNumbers GetAllPinNumbers();

#ifdef REASSOCIATE_HACK
    void SetWidget( wxDataViewCtrl* aWidget ) { m_Widget = aWidget; }
#endif

    enum
    {
        NONE            = -1,
        PIN_NUMBER      = 0,
        PIN_NAME        = 1,
        PIN_TYPE        = 2,
        PIN_POSITION    = 3
    };

private:
    LIB_PART& m_Part;
    LIB_PINS m_Backing;
    int m_GroupingColumn;
    int m_UnitCount;

    class Item;
    class Group;
    class Pin;

    mutable std::list<Pin> m_Pins;
    mutable std::map<wxString, Group> m_Groups;

    // like GetValue, but always returns a string
    wxString GetString( const wxDataViewItem&, unsigned int ) const;

#ifdef REASSOCIATE_HACK
    wxDataViewCtrl* m_Widget;
#endif
};

class DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Item
{
public:
    virtual void            GetValue( wxVariant& aValue, unsigned int aCol ) const = 0;
    virtual wxString        GetString( unsigned int aCol ) const = 0;
    virtual wxDataViewItem  GetParent() const = 0;
    virtual bool            IsContainer() const = 0;
    virtual unsigned int    GetChildren( wxDataViewItemArray& ) const = 0;
};

class DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Group :
    public Item
{
public:
    Group( unsigned int aGroupingColumn ) : m_GroupingColumn( aGroupingColumn ) {}

    virtual void            GetValue( wxVariant& aValue, unsigned int aCol ) const override;
    virtual wxString        GetString( unsigned int aCol ) const override;
    virtual wxDataViewItem  GetParent() const override { return wxDataViewItem(); }
    virtual bool            IsContainer() const override { return true; }
    virtual unsigned int    GetChildren( wxDataViewItemArray& aItems ) const override
    {
        /// @todo C++11
        for( std::list<Pin*>::const_iterator i = m_Members.begin(); i != m_Members.end(); ++i )
            aItems.push_back( wxDataViewItem( *i ) );

        return aItems.size();
    }

    unsigned int            GetCount() const { return m_Members.size(); }
    void                    Add( Pin* aPin );

private:
    std::list<Pin*> m_Members;
    unsigned int m_GroupingColumn;
};

class DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Pin :
    public Item
{
public:
    Pin( DataViewModel& aModel,
            LIB_PIN* aBacking ) : m_Model( aModel ), m_Backing( aBacking ), m_Group( 0 ) {}

    virtual void GetValue( wxVariant& aValue, unsigned int aCol ) const override;
    virtual wxString        GetString( unsigned int aCol ) const override;
    virtual wxDataViewItem GetParent() const override { return wxDataViewItem( m_Group ); }
    virtual bool IsContainer() const override { return false; }
    virtual unsigned int GetChildren( wxDataViewItemArray& ) const override { return 0; }

    void SetGroup( Group* aGroup ) { m_Group = aGroup; }

private:
    DataViewModel& m_Model;
    LIB_PIN* m_Backing;
    Group* m_Group;
};

DIALOG_LIB_EDIT_PIN_TABLE::DIALOG_LIB_EDIT_PIN_TABLE( wxWindow* parent,
        LIB_PART& aPart ) :
    DIALOG_LIB_EDIT_PIN_TABLE_BASE( parent ),
    m_Model( new DataViewModel( aPart ) )
{
#ifdef REASSOCIATE_HACK
    m_Model->SetWidget( m_Pins );
#endif
    m_Pins->AssociateModel( m_Model.get() );

    /// @todo wxFormBuilder bug #61 -- move to base once supported
    wxDataViewTextRenderer* rend0 = new wxDataViewTextRenderer( wxT( "string" ), wxDATAVIEW_CELL_INERT );
    wxDataViewColumn* col0 = new wxDataViewColumn( _( "Number" ),
            rend0,
            DataViewModel::PIN_NUMBER,
            100,
            wxAlignment( wxALIGN_LEFT | wxALIGN_TOP ),
            wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE );
    wxDataViewTextRenderer* rend1 = new wxDataViewTextRenderer( wxT( "string" ), wxDATAVIEW_CELL_INERT );
    wxDataViewColumn* col1 = new wxDataViewColumn( _( "Name" ),
            rend1,
            DataViewModel::PIN_NAME,
            100,
            wxAlignment( wxALIGN_LEFT | wxALIGN_TOP ),
            wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE );
    wxDataViewIconTextRenderer* rend2 = new wxDataViewIconTextRenderer( wxT( "wxDataViewIconText" ), wxDATAVIEW_CELL_INERT );
    wxDataViewColumn* col2 = new wxDataViewColumn( _( "Type" ),
            rend2,
            DataViewModel::PIN_TYPE,
            100,
            wxAlignment( wxALIGN_LEFT | wxALIGN_TOP ),
            wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE );
    wxDataViewTextRenderer* rend3 = new wxDataViewTextRenderer( wxT( "string" ), wxDATAVIEW_CELL_INERT );
    wxDataViewColumn* col3 = new wxDataViewColumn( _( "Position" ),
            rend3,
            DataViewModel::PIN_POSITION,
            100,
            wxAlignment( wxALIGN_LEFT | wxALIGN_TOP ),
            wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE );
    m_Pins->AppendColumn( col0 );
    m_Pins->SetExpanderColumn( col0 );
    m_Pins->AppendColumn( col1 );
    m_Pins->AppendColumn( col2 );
    m_Pins->AppendColumn( col3 );

    UpdateSummary();

    GetSizer()->SetSizeHints(this);
    Centre();
}


DIALOG_LIB_EDIT_PIN_TABLE::~DIALOG_LIB_EDIT_PIN_TABLE()
{
}


void DIALOG_LIB_EDIT_PIN_TABLE::UpdateSummary()
{
    PinNumbers pins = m_Model->GetAllPinNumbers();

    m_Summary->SetValue( pins.GetSummary() );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnColumnHeaderRightClicked( wxDataViewEvent& event )
{
    m_Model->SetGroupingColumn( event.GetDataViewColumn()->GetModelColumn() );
    event.Skip();
}


DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::DataViewModel( LIB_PART& aPart ) :
    m_Part( aPart ),
    m_GroupingColumn( 1 ),
    m_UnitCount( m_Part.GetUnitCount() )
{
#ifdef REASSOCIATE_HACK
    m_Widget = NULL;
#endif
    aPart.GetPins( m_Backing );
    /// @todo C++11
    for( LIB_PINS::const_iterator i = m_Backing.begin(); i != m_Backing.end(); ++i )
        m_Pins.push_back( Pin( *this, *i ) );

    CalculateGrouping();
}


unsigned int DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetColumnCount() const
{
    return 4;
}


wxString DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetColumnType( unsigned int aCol ) const
{
    switch( aCol )
    {
    case PIN_NUMBER:
        return wxT( "string" );

    case PIN_NAME:
        return wxT( "string" );

    case PIN_TYPE:
        return wxT( "wxDataViewIconText" );

    case PIN_POSITION:
        return wxT( "string" );
    }

    assert( ! "Unhandled column" );
    return wxT( "" );
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetValue( wxVariant& aVal,
        const wxDataViewItem& aItem,
        unsigned int aCol ) const
{
    assert( aItem.IsOk() );

    reinterpret_cast<Item const*>( aItem.GetID() )->GetValue( aVal, aCol );
}


bool DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::SetValue( const wxVariant&,
        const wxDataViewItem&,
        unsigned int )
{
    return false;
}


wxDataViewItem DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetParent( const wxDataViewItem& aItem )
const
{
    assert( aItem.IsOk() );

    return reinterpret_cast<Item const*>( aItem.GetID() )->GetParent();
}


bool DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::IsContainer( const wxDataViewItem& aItem ) const
{
    if( aItem.IsOk() )
        return reinterpret_cast<Item const*>( aItem.GetID() )->IsContainer();
    else
        return true;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::HasContainerColumns( const wxDataViewItem& ) const
{
    return true;
}


unsigned int DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetChildren( const wxDataViewItem& aItem,
        wxDataViewItemArray& aItems ) const
{
    if( !aItem.IsOk() )
    {
        for( std::map<wxString, Group>::iterator i = m_Groups.begin(); i != m_Groups.end(); ++i )
            if( i->second.GetCount() > 1 )
                aItems.push_back( wxDataViewItem( &i->second ) );

        for( std::list<Pin>::iterator i = m_Pins.begin(); i != m_Pins.end(); ++i )
            if( !i->GetParent().IsOk() )
                aItems.push_back( wxDataViewItem( &*i ) );

        return aItems.size();
    }
    else
        return reinterpret_cast<Item const*>( aItem.GetID() )->GetChildren( aItems );
}


int DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Compare( const wxDataViewItem& aItem1,
        const wxDataViewItem& aItem2,
        unsigned int aCol,
        bool aAscending ) const
{
    wxString str1 = GetString( aItem1, aCol );
    wxString str2 = GetString( aItem2, aCol );
    int res = PinNumbers::Compare( str1, str2 );

    if( res == 0 )
        res = ( aItem1.GetID() < aItem2.GetID() ) ? -1 : 1;

    return res * ( aAscending ? 1 : -1 );
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::SetGroupingColumn( int aCol )
{
    if( m_GroupingColumn == aCol )
        m_GroupingColumn = NONE;
    else
        m_GroupingColumn = aCol;

    Cleared();
    CalculateGrouping();
    Refresh();
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::CalculateGrouping()
{
    m_Groups.clear();

    if( m_GroupingColumn != -1 )
    {
        wxVariant value;

        for( std::list<Pin>::iterator i = m_Pins.begin(); i != m_Pins.end(); ++i )
        {
            wxString str = i->GetString( m_GroupingColumn );
            std::map<wxString, Group>::iterator j = m_Groups.find( str );

            if( j == m_Groups.end() )
                j = m_Groups.insert( std::make_pair( str, m_GroupingColumn ) ).first;

            j->second.Add( &*i );
        }
    }
    else
    {
        for( std::list<Pin>::iterator i = m_Pins.begin(); i != m_Pins.end(); ++i )
            i->SetGroup( 0 );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Refresh()
{
#ifdef REASSOCIATE_HACK
    m_Widget->AssociateModel( this );
#else
    std::queue<wxDataViewItem> todo;
    todo.push( wxDataViewItem() );

    while( !todo.empty() )
    {
        wxDataViewItem current = todo.front();
        wxDataViewItemArray items;

        GetChildren( current, items );
        ItemsAdded( current, items );

        for( wxDataViewItemArray::const_iterator i = items.begin(); i != items.end(); ++i )
        {
            if( IsContainer( *i ) )
                todo.push( *i );
        }

        todo.pop();
    }

#endif
}


PinNumbers DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetAllPinNumbers()
{
    PinNumbers ret;

    for( std::list<Pin>::const_iterator i = m_Pins.begin(); i != m_Pins.end(); ++i )
        ret.insert( i->GetString( PIN_NUMBER ) );

    return ret;
}


wxString DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::GetString( const wxDataViewItem& aItem, unsigned int aCol ) const
{
    assert( aItem.IsOk() );

    return reinterpret_cast<Item const*>( aItem.GetID() )->GetString( aCol );
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Group::GetValue( wxVariant& aValue,
        unsigned int aCol ) const
{
    if( aCol == m_GroupingColumn )
        // shortcut
        m_Members.front()->GetValue( aValue, aCol );
    else if( aCol != PIN_TYPE )
        aValue = GetString( aCol );
    else
    {
        PinNumbers values;

        for( std::list<Pin*>::const_iterator i = m_Members.begin(); i != m_Members.end(); ++i )
            values.insert( (*i)->GetString( aCol ) );

        if( values.size() > 1 )
            aValue << wxDataViewIconText( boost::algorithm::join( values, "," ), wxNullIcon );
        else
            m_Members.front()->GetValue( aValue, aCol );
    }
}


wxString DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Group::GetString( unsigned int aCol ) const
{
    if( aCol == m_GroupingColumn )
        return m_Members.front()->GetString( aCol );

    PinNumbers values;

    for( std::list<Pin*>::const_iterator i = m_Members.begin(); i != m_Members.end(); ++i )
        values.insert( (*i)->GetString( aCol ) );

    if( values.size() > 1 )
        return boost::algorithm::join( values, "," );
    else
        return m_Members.front()->GetString( aCol );
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Group::Add( Pin* aPin )
{
    switch( GetCount() )
    {
    case 0:
        aPin->SetGroup( 0 );
        break;

    case 1:
        m_Members.front()->SetGroup( this );
        // fall through

    default:
        aPin->SetGroup( this );
    }

    m_Members.push_back( aPin );
}


void DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Pin::GetValue( wxVariant& aValue,
        unsigned int aCol ) const
{
    switch( aCol )
    {
    case PIN_NUMBER:
    case PIN_NAME:
    case PIN_POSITION:
        aValue = GetString( aCol );
        break;

    case PIN_TYPE:
        {
            wxIcon icon;
            icon.CopyFromBitmap( KiBitmap ( GetBitmap( m_Backing->GetType() ) ) );
            aValue << wxDataViewIconText( m_Backing->GetElectricalTypeName(), icon );
        }
        break;
    }
}


wxString DIALOG_LIB_EDIT_PIN_TABLE::DataViewModel::Pin::GetString( unsigned int aCol ) const
{
    switch( aCol )
    {
    case PIN_NUMBER:
        return m_Backing->GetNumberString();

    case PIN_NAME:
        if( m_Model.m_UnitCount > 1 )
        {
            wxString name;
            int unit = m_Backing->GetPartNumber();

            if( unit )
                name << unit;
            else
                name << "com";

            name << ':';
            name << m_Backing->GetName();
            return name;
        }
        else
        {
            return m_Backing->GetName();
        }

    case PIN_TYPE:
        return m_Backing->GetElectricalTypeName();

    case PIN_POSITION:
        {
            wxPoint position = m_Backing->GetPosition();
            wxString value;
            value << "(" << position.x << "," << position.y << ")";
            return value;
        }
    }

    return wxEmptyString;
}
