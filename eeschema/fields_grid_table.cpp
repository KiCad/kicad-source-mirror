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

#include <embedded_files.h>
#include <kiway.h>
#include <kiway_player.h>
#include <dialog_shim.h>
#include <fields_grid_table.h>
#include <sch_base_frame.h>
#include <sch_field.h>
#include <sch_label.h>
#include <sch_validators.h>
#include <validators.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <widgets/grid_text_button_helpers.h>
#include <wildcards_and_files_ext.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include "eda_doc.h"
#include "widgets/grid_color_swatch_helpers.h"
#include "font/fontconfig.h"
#include "font/kicad_font_name.h"
#include "widgets/grid_text_helpers.h"
#include <wx/settings.h>
#include <string_utils.h>
#include <widgets/grid_combobox.h>
#include <pgm_base.h>
#include <project_sch.h>


enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET
};


#define DEFAULT_FONT_NAME _( "Default Font" )


static wxString netList( SCH_SYMBOL* aSymbol, SCH_SHEET_PATH& aSheetPath )
{
    /*
     * Symbol netlist format:
     *   pinNumber pinName <tab> pinNumber pinName...
     *   fpFilter fpFilter...
     */
    wxString netlist;

    // We need the list of pins of the lib symbol, not just the pins of the current
    // sch symbol, that can be just an unit of a multi-unit symbol, to be able to
    // select/filter right footprints
    wxArrayString pins;

    const std::unique_ptr< LIB_SYMBOL >& lib_symbol = aSymbol->GetLibSymbolRef();

    if( lib_symbol )
    {
        for( SCH_PIN* pin : lib_symbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
        {
            bool                  valid = false;
            std::vector<wxString> expanded = pin->GetStackedPinNumbers( &valid );

            if( valid && !expanded.empty() )
            {
                for( const wxString& num : expanded )
                    pins.push_back( num + ' ' + pin->GetShownName() );
            }
            else
            {
                pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );
            }
        }
    }

    if( !pins.IsEmpty() )
    {
        wxString dbg = wxJoin( pins, '\t' );
        wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "Chooser payload pins: %s" ), dbg );
        netlist << EscapeString( dbg, CTX_LINE );
    }

    netlist << wxS( "\r" );

    if( lib_symbol )
    {
        wxArrayString fpFilters = lib_symbol->GetFPFilters();

        if( !fpFilters.IsEmpty() )
            netlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );
    }

    netlist << wxS( "\r" );

    return netlist;
}


static wxString netList( LIB_SYMBOL* aSymbol )
{
    /*
     * Symbol netlist format:
     *   pinNumber pinName <tab> pinNumber pinName...
     *   fpFilter fpFilter...
     */
    wxString      netlist;
    wxArrayString pins;

    for( SCH_PIN* pin : aSymbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
    {
        bool valid = false;
        std::vector<wxString> expanded = pin->GetStackedPinNumbers( &valid );

        if( valid && !expanded.empty() )
        {
            for( const wxString& num : expanded )
                pins.push_back( num + ' ' + pin->GetShownName() );
        }
        else
        {
            pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );
        }
    }

    if( !pins.IsEmpty() )
    {
        wxString dbg = wxJoin( pins, '\t' );
        wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "Chooser payload pins: %s" ), dbg );
        netlist << EscapeString( dbg, CTX_LINE );
    }

    netlist << wxS( "\r" );

    wxArrayString fpFilters = aSymbol->GetFPFilters();

    if( !fpFilters.IsEmpty() )
        netlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );

    netlist << wxS( "\r" );

    return netlist;
}


FIELDS_GRID_TABLE::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_BASE_FRAME* aFrame, WX_GRID* aGrid,
                                      LIB_SYMBOL* aSymbol, std::vector<EMBEDDED_FILES*> aFilesStack ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_parentType( SCH_SYMBOL_T ),
        m_part( aSymbol ),
        m_filesStack( aFilesStack ),
        m_symbolNetlist( netList( aSymbol ) ),
        m_fieldNameValidator( FIELD_T::USER ),
        m_referenceValidator( FIELD_T::REFERENCE ),
        m_valueValidator( FIELD_T::VALUE ),
        m_urlValidator( FIELD_T::USER ),
        m_nonUrlValidator( FIELD_T::USER ),
        m_filepathValidator( FIELD_T::SHEET_FILENAME )
{
    initGrid( aGrid );
}


FIELDS_GRID_TABLE::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                                      SCH_SYMBOL* aSymbol ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_parentType( SCH_SYMBOL_T ),
        m_part( aSymbol->GetLibSymbolRef().get() ),
        m_symbolNetlist( netList( aSymbol, aFrame->GetCurrentSheet() ) ),
        m_fieldNameValidator( FIELD_T::USER ),
        m_referenceValidator( FIELD_T::REFERENCE ),
        m_valueValidator( FIELD_T::VALUE ),
        m_urlValidator( FIELD_T::USER ),
        m_nonUrlValidator( FIELD_T::USER ),
        m_filepathValidator( FIELD_T::SHEET_FILENAME )
{
    m_filesStack.push_back( aSymbol->Schematic() );

    if( m_part )
        m_filesStack.push_back( m_part );

    initGrid( aGrid );
}


FIELDS_GRID_TABLE::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                                      SCH_SHEET* aSheet ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_parentType( SCH_SHEET_T ),
        m_part( nullptr ),
        m_fieldNameValidator( FIELD_T::USER ),
        m_referenceValidator( FIELD_T::SHEET_NAME ),
        m_valueValidator( FIELD_T::VALUE ),
        m_urlValidator( FIELD_T::USER ),
        m_nonUrlValidator( FIELD_T::USER ),
        m_filepathValidator( FIELD_T::SHEET_FILENAME )
{
    m_filesStack.push_back( aSheet->Schematic() );

    initGrid( aGrid );
}


FIELDS_GRID_TABLE::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame, WX_GRID* aGrid,
                                      SCH_LABEL_BASE* aLabel ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_parentType( SCH_LABEL_LOCATE_ANY_T ),
        m_part( nullptr ),
        m_fieldNameValidator( FIELD_T::USER ),
        m_referenceValidator( FIELD_T::USER ),
        m_valueValidator( FIELD_T::USER ),
        m_urlValidator( FIELD_T::USER ),
        m_nonUrlValidator( FIELD_T::USER ),
        m_filepathValidator( FIELD_T::USER )
{
    m_filesStack.push_back( aLabel->Schematic() );

    initGrid( aGrid );
}


int FIELDS_GRID_TABLE::GetMandatoryRowCount() const
{
    int mandatoryRows = 0;

    for( const SCH_FIELD& field : *this )
    {
        if( field.IsMandatory() )
            mandatoryRows++;
    }

    return mandatoryRows;
}


void FIELDS_GRID_TABLE::push_back( const SCH_FIELD& aField )
{
    std::vector<SCH_FIELD>::push_back( aField );

    m_isInherited.resize( size() );
    m_parentFields.resize( size() );
}


void FIELDS_GRID_TABLE::initGrid( WX_GRID* aGrid )
{
    // Build the various grid cell attributes.
    // NOTE: validators and cellAttrs are member variables to get the destruction order
    // right.  wxGrid is VERY cranky about this.

    m_readOnlyAttr = new wxGridCellAttr;
    m_readOnlyAttr->SetReadOnly( true );

    m_fieldNameAttr = new wxGridCellAttr;
    GRID_CELL_TEXT_EDITOR* nameEditor = new GRID_CELL_TEXT_EDITOR();
    nameEditor->SetValidator( m_fieldNameValidator );
    m_fieldNameAttr->SetEditor( nameEditor );

    m_referenceAttr = new wxGridCellAttr;
    GRID_CELL_TEXT_EDITOR* referenceEditor = new GRID_CELL_TEXT_EDITOR();
    referenceEditor->SetValidator( m_referenceValidator );
    m_referenceAttr->SetEditor( referenceEditor );

    m_valueAttr = new wxGridCellAttr;

    if( m_parentType == LIB_SYMBOL_T )
    {
        GRID_CELL_TEXT_EDITOR* valueEditor = new GRID_CELL_TEXT_EDITOR();
        valueEditor->SetValidator( m_valueValidator );
        m_valueAttr->SetEditor( valueEditor );
    }
    else
    {
        GRID_CELL_STC_EDITOR* valueEditor = new GRID_CELL_STC_EDITOR( true, true,
                [this]( wxStyledTextEvent& aEvent, SCINTILLA_TRICKS* aScintillaTricks )
                {
                    SCH_FIELD* valueField = this->GetField( FIELD_T::VALUE );
                    valueField->OnScintillaCharAdded( aScintillaTricks, aEvent );
                } );

        m_valueAttr->SetEditor( valueEditor );
    }

    m_footprintAttr = new wxGridCellAttr;
    GRID_CELL_FPID_EDITOR* fpIdEditor = new GRID_CELL_FPID_EDITOR( m_dialog, m_symbolNetlist );
    fpIdEditor->SetValidator( m_nonUrlValidator );
    m_footprintAttr->SetEditor( fpIdEditor );

    m_urlAttr = new wxGridCellAttr;
    SEARCH_STACK* prjSearchStack = PROJECT_SCH::SchSearchS( &m_frame->Prj() );
    GRID_CELL_URL_EDITOR* urlEditor = new GRID_CELL_URL_EDITOR( m_dialog, prjSearchStack, m_filesStack );
    urlEditor->SetValidator( m_urlValidator );
    m_urlAttr->SetEditor( urlEditor );

    m_nonUrlAttr = new wxGridCellAttr;
    GRID_CELL_TEXT_EDITOR* nonUrlEditor = new GRID_CELL_TEXT_EDITOR();
    nonUrlEditor->SetValidator( m_nonUrlValidator );
    m_nonUrlAttr->SetEditor( nonUrlEditor );

    m_curdir = m_frame->Prj().GetProjectPath();
    m_filepathAttr = new wxGridCellAttr;

    // Create a wild card using wxFileDialog syntax.
    wxString wildCard( _( "Schematic Files" ) );
    std::vector<std::string> exts;
    exts.push_back( FILEEXT::KiCadSchematicFileExtension );
    wildCard += AddFileExtListToFilter( exts );

    auto filepathEditor = new GRID_CELL_PATH_EDITOR( m_dialog, aGrid, &m_curdir, wildCard );
    filepathEditor->SetValidator( m_filepathValidator );
    m_filepathAttr->SetEditor( filepathEditor );

    m_boolAttr = new wxGridCellAttr;
    m_boolAttr->SetRenderer( new wxGridCellBoolRenderer() );
    m_boolAttr->SetEditor( new wxGridCellBoolEditor() );
    m_boolAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    wxArrayString vAlignNames;
    vAlignNames.Add( _( "Top" ) );
    vAlignNames.Add( _( "Center" ) );
    vAlignNames.Add( _( "Bottom" ) );
    m_vAlignAttr = new wxGridCellAttr;
    m_vAlignAttr->SetEditor( new wxGridCellChoiceEditor( vAlignNames ) );
    m_vAlignAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    wxArrayString hAlignNames;
    hAlignNames.Add( _( "Left" ) );
    hAlignNames.Add(_( "Center" ) );
    hAlignNames.Add(_( "Right" ) );
    m_hAlignAttr = new wxGridCellAttr;
    m_hAlignAttr->SetEditor( new wxGridCellChoiceEditor( hAlignNames ) );
    m_hAlignAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    wxArrayString orientationNames;
    orientationNames.Add( _( "Horizontal" ) );
    orientationNames.Add(_( "Vertical" ) );
    m_orientationAttr = new wxGridCellAttr;
    m_orientationAttr->SetEditor( new wxGridCellChoiceEditor( orientationNames ) );
    m_orientationAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
    wxArrayString   existingNetclasses;

    wxArrayString            fonts;
    std::vector<std::string> fontNames;

    if( editFrame )
    {
        // Load the combobox with existing existingNetclassNames
        PROJECT_FILE&                        projectFile = editFrame->Prj().GetProjectFile();
        const std::shared_ptr<NET_SETTINGS>& settings = projectFile.NetSettings();

        existingNetclasses.push_back( settings->GetDefaultNetclass()->GetName() );

        for( const auto& [name, netclass] : settings->GetNetclasses() )
            existingNetclasses.push_back( name );

        // We don't need to re-cache the embedded fonts when looking at symbols in the schematic
        // editor because the fonts are all available in the schematic.
        const std::vector<wxString>* fontFiles = nullptr;

        if( m_frame->GetScreen() && m_frame->GetScreen()->Schematic() )
            fontFiles = m_frame->GetScreen()->Schematic()->GetEmbeddedFiles()->GetFontFiles();

        Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ),
                                 fontFiles, false );
    }
    else
    {
        const std::vector<wxString>* fontFiles = m_part->GetEmbeddedFiles()->UpdateFontFiles();

        // If there are font files embedded, we want to re-cache our fonts for each symbol that
        // we are looking at in the symbol editor.
        Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ),
                                 fontFiles, !fontFiles->empty() );
    }

    m_netclassAttr = new wxGridCellAttr;
    m_netclassAttr->SetEditor( new GRID_CELL_COMBOBOX( existingNetclasses ) );

    for( const std::string& name : fontNames )
        fonts.Add( wxString( name ) );

    fonts.Sort();
    fonts.Insert( KICAD_FONT_NAME, 0 );
    fonts.Insert( DEFAULT_FONT_NAME, 0 );

    m_fontAttr = new wxGridCellAttr;
    m_fontAttr->SetEditor( new GRID_CELL_COMBOBOX( fonts ) );

    m_colorAttr = new wxGridCellAttr;
    m_colorAttr->SetRenderer( new GRID_CELL_COLOR_RENDERER( m_dialog ) );
    m_colorAttr->SetEditor( new GRID_CELL_COLOR_SELECTOR( m_dialog, aGrid ) );

    m_eval = std::make_unique<NUMERIC_EVALUATOR>( m_frame->GetUserUnits() );

    aGrid->SetupColumnAutosizer( FDC_VALUE );

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &FIELDS_GRID_TABLE::onUnitsChanged, this );
}


FIELDS_GRID_TABLE::~FIELDS_GRID_TABLE()
{
    m_readOnlyAttr->DecRef();
    m_fieldNameAttr->DecRef();
    m_boolAttr->DecRef();
    m_referenceAttr->DecRef();
    m_valueAttr->DecRef();
    m_footprintAttr->DecRef();
    m_urlAttr->DecRef();
    m_nonUrlAttr->DecRef();
    m_filepathAttr->DecRef();
    m_vAlignAttr->DecRef();
    m_hAlignAttr->DecRef();
    m_orientationAttr->DecRef();
    m_netclassAttr->DecRef();
    m_fontAttr->DecRef();
    m_colorAttr->DecRef();

    for( SCH_FIELD& field : m_parentFields )
        field.SetParent( nullptr );

    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &FIELDS_GRID_TABLE::onUnitsChanged, this );
}


void FIELDS_GRID_TABLE::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( GetView() )
        GetView()->ForceRefresh();

    aEvent.Skip();
}


int FIELDS_GRID_TABLE::getColumnCount() const
{
    if( m_frame->GetFrameType() == FRAME_SCH
        || m_frame->GetFrameType() == FRAME_SCH_VIEWER )
    {
        return FDC_SCH_EDIT_COUNT;
    }
    else
    {
        return FDC_SYMBOL_EDITOR_COUNT;
    }
}


int FIELDS_GRID_TABLE::getVisibleRowCount() const
{
    if( m_frame->GetFrameType() == FRAME_SCH
        || m_frame->GetFrameType() == FRAME_SCH_VIEWER )
    {
        int visibleRows = 0;

        for( const SCH_FIELD& field : *this )
        {
            if( !field.IsPrivate() )
                visibleRows++;
        }

        return visibleRows;
    }

    return (int) this->size();
}


SCH_FIELD& FIELDS_GRID_TABLE::getField( int aRow )
{
    if( m_frame->GetFrameType() == FRAME_SCH
        || m_frame->GetFrameType() == FRAME_SCH_VIEWER )
    {
        int visibleRow = 0;

        for( SCH_FIELD& field : *this )
        {
            if( field.IsPrivate() )
                continue;

            if( visibleRow == aRow )
                return field;

            ++visibleRow;
        }

        wxFAIL_MSG( wxT( "Row index off end of visible row count" ) );
    }

    return this->at( aRow );
}


wxString FIELDS_GRID_TABLE::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case FDC_NAME:            return _( "Name" );
    case FDC_VALUE:           return _( "Value" );
    case FDC_SHOWN:           return _( "Show" );
    case FDC_SHOW_NAME:       return _( "Show Name" );
    case FDC_H_ALIGN:         return _( "H Align" );
    case FDC_V_ALIGN:         return _( "V Align" );
    case FDC_ITALIC:          return _( "Italic" );
    case FDC_BOLD:            return _( "Bold" );
    case FDC_TEXT_SIZE:       return _( "Text Size" );
    case FDC_ORIENTATION:     return _( "Orientation" );
    case FDC_POSX:            return _( "X Position" );
    case FDC_POSY:            return _( "Y Position" );
    case FDC_FONT:            return _( "Font" );
    case FDC_COLOR:           return _( "Color" );
    case FDC_ALLOW_AUTOPLACE: return _( "Allow Autoplacement" );
    case FDC_PRIVATE:         return _( "Private" );
    default:      wxFAIL;     return wxEmptyString;
    }
}


bool FIELDS_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    switch( aCol )
    {
    case FDC_NAME:
    case FDC_VALUE:
    case FDC_H_ALIGN:
    case FDC_V_ALIGN:
    case FDC_TEXT_SIZE:
    case FDC_ORIENTATION:
    case FDC_POSX:
    case FDC_POSY:
    case FDC_FONT:
    case FDC_COLOR:
        return aTypeName == wxGRID_VALUE_STRING;

    case FDC_SHOWN:
    case FDC_SHOW_NAME:
    case FDC_ITALIC:
    case FDC_BOLD:
    case FDC_ALLOW_AUTOPLACE:
    case FDC_PRIVATE:
        return aTypeName == wxGRID_VALUE_BOOL;

    default:
        wxFAIL;
        return false;
    }
}


bool FIELDS_GRID_TABLE::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


wxGridCellAttr* FIELDS_GRID_TABLE::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind  )
{
    wxCHECK( aRow < GetNumberRows(), nullptr );

    const SCH_FIELD& field = getField( aRow );
    wxGridCellAttr*  attr = nullptr;

    switch( aCol )
    {
    case FDC_NAME:
        if( field.IsMandatory() )
        {
            attr = m_fieldNameAttr->Clone();
            attr->SetReadOnly( true );
        }
        else
        {
            m_fieldNameAttr->IncRef();
            attr = m_fieldNameAttr;
        }

        break;

    case FDC_VALUE:
        if( field.GetId() == FIELD_T::REFERENCE )
        {
            m_referenceAttr->IncRef();
            attr = m_referenceAttr;
        }
        else if( field.GetId() == FIELD_T::VALUE )
        {
            m_valueAttr->IncRef();
            attr = m_valueAttr;
        }
        else if( field.GetId() == FIELD_T::FOOTPRINT )
        {
            // Power symbols have do not appear in the board, so don't allow
            // a footprint (m_part can be nullptr when loading a old schematic
            // (for instance Kicad 4) with libraries missing)
            if( m_part && m_part->IsPower() )
            {
                m_readOnlyAttr->IncRef();
                attr = m_readOnlyAttr;
            }
            else
            {
                m_footprintAttr->IncRef();
                attr = m_footprintAttr;
            }
        }
        else if( field.GetId() == FIELD_T::DATASHEET )
        {
            m_urlAttr->IncRef();
            attr = m_urlAttr;
        }
        else if( field.GetId() == FIELD_T::SHEET_NAME )
        {
            m_referenceAttr->IncRef();
            attr = m_referenceAttr;
        }
        else if( field.GetId() == FIELD_T::SHEET_FILENAME )
        {
            m_filepathAttr->IncRef();
            attr = m_filepathAttr;
        }
        else if( ( m_parentType == SCH_LABEL_LOCATE_ANY_T )
                && field.GetCanonicalName() == wxT( "Netclass" ) )
        {
            m_netclassAttr->IncRef();
            attr = m_netclassAttr;
        }
        else
        {
            wxString fn = GetValue( aRow, FDC_NAME );

            SCHEMATIC_SETTINGS* settings = m_frame->Prj().GetProjectFile().m_SchematicSettings;

            const TEMPLATE_FIELDNAME* templateFn =
                    settings ? settings->m_TemplateFieldNames.GetFieldName( fn ) : nullptr;

            if( ( templateFn && templateFn->m_URL ) || field.HasHypertext() )
            {
                m_urlAttr->IncRef();
                attr = m_urlAttr;
            }
            else
            {
                m_nonUrlAttr->IncRef();
                attr = m_nonUrlAttr;
            }
        }

        break;

    case FDC_TEXT_SIZE:
    case FDC_POSX:
    case FDC_POSY:
        break;

    case FDC_H_ALIGN:
        m_hAlignAttr->IncRef();
        attr = m_hAlignAttr;
        break;

    case FDC_V_ALIGN:
        m_vAlignAttr->IncRef();
        attr = m_vAlignAttr;
        break;

    case FDC_ORIENTATION:
        m_orientationAttr->IncRef();
        attr = m_orientationAttr;
        break;

    case FDC_SHOWN:
    case FDC_SHOW_NAME:
    case FDC_ITALIC:
    case FDC_BOLD:
    case FDC_ALLOW_AUTOPLACE:
    case FDC_PRIVATE:
        m_boolAttr->IncRef();
        attr = m_boolAttr;
        break;

    case FDC_FONT:
        m_fontAttr->IncRef();
        attr = m_fontAttr;
        break;

    case FDC_COLOR:
        m_colorAttr->IncRef();
        attr = m_colorAttr;
        break;

    default:
        attr = nullptr;
        break;
    }

    if( !attr )
        return nullptr;

    attr = enhanceAttr( attr, aRow, aCol, aKind );

    if( IsInherited( aRow ) )
    {
        wxGridCellAttr* text_attr = attr ? attr->Clone() : new wxGridCellAttr;
        wxFont font;

        if( !text_attr->HasFont() )
            font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
        else
            font = text_attr->GetFont();

        font.MakeItalic();
        text_attr->SetFont( font );
        text_attr->SetTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

        if( attr )
            attr->DecRef();

        attr = text_attr;
    }

    return attr;
}


wxString FIELDS_GRID_TABLE::GetValue( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), wxEmptyString );

    wxGrid*          grid = GetView();
    const SCH_FIELD& field = getField( aRow );

    if( grid->GetGridCursorRow() == aRow && grid->GetGridCursorCol() == aCol
            && grid->IsCellEditControlShown() )
    {
        auto it = m_evalOriginal.find( { aRow, aCol } );

        if( it != m_evalOriginal.end() )
            return it->second;
    }

    switch( aCol )
    {
    case FDC_NAME:
        // Use default field names for mandatory and system fields because they are translated
        // according to the current locale
        if( m_parentType == SCH_LABEL_LOCATE_ANY_T )
        {
            return SCH_LABEL_BASE::GetDefaultFieldName( field.GetCanonicalName(), false );
        }
        else
        {
            if( field.IsMandatory() )
                return GetDefaultFieldName( field.GetId(), DO_TRANSLATE );
            else
                return field.GetName( false );
        }

    case FDC_VALUE:
        return EscapeString( UnescapeString( field.GetText() ), CTX_LINE );

    case FDC_SHOWN:
        return StringFromBool( field.IsVisible() );

    case FDC_SHOW_NAME:
        return StringFromBool( field.IsNameShown() );

    case FDC_H_ALIGN:
        switch ( field.GetEffectiveHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:          return _( "Left" );
        case GR_TEXT_H_ALIGN_CENTER:        return _( "Center" );
        case GR_TEXT_H_ALIGN_RIGHT:         return _( "Right" );
        case GR_TEXT_H_ALIGN_INDETERMINATE: return INDETERMINATE_STATE;
        }

        break;

    case FDC_V_ALIGN:
        switch ( field.GetEffectiveVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:           return _( "Top" );
        case GR_TEXT_V_ALIGN_CENTER:        return _( "Center" );
        case GR_TEXT_V_ALIGN_BOTTOM:        return _( "Bottom" );
        case GR_TEXT_V_ALIGN_INDETERMINATE: return INDETERMINATE_STATE;
        }

        break;

    case FDC_ITALIC:
        return StringFromBool( field.IsItalic() );

    case FDC_BOLD:
        return StringFromBool( field.IsBold() );

    case FDC_TEXT_SIZE:
        return m_frame->StringFromValue( field.GetTextHeight(), true );

    case FDC_ORIENTATION:
        if( field.GetTextAngle().IsHorizontal() )
            return _( "Horizontal" );
        else
            return _( "Vertical" );

    case FDC_POSX:
        return m_frame->StringFromValue( field.GetTextPos().x, true );

    case FDC_POSY:
        return m_frame->StringFromValue( field.GetTextPos().y, true );

    case FDC_FONT:
        if( field.GetFont() )
            return field.GetFont()->GetName();
        else
            return DEFAULT_FONT_NAME;

    case FDC_COLOR:
        return field.GetTextColor().ToCSSString();

    case FDC_ALLOW_AUTOPLACE:
        return StringFromBool( field.CanAutoplace() );

    case FDC_PRIVATE:
        return StringFromBool( field.IsPrivate() );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        break;
    }

    return wxT( "bad wxWidgets!" );
}


bool FIELDS_GRID_TABLE::GetValueAsBool( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), false );
    const SCH_FIELD& field = getField( aRow );

    switch( aCol )
    {
    case FDC_SHOWN:           return field.IsVisible();
    case FDC_SHOW_NAME:       return field.IsNameShown();
    case FDC_ITALIC:          return field.IsItalic();
    case FDC_BOLD:            return field.IsBold();
    case FDC_ALLOW_AUTOPLACE: return field.CanAutoplace();
    case FDC_PRIVATE:         return field.IsPrivate();
    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


void FIELDS_GRID_TABLE::SetValue( int aRow, int aCol, const wxString &aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );
    SCH_FIELD& field = getField( aRow );
    VECTOR2I   pos;
    wxString   value = aValue;

    if( aCol != FDC_VALUE )
        value.Trim( true ).Trim( false );

    if( aCol == FDC_TEXT_SIZE || aCol == FDC_POSX || aCol == FDC_POSY )
    {
        m_eval->SetDefaultUnits( m_frame->GetUserUnits() );

        if( m_eval->Process( value ) )
        {
            m_evalOriginal[ { aRow, aCol } ] = value;
            value = m_eval->Result();
        }
    }

    switch( aCol )
    {
    case FDC_NAME:
        field.SetName( value );
        break;

    case FDC_VALUE:
        if( m_parentType == SCH_SHEET_T && field.GetId() == FIELD_T::SHEET_FILENAME )
        {
            value = EnsureFileExtension( value, FILEEXT::KiCadSchematicFileExtension );
        }
        else if( m_parentType == LIB_SYMBOL_T && field.GetId() == FIELD_T::VALUE )
        {
            value = EscapeString( value, CTX_LIBID );
        }

        field.SetText( UnescapeString( value ) );
        break;

    case FDC_SHOWN:
        field.SetVisible( BoolFromString( value ) );
        break;

    case FDC_SHOW_NAME:
        field.SetNameShown( BoolFromString( value ) );
        break;

    case FDC_H_ALIGN:
    {
        GR_TEXT_H_ALIGN_T horizontalJustification = GR_TEXT_H_ALIGN_CENTER;

        if( value == _( "Left" ) )
            horizontalJustification = GR_TEXT_H_ALIGN_LEFT;
        else if( value == _( "Center" ) )
            horizontalJustification = GR_TEXT_H_ALIGN_CENTER;
        else if( value == _( "Right" ) )
            horizontalJustification = GR_TEXT_H_ALIGN_RIGHT;

        // Note that we must set justifications before we can ask if they're flipped.  If the old
        // justification is center then it won't know (whereas if the new justification is center
        // the we don't care).
        field.SetHorizJustify( horizontalJustification );

        if( field.IsHorizJustifyFlipped() )
            field.SetHorizJustify( EDA_TEXT::MapHorizJustify( - horizontalJustification ) );

        break;
    }

    case FDC_V_ALIGN:
    {
        GR_TEXT_V_ALIGN_T verticalJustification = GR_TEXT_V_ALIGN_BOTTOM;

        if( value == _( "Top" ) )
            verticalJustification = GR_TEXT_V_ALIGN_TOP;
        else if( value == _( "Center" ) )
            verticalJustification = GR_TEXT_V_ALIGN_CENTER;
        else if( value == _( "Bottom" ) )
            verticalJustification = GR_TEXT_V_ALIGN_BOTTOM;

        // Note that we must set justifications before we can ask if they're flipped.  If the old
        // justification is center then it won't know (whereas if the new justification is center
        // the we don't care).
        field.SetVertJustify( verticalJustification );

        if( field.IsVertJustifyFlipped() )
            field.SetVertJustify( EDA_TEXT::MapVertJustify( -verticalJustification ) );

        break;
    }

    case FDC_ITALIC:
        field.SetItalic( BoolFromString( value ) );
        break;

    case FDC_BOLD:
        field.SetBold( BoolFromString( value ) );
        break;

    case FDC_TEXT_SIZE:
        field.SetTextSize( VECTOR2I( m_frame->ValueFromString( value ), m_frame->ValueFromString( value ) ) );
        break;

    case FDC_ORIENTATION:
        if( value == _( "Horizontal" ) )
            field.SetTextAngle( ANGLE_HORIZONTAL );
        else if( value == _( "Vertical" ) )
            field.SetTextAngle( ANGLE_VERTICAL );
        else
            wxFAIL_MSG( wxT( "unknown orientation: " ) + value );

        break;

    case FDC_POSX:
    case FDC_POSY:
        pos = field.GetTextPos();

        if( aCol == FDC_POSX )
            pos.x = m_frame->ValueFromString( value );
        else
            pos.y = m_frame->ValueFromString( value );

        field.SetTextPos( pos );
        break;

    case FDC_FONT:
        if( value == DEFAULT_FONT_NAME )
            field.SetFont( nullptr );
        else if( value == KICAD_FONT_NAME )
            field.SetFont( KIFONT::FONT::GetFont( wxEmptyString, field.IsBold(), field.IsItalic() ) );
        else
            field.SetFont( KIFONT::FONT::GetFont( aValue, field.IsBold(), field.IsItalic() ) );

        break;

    case FDC_COLOR:
        field.SetTextColor( wxColor( value ) );
        break;

    case FDC_ALLOW_AUTOPLACE:
        field.SetCanAutoplace( BoolFromString( value ) );
        break;

    case FDC_PRIVATE:
        field.SetPrivate( BoolFromString( value ) );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
        break;
    }

    m_dialog->OnModify();

    GetView()->Refresh();
}


void FIELDS_GRID_TABLE::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );
    SCH_FIELD& field = getField( aRow );

    switch( aCol )
    {
    case FDC_SHOWN:
        field.SetVisible( aValue );
        break;

    case FDC_SHOW_NAME:
        field.SetNameShown( aValue );
        break;

    case FDC_ITALIC:
        field.SetItalic( aValue );
        break;

    case FDC_BOLD:
        field.SetBold( aValue );
        break;

    case FDC_ALLOW_AUTOPLACE:
        field.SetCanAutoplace( aValue );
        break;

    case FDC_PRIVATE:
        field.SetPrivate( aValue );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }

    m_dialog->OnModify();
}


wxString FIELDS_GRID_TABLE::StringFromBool( bool aValue ) const
{
    if( aValue )
        return wxT( "1" );
    else
        return wxT( "0" );
}


bool FIELDS_GRID_TABLE::BoolFromString( const wxString& aValue ) const
{
    if( aValue == wxS( "1" ) )
    {
        return true;
    }
    else if( aValue == wxS( "0" ) )
    {
        return false;
    }
    else
    {
        wxFAIL_MSG( wxString::Format( "string '%s' can't be converted to boolean correctly and "
                                      "will be perceived as FALSE", aValue ) );
        return false;
    }
}


SCH_FIELD* FIELDS_GRID_TABLE::GetField( FIELD_T aFieldId )
{
    for( SCH_FIELD& field : *this )
    {
        if( field.GetId() == aFieldId )
            return &field;
    }

    return nullptr;
}


int FIELDS_GRID_TABLE::GetFieldRow( FIELD_T aFieldId )
{
    for( int ii = 0; ii < (int) this->size(); ++ii )
    {
        if( this->at( ii ).GetId() == aFieldId )
            return ii;
    }

    return -1;
}

void FIELDS_GRID_TABLE::AddInheritedField( const SCH_FIELD& aParent )
{
    push_back( aParent );
    back().SetParent( m_part );
    m_isInherited.back() = true;
    m_parentFields.back() = aParent;
}

bool FIELDS_GRID_TABLE::EraseRow( size_t aRow )
{
    if( m_isInherited.size() > aRow )
    {
        // You can't erase inherited fields, but you can reset them to the parent value.
        if( m_isInherited[aRow] )
        {
            at( aRow ) = m_parentFields[aRow];
            return false;
        }

        m_isInherited.erase( m_isInherited.begin() + aRow );
    }

    if( m_parentFields.size() > aRow )
        m_parentFields.erase( m_parentFields.begin() + aRow );

    std::vector<SCH_FIELD>::erase( begin() + aRow );
    return true;
}

void FIELDS_GRID_TABLE::SwapRows( size_t a, size_t b )
{
    wxCHECK( a < this->size() && b < this->size(), /*void*/ );

    std::swap( at( a ), at( b ) );

    bool tmpInherited = m_isInherited[a];
    m_isInherited[a] = m_isInherited[b];
    m_isInherited[b] = tmpInherited;

    std::swap( m_parentFields[a], m_parentFields[b] );
}


void FIELDS_GRID_TABLE::DetachFields()
{
    for( SCH_FIELD& field : *this )
        field.SetParent( nullptr );

    for( SCH_FIELD& field : m_parentFields )
        field.SetParent( nullptr );
}


int FIELDS_GRID_TRICKS::getFieldRow( FIELD_T aFieldId )
{
    return static_cast<FIELDS_GRID_TABLE*>( m_grid->GetTable() )->GetFieldRow( aFieldId );
}


void FIELDS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    if( m_grid->GetGridCursorRow() == getFieldRow( FIELD_T::FOOTPRINT )
        && m_grid->GetGridCursorCol() == FDC_VALUE
        && !m_grid->IsReadOnly( getFieldRow( FIELD_T::FOOTPRINT ), FDC_VALUE ) )
    {
        menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ),
                     _( "Browse for footprint" ) );
        menu.AppendSeparator();
    }
    else if( m_grid->GetGridCursorRow() == getFieldRow( FIELD_T::DATASHEET )
           && m_grid->GetGridCursorCol() == FDC_VALUE
           && !m_grid->IsReadOnly( getFieldRow( FIELD_T::DATASHEET ), FDC_VALUE ) )
    {
        menu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ),
                     _( "Show datasheet in browser" ) );
        menu.AppendSeparator();
    }

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void FIELDS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == MYID_SELECT_FOOTPRINT )
    {
        // pick a footprint using the footprint picker.
        wxString fpid = m_grid->GetCellValue( getFieldRow( FIELD_T::FOOTPRINT ), FDC_VALUE );

        if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
        {
            if( frame->ShowModal( &fpid, m_dlg ) )
                m_grid->SetCellValue( getFieldRow( FIELD_T::FOOTPRINT ), FDC_VALUE, fpid );

            frame->Destroy();
        }
    }
    else if (event.GetId() == MYID_SHOW_DATASHEET )
    {
        wxString datasheet_uri = m_grid->GetCellValue( getFieldRow( FIELD_T::DATASHEET ), FDC_VALUE );

        GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), PROJECT_SCH::SchSearchS( &m_dlg->Prj() ),
                               m_filesStack );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


