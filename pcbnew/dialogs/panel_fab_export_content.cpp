/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "panel_fab_export_content.h"
#include <pcb_io/odbpp/odb_export_job.h>
#include <pcb_io/odbpp/odb_util.h>

#include <algorithm>
#include <board.h>
#include <pcb_io/ipc2581/ipc2581_function_mode.h>
#include <string_utils.h>

#include <wx/choicdlg.h>

namespace
{
struct SECTION_ROW
{
    FAB::SECTION m_section;
    wxString     m_label;
    wxString     m_odb;
    wxString     m_ipc;
};


std::vector<SECTION_ROW> sectionRows()
{
    using SECTION = FAB::SECTION;

    return {
        { SECTION::BOM_AVL, _( "BOM" ), wxS( "components PRP, boms/" ), wxS( "Bom, Avl" ) },
        { SECTION::PACKAGES, _( "packages" ), wxS( "eda/data PKG" ), wxS( "Package" ) },
        { SECTION::COMPONENTS, _( "components" ), wxS( "comp_+_top, comp_+_bot" ), wxS( "Component" ) },
        { SECTION::PADSTACKS, _( "padstacks" ), wxS( "features .geometry" ), wxS( "PadStackDef" ) },
        { SECTION::STACKUP, _( "stackup" ), wxS( "matrix/stackup.xml" ), wxS( "Stackup, Spec" ) },
        { SECTION::PROFILE, _( "board profile" ), wxS( "steps/pcb/profile" ), wxS( "Profile" ) },
        { SECTION::SOLDERMASK, _( "solder mask" ), wxS( "SOLDER_MASK" ), wxS( "SOLDERMASK" ) },
        { SECTION::SOLDERPASTE, _( "solder paste" ), wxS( "SOLDER_PASTE" ), wxS( "SOLDERPASTE" ) },
        { SECTION::SILKSCREEN, _( "silkscreen" ), wxS( "SILK_SCREEN" ), wxS( "SILKSCREEN" ) },
        { SECTION::DRILL_ROUT, _( "drill and router" ), wxS( "DRILL, ROUT" ), wxS( "DRILL, ROUT" ) },
        { SECTION::DOCUMENTATION, _( "documentation" ), wxS( "DOCUMENT" ), wxS( "DOCUMENT" ) },
        { SECTION::OUTER_COPPER, _( "outer copper" ), wxS( "SIGNAL" ), wxS( "CONDUCTOR, SIGNAL" ) },
        { SECTION::INNER_COPPER, _( "inner copper" ), wxS( "SIGNAL, POWER_GROUND, MIXED" ),
          wxS( "SIGNAL, PLANE, MIXED" ) },
        { SECTION::DIELECTRIC, _( "dielectric" ), wxS( "DIELECTRIC rows" ), wxS( "DIELCORE, DIELPREG" ) },
        { SECTION::MISC_FAB, _( "misc fab layers" ), wxS( "DOCUMENT (MISC)" ), wxS( "Layer (other functions)" ) },
        { SECTION::LOGICAL_NET, _( "logical netlist" ), wxS( "eda/data NET" ), wxS( "LogicalNet" ) },
        { SECTION::PHYSICAL_NET, _( "physical netlist" ), wxS( "netlists/cadnet" ), wxS( "PhyNetGroup" ) },
    };
}


class DIALOG_FAB_CUSTOMIZE : public DIALOG_FAB_CUSTOMIZE_BASE
{
public:
    DIALOG_FAB_CUSTOMIZE( wxWindow* aParent, FAB_CONTENT_FORMAT aFormat, FAB::MODE aMode,
                          const FAB::SECTION_SET& aCurrent, bool aBoardMetadata ) :
            DIALOG_FAB_CUSTOMIZE_BASE( aParent ),
            m_original( aCurrent )
    {
        bool odb = aFormat == FAB_CONTENT_FORMAT::ODBPP;
        m_sections->AppendToggleColumn( wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE, 36 );
        m_sections->AppendTextColumn( _( "Section" ), wxDATAVIEW_CELL_INERT, 170 );
        m_sections->AppendTextColumn( odb ? _( "ODB++" ) : _( "IPC-2581" ), wxDATAVIEW_CELL_INERT, 260 );
        FAB::SECTION_SET optional = FAB::OptionalSections( aMode );

        for( const SECTION_ROW& row : sectionRows() )
        {
            m_rows.push_back( row.m_section );
            // A preset allows changes only to its optional sections
            m_editable.push_back( ( odb && aMode == FAB::MODE::USERDEF ) || optional.Contains( row.m_section ) );
            wxVector<wxVariant> values;
            values.push_back( wxVariant( aCurrent.Contains( row.m_section ) ) );
            values.push_back( wxVariant( row.m_label ) );
            values.push_back( wxVariant( odb ? row.m_odb : row.m_ipc ) );
            m_sections->AppendItem( values );
        }

        if( odb )
        {
            m_intro->SetLabel(
                    _( "Choose sections to include. Intentional shorts are written when the board has net ties." ) );
            wxVector<wxVariant> values;
            values.push_back( wxVariant( aBoardMetadata ) );
            values.push_back( wxVariant( _( "board metadata" ) ) );
            values.push_back( wxVariant( wxS( "misc/metadata.xml" ) ) );
            m_metadataRow = m_sections->GetItemCount();
            m_sections->AppendItem( values );
            m_editable.push_back( true );

            values.clear();
            values.push_back( wxVariant( true ) );
            values.push_back( wxVariant( _( "intentional shorts" ) ) );
            values.push_back( wxVariant( wxS( "eda/shortf" ) ) );
            m_sections->AppendItem( values );
            m_editable.push_back( false );
        }

        m_sections->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DIALOG_FAB_CUSTOMIZE::onValueChanged, this );
        SetupStandardButtons();
        finishDialogSettings();
    }

    FAB::SECTION_SET Selected() const
    {
        FAB::SECTION_SET selected;

        for( size_t index = 0; index < m_rows.size(); ++index )
        {
            wxVariant value;
            m_sections->GetValue( value, static_cast<unsigned int>( index ), 0 );

            if( value.GetBool() )
                selected.Set( m_rows[index] );
        }

        return selected;
    }

    bool BoardMetadata() const
    {
        wxVariant value;
        m_sections->GetValue( value, m_metadataRow, 0 );
        return value.GetBool();
    }

private:
    void onValueChanged( wxDataViewEvent& aEvent )
    {
        if( m_restoring )
            return;

        unsigned int row = m_sections->ItemToRow( aEvent.GetItem() );

        if( row >= m_editable.size() || m_editable[row] )
            return;

        m_restoring = true;
        bool included = row < m_rows.size() && m_original.Contains( m_rows[row] );

        if( row >= m_rows.size() )
            included = true;

        m_sections->SetValue( wxVariant( included ), row, 0 );
        m_restoring = false;
    }

    std::vector<FAB::SECTION> m_rows;
    std::vector<bool>         m_editable;
    unsigned int              m_metadataRow = 0;
    FAB::SECTION_SET          m_original;
    bool                      m_restoring = false;
};
} // namespace


PANEL_FAB_EXPORT_CONTENT::PANEL_FAB_EXPORT_CONTENT( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                                                    const wxSize& aSize, long aStyle, const wxString& aName ) :
        PANEL_FAB_EXPORT_CONTENT_BASE( aParent, aId, aPos, aSize, aStyle, aName )
{
}


void PANEL_FAB_EXPORT_CONTENT::Configure( FAB_CONTENT_FORMAT aFormat, BOARD* aBoard )
{
    m_format = aFormat;
    m_board = aBoard;
    bool odb = aFormat == FAB_CONTENT_FORMAT::ODBPP;
    m_lblRefDes->Show( !odb );
    m_choiceRefDes->Show( !odb );
    m_choiceDataSet->Clear();
    m_modes.clear();

    auto addMode = [&]( FAB::MODE aMode, const wxString& aLabel )
    {
        m_modes.push_back( aMode );
        m_choiceDataSet->Append( aLabel );
    };

    addMode( FAB::MODE::USERDEF, _( "All data (user-defined)" ) );

    if( !odb )
        addMode( FAB::MODE::BOM, _( "Bill of materials" ) );

    addMode( FAB::MODE::STACKUP, _( "Stackup" ) );
    addMode( FAB::MODE::FABRICATION, _( "Fabrication" ) );
    addMode( FAB::MODE::ASSEMBLY, _( "Assembly" ) );
    addMode( FAB::MODE::TEST, _( "Test" ) );

    if( !odb )
        addMode( FAB::MODE::STENCIL, _( "Stencil" ) );

    m_choiceDataSet->SetSelection( 0 );
    m_choiceVariant->Clear();
    wxString current = aBoard ? aBoard->GetCurrentVariant() : wxString();

    if( current.IsEmpty() )
        current = GetDefaultVariantName();

    m_choiceVariant->Append( wxString::Format( _( "Current (%s)" ), current ) );

    if( aBoard )
    {
        for( const wxString& variant : aBoard->GetVariantNames() )
            m_choiceVariant->Append( variant );
    }

    if( odb )
    {
        m_choiceVariant->Append( _( "Selected variants…" ) );
        m_choiceVariant->Append( _( "All variants" ) );
    }

    m_choiceVariant->SetSelection( 0 );
    m_selectedChoice = 0;

    // Every entry names the same output when the board has no variants
    bool hasVariants = aBoard && !aBoard->GetVariantNames().empty();
    m_lblVariant->Enable( hasVariants );
    m_choiceVariant->Enable( hasVariants );
    m_variantOutput->Show( false );
    m_variantHint->Show( false );
    updateSummary();
}


FAB::MODE PANEL_FAB_EXPORT_CONTENT::GetDataSet() const
{
    int selection = m_choiceDataSet->GetSelection();
    return selection >= 0 && static_cast<size_t>( selection ) < m_modes.size() ? m_modes[selection]
                                                                               : FAB::MODE::USERDEF;
}


void PANEL_FAB_EXPORT_CONTENT::SetDataSet( FAB::MODE aMode )
{
    auto it = std::find( m_modes.begin(), m_modes.end(), aMode );
    m_choiceDataSet->SetSelection( it == m_modes.end() ? 0 : static_cast<int>( it - m_modes.begin() ) );
    updateSummary();
}


JOB_EXPORT_PCB_FAB::NET_NAMES PANEL_FAB_EXPORT_CONTENT::GetNetNames() const
{
    return m_choiceNetNames->GetSelection() == 1 ? JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE
                                                 : JOB_EXPORT_PCB_FAB::NET_NAMES::INCLUDE;
}


void PANEL_FAB_EXPORT_CONTENT::SetNetNames( JOB_EXPORT_PCB_FAB::NET_NAMES aNetNames )
{
    m_choiceNetNames->SetSelection( aNetNames == JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE ? 1 : 0 );
}


JOB_EXPORT_PCB_IPC2581::REF_DES PANEL_FAB_EXPORT_CONTENT::GetRefDes() const
{
    return m_choiceRefDes->GetSelection() == 1 ? JOB_EXPORT_PCB_IPC2581::REF_DES::OMIT
                                               : JOB_EXPORT_PCB_IPC2581::REF_DES::INCLUDE;
}


void PANEL_FAB_EXPORT_CONTENT::SetRefDes( JOB_EXPORT_PCB_IPC2581::REF_DES aRefDes )
{
    m_choiceRefDes->SetSelection( aRefDes == JOB_EXPORT_PCB_IPC2581::REF_DES::OMIT ? 1 : 0 );
}


void PANEL_FAB_EXPORT_CONTENT::SetSectionKey( const std::optional<wxString>& aKey )
{
    m_sectionKey = aKey;
    updateSummary();
}


void PANEL_FAB_EXPORT_CONTENT::SetBoardMetadata( bool aValue )
{
    m_boardMetadata = aValue;
    updateSummary();
}


std::vector<wxString> PANEL_FAB_EXPORT_CONTENT::GetVariantNames() const
{
    int selection = m_choiceVariant->GetSelection();

    if( !m_board || selection <= 0 )
        return {};

    const std::vector<wxString>& names = m_board->GetVariantNames();

    if( static_cast<size_t>( selection ) <= names.size() )
        return { names[selection - 1] };

    if( m_format == FAB_CONTENT_FORMAT::IPC2581 )
        return {};

    if( static_cast<size_t>( selection ) == names.size() + 1 )
        return m_selectedVariants;

    std::vector<wxString> all = { GetDefaultVariantName() };
    all.insert( all.end(), names.begin(), names.end() );
    return all;
}


void PANEL_FAB_EXPORT_CONTENT::SetVariantNames( const std::vector<wxString>& aNames )
{
    m_selectedVariants = aNames;
    int selection = 0;

    if( m_board && !aNames.empty() )
    {
        const std::vector<wxString>& names = m_board->GetVariantNames();

        if( aNames.size() == 1 )
        {
            auto it = std::find( names.begin(), names.end(), aNames.front() );

            if( it != names.end() )
                selection = static_cast<int>( it - names.begin() ) + 1;
        }

        if( selection == 0 && m_format == FAB_CONTENT_FORMAT::ODBPP )
            selection = static_cast<int>( names.size() ) + 1;
    }

    m_choiceVariant->SetSelection( selection );
    m_selectedChoice = selection;
    updateVariantOutput();
}


void PANEL_FAB_EXPORT_CONTENT::SetCombinedVariantOutput( bool aCombined )
{
    m_variantOutput->SetSelection( aCombined ? 1 : 0 );
    updateVariantOutput();
}


FAB::SECTION_SET PANEL_FAB_EXPORT_CONTENT::ResolvedSections() const
{
    FAB::MODE        mode = GetDataSet();
    FAB::SECTION_SET selected;

    if( m_sectionKey && FAB::SectionSetFromKeyString( *m_sectionKey, selected ) )
    {
        if( m_format == FAB_CONTENT_FORMAT::ODBPP && selected.Contains( FAB::SECTION::COMPONENTS ) )
            selected.Set( FAB::SECTION::PACKAGES );

        // Table 4 by itself only gives the schema sections
        return m_format == FAB_CONTENT_FORMAT::ODBPP
                       ? selected
                       : IPC2581::ResolveSections( IPC2581::REVISION::C, mode, selected ).m_included;
    }

    if( m_format == FAB_CONTENT_FORMAT::ODBPP )
        return OdbDefaultSections( mode );

    // Table 4 by itself only gives the schema sections
    return IPC2581::ResolveSections( IPC2581::REVISION::C, mode, FAB::RecommendedOptionalSections( mode ) )
            .m_included;
}


void PANEL_FAB_EXPORT_CONTENT::updateSummary()
{
    FAB::SECTION_SET      sections = ResolvedSections();
    wxString              summary = _( "Includes: " );
    wxString              line = summary;
    int                   lines = 1;
    std::vector<wxString> labels;

    for( const SECTION_ROW& row : sectionRows() )
    {
        if( sections.Contains( row.m_section ) )
            labels.push_back( row.m_label );
    }

    if( m_format == FAB_CONTENT_FORMAT::ODBPP && m_boardMetadata )
        labels.push_back( _( "board metadata" ) );

    for( const wxString& label : labels )
    {
        wxString item = label;
        bool     first = line == _( "Includes: " );
        item = first ? item : wxS( ", " ) + item;

        if( line.length() + item.length() > 70 && !first )
        {
            summary << wxS( "\n" );
            line.clear();
            ++lines;
            item = label;
        }

        summary << item;
        line << item;
    }

    m_lblIncludes->SetLabel( summary );
    m_lblIncludes->SetMinSize( wxSize( -1, lines * GetCharHeight() + 5 ) );
    m_btnBomFields->Enable(
            sections.Contains( FAB::SECTION::BOM_AVL )
            || ( m_format == FAB_CONTENT_FORMAT::ODBPP && sections.Contains( FAB::SECTION::COMPONENTS ) ) );
    Layout();

    if( GetParent() )
        GetParent()->Layout();
}


void PANEL_FAB_EXPORT_CONTENT::updateVariantOutput()
{
    bool multiple = m_format == FAB_CONTENT_FORMAT::ODBPP && GetVariantNames().size() > 1;
    m_variantOutput->Show( multiple );
    m_variantHint->Show( multiple && !IsCombinedVariantOutput() );
    Layout();

    if( GetParent() )
        GetParent()->Layout();
}


void PANEL_FAB_EXPORT_CONTENT::changed()
{
    if( m_contentChanged )
        m_contentChanged();
}


void PANEL_FAB_EXPORT_CONTENT::onDataSetChange( wxCommandEvent& aEvent )
{
    // A new function mode removes the section key of the previous function mode
    m_sectionKey.reset();
    updateSummary();
    changed();
}


void PANEL_FAB_EXPORT_CONTENT::onVariantChange( wxCommandEvent& aEvent )
{
    if( m_format == FAB_CONTENT_FORMAT::ODBPP && m_board )
    {
        const std::vector<wxString>& names = m_board->GetVariantNames();
        int                          selectedIndex = static_cast<int>( names.size() ) + 1;

        if( m_choiceVariant->GetSelection() == selectedIndex )
        {
            wxArrayString choices;
            choices.Add( GetDefaultVariantName() );

            for( const wxString& name : names )
                choices.Add( name );

            wxMultiChoiceDialog dialog( this, _( "Choose the variants to export" ), _( "Selected variants" ), choices );
            wxArrayInt          checked;

            for( const wxString& name : m_selectedVariants )
            {
                int index = choices.Index( name );

                if( index != wxNOT_FOUND )
                    checked.Add( index );
            }

            dialog.SetSelections( checked );

            if( dialog.ShowModal() != wxID_OK )
            {
                m_choiceVariant->SetSelection( m_selectedChoice );
                return;
            }

            m_selectedVariants.clear();

            for( int index : dialog.GetSelections() )
                m_selectedVariants.push_back( choices[index] );

            if( m_selectedVariants.empty() )
            {
                m_choiceVariant->SetSelection( m_selectedChoice );
                return;
            }
        }
    }

    m_selectedChoice = m_choiceVariant->GetSelection();
    updateVariantOutput();
    changed();
}


void PANEL_FAB_EXPORT_CONTENT::onVariantOutputChange( wxCommandEvent& aEvent )
{
    updateVariantOutput();
    changed();
}


void PANEL_FAB_EXPORT_CONTENT::onCustomizeClick( wxCommandEvent& aEvent )
{
    DIALOG_FAB_CUSTOMIZE dialog( this, m_format, GetDataSet(), ResolvedSections(), m_boardMetadata );

    if( dialog.ShowModal() != wxID_OK )
        return;

    m_sectionKey = m_format == FAB_CONTENT_FORMAT::ODBPP ? OdbSectionKeyForSelection( GetDataSet(), dialog.Selected() )
                                                         : FAB::SectionKeyString( dialog.Selected() );

    if( m_format == FAB_CONTENT_FORMAT::ODBPP && m_sectionKey->IsEmpty() )
        m_sectionKey.reset();

    if( m_format == FAB_CONTENT_FORMAT::ODBPP )
        m_boardMetadata = dialog.BoardMetadata();

    updateSummary();
    changed();
}


void PANEL_FAB_EXPORT_CONTENT::onBomFieldsClick( wxCommandEvent& aEvent )
{
    if( !m_board )
        return;

    DIALOG_EXPORT_2581_BOM dialog( this, m_board, m_bomFields, m_format == FAB_CONTENT_FORMAT::ODBPP );

    if( dialog.ShowModal() == wxID_OK )
        m_bomFields = dialog.GetFields();
}
