/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <memory>

#include <wx/dataview.h>
#include <wx/filename.h>

#include <bitmaps/bitmap_types.h>
#include <widgets/file_checklist_ctrl.h>


void FILE_CHECKLIST_DATA::SetEntries( const std::vector<FILE_CHECKLIST_ENTRY>& aEntries )
{
    m_sections.clear();
    m_rowsBySection.clear();

    for( const FILE_CHECKLIST_ENTRY& entry : aEntries )
    {
        if( m_rowsBySection.find( entry.section ) == m_rowsBySection.end() )
            m_sections.push_back( entry.section );

        m_rowsBySection[entry.section].push_back( entry );
    }
}


const std::vector<FILE_CHECKLIST_ENTRY>& FILE_CHECKLIST_DATA::Rows( const wxString& aSection ) const
{
    static const std::vector<FILE_CHECKLIST_ENTRY> empty;

    auto it = m_rowsBySection.find( aSection );
    return it == m_rowsBySection.end() ? empty : it->second;
}


void FILE_CHECKLIST_DATA::SetChecked( const wxString& aPath, bool aChecked )
{
    for( auto& [section, rows] : m_rowsBySection )
    {
        for( FILE_CHECKLIST_ENTRY& row : rows )
        {
            if( row.path == aPath )
            {
                row.checked = aChecked;
                return;
            }
        }
    }
}


void FILE_CHECKLIST_DATA::SetSectionChecked( const wxString& aSection, bool aChecked )
{
    auto it = m_rowsBySection.find( aSection );

    if( it == m_rowsBySection.end() )
        return;

    for( FILE_CHECKLIST_ENTRY& row : it->second )
        row.checked = aChecked;
}


bool FILE_CHECKLIST_DATA::IsSectionChecked( const wxString& aSection ) const
{
    auto it = m_rowsBySection.find( aSection );

    if( it == m_rowsBySection.end() || it->second.empty() )
        return false;

    for( const FILE_CHECKLIST_ENTRY& row : it->second )
    {
        if( !row.checked )
            return false;
    }

    return true;
}


std::vector<wxString> FILE_CHECKLIST_DATA::CheckedPaths() const
{
    std::vector<wxString> paths;

    for( const wxString& section : m_sections )
    {
        for( const FILE_CHECKLIST_ENTRY& row : m_rowsBySection.at( section ) )
        {
            if( row.checked )
                paths.push_back( row.path );
        }
    }

    return paths;
}


// Addressing node for the data view. The row itself lives in FILE_CHECKLIST_DATA, so a
// node only says which section it belongs to and, for a file row, its index within that
// section. Indices stay valid because the nodes are rebuilt whenever the rows are.
struct CHECKLIST_NODE
{
    bool                                         isSection = false;
    wxString                                     section;
    size_t                                       index = 0;
    CHECKLIST_NODE*                              parent = nullptr;
    std::vector<std::unique_ptr<CHECKLIST_NODE>> children;
};


class FILE_CHECKLIST_MODEL : public wxDataViewModel
{
public:
    enum COLUMN
    {
        COL_CHECK = 0,
        COL_NAME,
        COL_STATUS,
        COL_COUNT
    };

    void SetEntries( const std::vector<FILE_CHECKLIST_ENTRY>& aEntries )
    {
        m_data.SetEntries( aEntries );
        rebuildNodes();
        Cleared();
    }

    void SetAllChecked( bool aChecked )
    {
        // Update every row before notifying listeners.
        for( const std::unique_ptr<CHECKLIST_NODE>& section : m_sectionNodes )
            m_data.SetSectionChecked( section->section, aChecked );

        for( const std::unique_ptr<CHECKLIST_NODE>& section : m_sectionNodes )
        {
            for( const std::unique_ptr<CHECKLIST_NODE>& child : section->children )
                ValueChanged( wxDataViewItem( child.get() ), COL_CHECK );

            ValueChanged( wxDataViewItem( section.get() ), COL_CHECK );
        }
    }

    std::vector<wxString> CheckedPaths() const { return m_data.CheckedPaths(); }

    wxString PathFor( const wxDataViewItem& aItem ) const
    {
        CHECKLIST_NODE* node = toNode( aItem );

        if( !node || node->isSection )
            return wxEmptyString;

        return m_data.Rows( node->section )[node->index].path;
    }

    unsigned int GetColumnCount() const override { return COL_COUNT; }

    wxString GetColumnType( unsigned int aCol ) const override
    {
        switch( aCol )
        {
        case COL_CHECK: return wxT( "bool" );
        case COL_NAME: return wxT( "wxDataViewIconText" );
        default: return wxT( "string" );
        }
    }

    void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) const override
    {
        CHECKLIST_NODE* node = toNode( aItem );

        if( !node )
            return;

        if( node->isSection )
        {
            switch( aCol )
            {
            case COL_CHECK: aVariant = m_data.IsSectionChecked( node->section ); break;
            case COL_NAME:
                aVariant << wxDataViewIconText(
                        wxString::Format( wxT( "%s (%zu)" ), node->section, m_data.Rows( node->section ).size() ) );
                break;
            default: aVariant = wxString(); break;
            }
        }
        else
        {
            const FILE_CHECKLIST_ENTRY& row = m_data.Rows( node->section )[node->index];

            switch( aCol )
            {
            case COL_CHECK: aVariant = row.checked; break;
            case COL_NAME: aVariant << wxDataViewIconText( row.label, KiBitmapBundle( row.icon ) ); break;
            default: aVariant = row.status; break;
            }
        }
    }

    bool GetAttr( const wxDataViewItem& aItem, unsigned int aCol, wxDataViewItemAttr& aAttr ) const override
    {
        CHECKLIST_NODE* node = toNode( aItem );

        if( !node || node->isSection || aCol != COL_STATUS )
            return false;

        const FILE_CHECKLIST_ENTRY& row = m_data.Rows( node->section )[node->index];

        if( !row.statusColour.IsOk() )
            return false;

        aAttr.SetColour( row.statusColour );
        return true;
    }

    bool SetValue( const wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) override
    {
        CHECKLIST_NODE* node = toNode( aItem );

        if( !node || aCol != COL_CHECK )
            return false;

        bool checked = aVariant.GetBool();

        if( node->isSection )
        {
            m_data.SetSectionChecked( node->section, checked );

            for( const std::unique_ptr<CHECKLIST_NODE>& child : node->children )
                ValueChanged( wxDataViewItem( child.get() ), COL_CHECK );
        }
        else
        {
            m_data.SetChecked( m_data.Rows( node->section )[node->index].path, checked );

            if( node->parent )
                ValueChanged( wxDataViewItem( node->parent ), COL_CHECK );
        }

        return true;
    }

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override
    {
        CHECKLIST_NODE* node = toNode( aItem );
        return ( node && node->parent ) ? wxDataViewItem( node->parent ) : wxDataViewItem();
    }

    bool IsContainer( const wxDataViewItem& aItem ) const override
    {
        CHECKLIST_NODE* node = toNode( aItem );
        return !node || node->isSection;
    }

    // Section rows carry a name and a count in the file column. Without this the base class
    // reports that a container has a value only in column 0, and the section label is never
    // drawn, leaving the header as a bare checkbox.
    bool HasContainerColumns( const wxDataViewItem& aItem ) const override { return true; }

    unsigned int GetChildren( const wxDataViewItem& aItem, wxDataViewItemArray& aChildren ) const override
    {
        if( !aItem.IsOk() )
        {
            for( const std::unique_ptr<CHECKLIST_NODE>& section : m_sectionNodes )
                aChildren.Add( wxDataViewItem( section.get() ) );

            return aChildren.GetCount();
        }

        CHECKLIST_NODE* node = toNode( aItem );

        if( node && node->isSection )
        {
            for( const std::unique_ptr<CHECKLIST_NODE>& child : node->children )
                aChildren.Add( wxDataViewItem( child.get() ) );
        }

        return aChildren.GetCount();
    }

private:
    CHECKLIST_NODE* toNode( const wxDataViewItem& aItem ) const
    {
        return static_cast<CHECKLIST_NODE*>( aItem.GetID() );
    }

    void rebuildNodes()
    {
        m_sectionNodes.clear();

        for( const wxString& section : m_data.Sections() )
        {
            auto sectionNode = std::make_unique<CHECKLIST_NODE>();
            sectionNode->isSection = true;
            sectionNode->section = section;

            for( size_t i = 0; i < m_data.Rows( section ).size(); ++i )
            {
                auto fileNode = std::make_unique<CHECKLIST_NODE>();
                fileNode->section = section;
                fileNode->index = i;
                fileNode->parent = sectionNode.get();
                sectionNode->children.push_back( std::move( fileNode ) );
            }

            m_sectionNodes.push_back( std::move( sectionNode ) );
        }
    }

    FILE_CHECKLIST_DATA                          m_data;
    std::vector<std::unique_ptr<CHECKLIST_NODE>> m_sectionNodes;
};


FILE_CHECKLIST_CTRL::FILE_CHECKLIST_CTRL( wxWindow* aParent, wxWindowID aId ) :
        WX_DATAVIEWCTRL( aParent, aId, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES ),
        m_model( new FILE_CHECKLIST_MODEL() )
{
    AssociateModel( m_model.get() );

    AppendToggleColumn( wxEmptyString, FILE_CHECKLIST_MODEL::COL_CHECK, wxDATAVIEW_CELL_ACTIVATABLE,
                        wxCOL_WIDTH_AUTOSIZE );
    AppendIconTextColumn( _( "File" ), FILE_CHECKLIST_MODEL::COL_NAME, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );
    AppendTextColumn( _( "Status" ), FILE_CHECKLIST_MODEL::COL_STATUS, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );

    Bind( wxEVT_IDLE, &FILE_CHECKLIST_CTRL::onIdle, this );
}


void FILE_CHECKLIST_CTRL::DoSetToolTipText( const wxString& aTip )
{
    m_ownerTooltip = aTip;
    m_shownTooltip = aTip;
    wxDataViewCtrl::DoSetToolTipText( aTip );
}


// A row shows the file name alone, so two files with the same name in different directories
// read the same. Hovering one shows the path that tells them apart.
void FILE_CHECKLIST_CTRL::onIdle( wxIdleEvent& aEvent )
{
    aEvent.Skip();

    wxString tip = m_ownerTooltip;
    wxPoint  screenPos = wxGetMousePosition();

    if( IsShownOnScreen() && GetScreenRect().Contains( screenPos ) )
    {
        wxDataViewItem    item;
        wxDataViewColumn* column = nullptr;

        HitTest( ScreenToClient( screenPos ), item, column );

        wxString path = GetItemPath( item );

        if( !path.IsEmpty() && path != wxFileName( path ).GetFullName() )
            tip = path;
    }

    if( tip == m_shownTooltip )
        return;

    m_shownTooltip = tip;
    wxDataViewCtrl::DoSetToolTipText( tip );
}


// Out of line because wxObjectDataPtr needs the complete model type to release it, and the
// header only forward declares it.
FILE_CHECKLIST_CTRL::~FILE_CHECKLIST_CTRL()
{
}


void FILE_CHECKLIST_CTRL::SetEntries( const std::vector<FILE_CHECKLIST_ENTRY>& aEntries )
{
    m_model->SetEntries( aEntries );
    ExpandAll();
}


void FILE_CHECKLIST_CTRL::SetAllChecked( bool aChecked )
{
    m_model->SetAllChecked( aChecked );
}


std::vector<wxString> FILE_CHECKLIST_CTRL::GetCheckedPaths() const
{
    return m_model->CheckedPaths();
}


wxString FILE_CHECKLIST_CTRL::GetItemPath( const wxDataViewItem& aItem ) const
{
    return m_model->PathFor( aItem );
}
