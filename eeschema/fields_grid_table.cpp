/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_library.h>
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
#include <wx/settings.h>
#include <string_utils.h>
#include <widgets/grid_combobox.h>
#include <pgm_base.h>


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
     *   library:footprint
     *   reference
     *   value
     *   pinName,netName,pinFunction,pinType
     *   pinName,netName,pinFunction,pinType
     *   ...
     */
    wxString netlist;

    netlist << EscapeString( aSymbol->GetFootprintFieldText( true, &aSheetPath, false ), CTX_LINE ) << wxS( "\r" );
    netlist << EscapeString( aSymbol->GetRef( &aSheetPath ), CTX_LINE ) << wxS( "\r" );
    netlist << EscapeString( aSymbol->GetValueFieldText( true, &aSheetPath, false ), CTX_LINE );

    for( SCH_PIN* pin : aSymbol->GetPins( &aSheetPath ) )
    {
        netlist << wxS( "\r" );
        netlist << EscapeString( pin->GetNumber(), CTX_CSV ) << wxS( "," );
        netlist << EscapeString( pin->GetDefaultNetName( aSheetPath ), CTX_CSV ) << wxS( "," );
        netlist << EscapeString( pin->GetName(), CTX_CSV ) << wxS( "," );
        netlist << EscapeString( pin->GetCanonicalElectricalTypeName(), CTX_CSV );
    }

    return netlist;
}


template <class T>
FIELDS_GRID_TABLE<T>::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_BASE_FRAME* aFrame,
                                         WX_GRID* aGrid, LIB_SYMBOL* aSymbol ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_grid( aGrid ),
        m_parentType( SCH_SYMBOL_T ),
        m_mandatoryFieldCount( MANDATORY_FIELDS ),
        m_part( aSymbol ),
        m_fieldNameValidator( FIELD_NAME ),
        m_referenceValidator( REFERENCE_FIELD ),
        m_valueValidator( VALUE_FIELD ),
        m_urlValidator( FIELD_VALUE ),
        m_nonUrlValidator( FIELD_VALUE ),
        m_filepathValidator( SHEETFILENAME )
{
    initGrid( aGrid );
}


template <class T>
FIELDS_GRID_TABLE<T>::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame,
                                         WX_GRID* aGrid, SCH_SYMBOL* aSymbol ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_grid( aGrid ),
        m_parentType( SCH_SYMBOL_T ),
        m_mandatoryFieldCount( MANDATORY_FIELDS ),
        m_part( aSymbol->GetLibSymbolRef().get() ),
        m_symbolNetlist( netList( aSymbol, aFrame->GetCurrentSheet() ) ),
        m_fieldNameValidator( FIELD_NAME ),
        m_referenceValidator( REFERENCE_FIELD ),
        m_valueValidator( VALUE_FIELD ),
        m_urlValidator( FIELD_VALUE ),
        m_nonUrlValidator( FIELD_VALUE ),
        m_filepathValidator( SHEETFILENAME )
{
    initGrid( aGrid );
}


template <class T>
FIELDS_GRID_TABLE<T>::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame,
                                         WX_GRID* aGrid, SCH_SHEET* aSheet ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_grid( aGrid ),
        m_parentType( SCH_SHEET_T ),
        m_mandatoryFieldCount( SHEET_MANDATORY_FIELDS ),
        m_part( nullptr ),
        m_fieldNameValidator( FIELD_NAME ),
        m_referenceValidator( SHEETNAME_V ),
        m_valueValidator( VALUE_FIELD ),
        m_urlValidator( FIELD_VALUE ),
        m_nonUrlValidator( FIELD_VALUE ),
        m_filepathValidator( SHEETFILENAME_V )
{
    initGrid( aGrid );
}


template <class T>
FIELDS_GRID_TABLE<T>::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, SCH_EDIT_FRAME* aFrame,
                                         WX_GRID* aGrid, SCH_LABEL_BASE* aLabel ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_grid( aGrid ),
        m_parentType( SCH_LABEL_LOCATE_ANY_T ),
        m_mandatoryFieldCount( aLabel->GetMandatoryFieldCount() ),
        m_part( nullptr ),
        m_fieldNameValidator( FIELD_NAME ),
        m_referenceValidator( 0 ),
        m_valueValidator( 0 ),
        m_urlValidator( FIELD_VALUE ),
        m_nonUrlValidator( FIELD_VALUE ),
        m_filepathValidator( 0 )
{
    initGrid( aGrid );
}


template <class T>
void FIELDS_GRID_TABLE<T>::initGrid( WX_GRID* aGrid )
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
    GRID_CELL_TEXT_EDITOR* valueEditor = new GRID_CELL_TEXT_EDITOR();
    valueEditor->SetValidator( m_valueValidator );
    m_valueAttr->SetEditor( valueEditor );

    m_footprintAttr = new wxGridCellAttr;
    GRID_CELL_FPID_EDITOR* fpIdEditor = new GRID_CELL_FPID_EDITOR( m_dialog, m_symbolNetlist );
    fpIdEditor->SetValidator( m_nonUrlValidator );
    m_footprintAttr->SetEditor( fpIdEditor );

    m_urlAttr = new wxGridCellAttr;
    GRID_CELL_URL_EDITOR* urlEditor = new GRID_CELL_URL_EDITOR( m_dialog,
                                                                m_frame->Prj().SchSearchS() );
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
    exts.push_back( KiCadSchematicFileExtension );
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
    m_vAlignAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    wxArrayString hAlignNames;
    hAlignNames.Add( _( "Left" ) );
    hAlignNames.Add(_( "Center" ) );
    hAlignNames.Add(_( "Right" ) );
    m_hAlignAttr = new wxGridCellAttr;
    m_hAlignAttr->SetEditor( new wxGridCellChoiceEditor( hAlignNames ) );
    m_hAlignAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    wxArrayString orientationNames;
    orientationNames.Add( _( "Horizontal" ) );
    orientationNames.Add(_( "Vertical" ) );
    m_orientationAttr = new wxGridCellAttr;
    m_orientationAttr->SetEditor( new wxGridCellChoiceEditor( orientationNames ) );
    m_orientationAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
    wxArrayString   existingNetclasses;

    if( editFrame )
    {
        // Load the combobox with existing existingNetclassNames
        PROJECT_FILE&                        projectFile = editFrame->Prj().GetProjectFile();
        const std::shared_ptr<NET_SETTINGS>& settings = projectFile.NetSettings();

        existingNetclasses.push_back( settings->m_DefaultNetClass->GetName() );

        for( const auto& [ name, netclass ] : settings->m_NetClasses )
            existingNetclasses.push_back( name );
    }

    m_netclassAttr = new wxGridCellAttr;
    m_netclassAttr->SetEditor( new GRID_CELL_COMBOBOX( existingNetclasses ) );

    wxArrayString            fonts;
    std::vector<std::string> fontNames;
    Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ) );

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

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &FIELDS_GRID_TABLE<T>::onUnitsChanged, this );
}


template <class T>
FIELDS_GRID_TABLE<T>::~FIELDS_GRID_TABLE()
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

    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &FIELDS_GRID_TABLE<T>::onUnitsChanged, this );
}


template <class T>
void FIELDS_GRID_TABLE<T>::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( GetView() )
        GetView()->ForceRefresh();

    aEvent.Skip();
}


template <class T>
wxString FIELDS_GRID_TABLE<T>::GetColLabelValue( int aCol )
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
    default:      wxFAIL;     return wxEmptyString;
    }
}


template <class T>
bool FIELDS_GRID_TABLE<T>::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
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
        return aTypeName == wxGRID_VALUE_BOOL;

    default:
        wxFAIL;
        return false;
    }
}


template <class T>
bool FIELDS_GRID_TABLE<T>::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


template <class T>
wxGridCellAttr* FIELDS_GRID_TABLE<T>::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind  )
{
    wxGridCellAttr* tmp;

    switch( aCol )
    {
    case FDC_NAME:
        if( aRow < m_mandatoryFieldCount )
        {
            tmp = m_fieldNameAttr->Clone();
            tmp->SetReadOnly( true );
            return tmp;
        }
        else
        {
            m_fieldNameAttr->IncRef();
            return m_fieldNameAttr;
        }

    case FDC_VALUE:
        if( m_parentType == SCH_SYMBOL_T && aRow == REFERENCE_FIELD )
        {
            m_referenceAttr->IncRef();
            return m_referenceAttr;
        }
        else if( m_parentType == SCH_SYMBOL_T && aRow == VALUE_FIELD )
        {
            m_valueAttr->IncRef();
            return m_valueAttr;
        }
        else if( m_parentType == SCH_SYMBOL_T && aRow == FOOTPRINT_FIELD )
        {
            m_footprintAttr->IncRef();
            return m_footprintAttr;
        }
        else if( m_parentType == SCH_SYMBOL_T && aRow == DATASHEET_FIELD )
        {
            m_urlAttr->IncRef();
            return m_urlAttr;
        }
        else if( m_parentType == SCH_SHEET_T && aRow == SHEETNAME )
        {
            m_referenceAttr->IncRef();
            return m_referenceAttr;
        }
        else if( m_parentType == SCH_SHEET_T && aRow == SHEETFILENAME )
        {
            m_filepathAttr->IncRef();
            return m_filepathAttr;
        }
        else if( ( m_parentType == SCH_LABEL_LOCATE_ANY_T )
                && this->at( (size_t) aRow ).GetCanonicalName() == wxT( "Netclass" ) )
        {
            m_netclassAttr->IncRef();
            return m_netclassAttr;
        }
        else
        {
            wxString fn = GetValue( aRow, FDC_NAME );

            SCHEMATIC_SETTINGS* settings = m_frame->Prj().GetProjectFile().m_SchematicSettings;

            const TEMPLATE_FIELDNAME* templateFn =
                    settings ? settings->m_TemplateFieldNames.GetFieldName( fn ) : nullptr;

            if( templateFn && templateFn->m_URL )
            {
                m_urlAttr->IncRef();
                return m_urlAttr;
            }
            else
            {
                m_nonUrlAttr->IncRef();
                return m_nonUrlAttr;
            }
        }

    case FDC_TEXT_SIZE:
    case FDC_POSX:
    case FDC_POSY:
        return nullptr;

    case FDC_H_ALIGN:
        m_hAlignAttr->IncRef();
        return m_hAlignAttr;

    case FDC_V_ALIGN:
        m_vAlignAttr->IncRef();
        return m_vAlignAttr;

    case FDC_ORIENTATION:
        m_orientationAttr->IncRef();
        return m_orientationAttr;

    case FDC_SHOWN:
    case FDC_SHOW_NAME:
    case FDC_ITALIC:
    case FDC_BOLD:
    case FDC_ALLOW_AUTOPLACE:
        m_boolAttr->IncRef();
        return m_boolAttr;

    case FDC_FONT:
        m_fontAttr->IncRef();
        return m_fontAttr;

    case FDC_COLOR:
        m_colorAttr->IncRef();
        return m_colorAttr;

    default:
        wxFAIL;
        return nullptr;
    }
}


template <class T>
wxString FIELDS_GRID_TABLE<T>::GetValue( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), wxEmptyString );

    wxGrid*  grid = GetView();
    const T& field = this->at( (size_t) aRow );

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
        if( m_parentType == SCH_SYMBOL_T )
        {
            if( aRow < m_mandatoryFieldCount )
                return TEMPLATE_FIELDNAME::GetDefaultFieldName( aRow, DO_TRANSLATE );
            else
                return field.GetName( false );
        }
        else if( m_parentType == SCH_SHEET_T )
        {
            if( aRow < m_mandatoryFieldCount )
                return SCH_SHEET::GetDefaultFieldName( aRow );
            else
                return field.GetName( false );
        }
        else if( m_parentType == SCH_LABEL_LOCATE_ANY_T )
        {
            return SCH_LABEL_BASE::GetDefaultFieldName( field.GetCanonicalName(), false );
        }
        else
        {
            wxFAIL_MSG( wxS( "Unhandled field owner type." ) );
            return field.GetName( false );
        }

    case FDC_VALUE:
        return UnescapeString( field.GetText() );

    case FDC_SHOWN:
        return StringFromBool( field.IsVisible() );

    case FDC_SHOW_NAME:
        return StringFromBool( field.IsNameShown() );

    case FDC_H_ALIGN:
        switch ( field.GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   return _( "Left" );
        case GR_TEXT_H_ALIGN_CENTER: return _( "Center" );
        case GR_TEXT_H_ALIGN_RIGHT:  return _( "Right" );
        }

        break;

    case FDC_V_ALIGN:
        switch ( field.GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:    return _( "Top" );
        case GR_TEXT_V_ALIGN_CENTER: return _( "Center" );
        case GR_TEXT_V_ALIGN_BOTTOM: return _( "Bottom" );
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

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        break;
    }

    return wxT( "bad wxWidgets!" );
}


template <class T>
bool FIELDS_GRID_TABLE<T>::GetValueAsBool( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), false );
    const T& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FDC_SHOWN:           return field.IsVisible();
    case FDC_SHOW_NAME:       return field.IsNameShown();
    case FDC_ITALIC:          return field.IsItalic();
    case FDC_BOLD:            return field.IsBold();
    case FDC_ALLOW_AUTOPLACE: return field.CanAutoplace();
    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


template <class T>
void FIELDS_GRID_TABLE<T>::SetValue( int aRow, int aCol, const wxString &aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );
    T& field = this->at( (size_t) aRow );
    VECTOR2I pos;
    wxString value = aValue;

    switch( aCol )
    {
    case FDC_TEXT_SIZE:
    case FDC_POSX:
    case FDC_POSY:
        m_eval->SetDefaultUnits( m_frame->GetUserUnits() );

        if( m_eval->Process( value ) )
        {
            m_evalOriginal[ { aRow, aCol } ] = value;
            value = m_eval->Result();
        }

        break;

    default:
        break;
    }

    switch( aCol )
    {
    case FDC_NAME:
        field.SetName( value );
        break;

    case FDC_VALUE:
    {
        if( m_parentType == SCH_SHEET_T && aRow == SHEETFILENAME )
        {
            value = EnsureFileExtension( value, KiCadSchematicFileExtension );
        }
        else if( m_parentType == LIB_SYMBOL_T && aRow == VALUE_FIELD )
        {
            value = EscapeString( value, CTX_LIBID );
        }

        field.SetText( value );
        break;
    }

    case FDC_SHOWN:
        field.SetVisible( BoolFromString( value ) );
        break;

    case FDC_SHOW_NAME:
        field.SetNameShown( BoolFromString( value ) );
        break;

    case FDC_H_ALIGN:
        if( value == _( "Left" ) )
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( value == _( "Center" ) )
            field.SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        else if( value == _( "Right" ) )
            field.SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else
            wxFAIL_MSG( wxT( "unknown horizontal alignment: " ) + value );

        break;

    case FDC_V_ALIGN:
        if( value == _( "Top" ) )
            field.SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( value == _( "Center" ) )
            field.SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        else if( value == _( "Bottom" ) )
            field.SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else
            wxFAIL_MSG( wxT( "unknown vertical alignment: " ) + value);

        break;

    case FDC_ITALIC:
        field.SetItalic( BoolFromString( value ) );
        break;

    case FDC_BOLD:
        field.SetBold( BoolFromString( value ) );
        break;

    case FDC_TEXT_SIZE:
        field.SetTextSize( VECTOR2I( m_frame->ValueFromString( value ),
                                     m_frame->ValueFromString( value ) ) );
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

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
        break;
    }

    m_dialog->OnModify();

    GetView()->Refresh();
}


template <class T>
void FIELDS_GRID_TABLE<T>::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );
    T& field = this->at( (size_t) aRow );

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

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }

    m_dialog->OnModify();
}


// Explicit Instantiations

template class FIELDS_GRID_TABLE<SCH_FIELD>;
template class FIELDS_GRID_TABLE<LIB_FIELD>;


void FIELDS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    if( m_grid->GetGridCursorRow() == FOOTPRINT_FIELD && m_grid->GetGridCursorCol() == FDC_VALUE )
    {
        menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ),
                     _( "Browse for footprint" ) );
        menu.AppendSeparator();
    }
    else if( m_grid->GetGridCursorRow() == DATASHEET_FIELD && m_grid->GetGridCursorCol() == FDC_VALUE )
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
        wxString      fpid = m_grid->GetCellValue( FOOTPRINT_FIELD, FDC_VALUE );
        KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true, m_dlg );

        if( frame->ShowModal( &fpid, m_dlg ) )
            m_grid->SetCellValue( FOOTPRINT_FIELD, FDC_VALUE, fpid );

        frame->Destroy();
    }
    else if (event.GetId() == MYID_SHOW_DATASHEET )
    {
        wxString datasheet_uri = m_grid->GetCellValue( DATASHEET_FIELD, FDC_VALUE );
        GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), m_dlg->Prj().SchSearchS() );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


template <class T>
wxString FIELDS_GRID_TABLE<T>::StringFromBool( bool aValue ) const
{
    if( aValue )
        return wxT( "1" );
    else
        return wxT( "0" );
}


template <class T>
bool FIELDS_GRID_TABLE<T>::BoolFromString( wxString aValue ) const
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
