/**
 * @file dialog_edit_module_for_Modedit.cpp
 *
 * @brief Dialog for editing a module properties in module editor (modedit)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <kiface_i.h>
#include <gestfich.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>
#include <base_units.h>
#include <macros.h>
#include <validators.h>
#include <kicad_string.h>
#include <board_commit.h>
#include <bitmaps.h>

#include <class_module.h>
#include <class_text_mod.h>
#include <module_editor_frame.h>
#include <dialog_edit_module_for_Modedit.h>
#include <wildcards_and_files_ext.h>
#include <pgm_base.h>
#include "3d_cache/dialogs/panel_prev_model.h"
#include "3d_cache/dialogs/3d_cache_dialogs.h"
#include "3d_cache/3d_cache.h"
#include "3d_cache/3d_filename_resolver.h"

size_t DIALOG_MODULE_MODULE_EDITOR::m_page = 0;     // remember the last open page during session


DIALOG_MODULE_MODULE_EDITOR::DIALOG_MODULE_MODULE_EDITOR( FOOTPRINT_EDIT_FRAME* aParent,
                                                          MODULE* aModule ) :
    DIALOG_MODULE_MODULE_EDITOR_BASE( aParent )
{
    m_parent = aParent;
    m_currentModule = aModule;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    aParent->Prj().Get3DCacheManager()->GetResolver()->SetProgramBase( &Pgm() );

    m_currentModuleCopy = new MODULE( *aModule );

    m_PreviewPane = new PANEL_PREV_3D( m_Panel3D,
                                       aParent->Prj().Get3DCacheManager(),
                                       m_currentModuleCopy,
                                       &m_shapes3D_list );

    bLowerSizer3D->Add( m_PreviewPane, 1, wxEXPAND, 5 );

    m_FootprintNameCtrl->SetValidator( FILE_NAME_CHAR_VALIDATOR() );
    initModeditProperties();

    m_NoteBook->SetSelection( m_page );

    m_sdbSizerStdButtonsOK->SetDefault();

    Layout();

}


DIALOG_MODULE_MODULE_EDITOR::~DIALOG_MODULE_MODULE_EDITOR()
{
    m_shapes3D_list.clear();

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    Prj().Get3DCacheManager()->FlushCache( false );

    // the GL canvas has to be visible before it is destroyed
    m_page = m_NoteBook->GetSelection();
    m_NoteBook->SetSelection( 1 );

    delete m_referenceCopy;
    m_referenceCopy = NULL;   // just in case, to avoid double-free

    delete m_valueCopy;
    m_valueCopy = NULL;

    delete m_PreviewPane;
    m_PreviewPane = NULL;   // just in case, to avoid double-free

    // this is already deleted by the board used on preview pane so
    // no need to delete here
    // delete m_currentModuleCopy;
    // m_currentModuleCopy = NULL;
}


void DIALOG_MODULE_MODULE_EDITOR::initModeditProperties()
{
    SetFocus();

    // Display the default path, given by environment variable KISYS3DMOD
    wxString default_path;
    wxGetEnv( KISYS3DMOD, &default_path );
#ifdef __WINDOWS__
    default_path.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    m_lastSelected3DShapeIndex = -1;

    // Init 3D shape list
    m_3D_ShapeNameListBox->Clear();
    std::list<S3D_INFO>::iterator sM = m_currentModule->Models().begin();
    std::list<S3D_INFO>::iterator eM = m_currentModule->Models().end();
    m_shapes3D_list.clear();


    wxString origPath;
    wxString alias;
    wxString shortPath;
    S3D_FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();

    while( sM != eM )
    {
        m_shapes3D_list.push_back( *sM );
        origPath = sM->m_Filename;

        if( res && res->SplitAlias( origPath, alias, shortPath ) )
        {
            origPath = alias;
            origPath.append( wxT( ":" ) );
            origPath.append( shortPath );
        }

        m_3D_ShapeNameListBox->Append( origPath );
        ++sM;
    }

    m_DocCtrl->SetValue( m_currentModule->GetDescription() );
    m_KeywordCtrl->SetValue( m_currentModule->GetKeywords() );
    m_referenceCopy = new TEXTE_MODULE( m_currentModule->Reference() );
    m_referenceCopy->SetParent( m_currentModule );
    m_valueCopy = new TEXTE_MODULE( m_currentModule->Value() );
    m_valueCopy->SetParent( m_currentModule );
    m_ReferenceCtrl->SetValue( m_referenceCopy->GetText() );
    m_ValueCtrl->SetValue( m_valueCopy->GetText() );

    m_currentFPID = m_currentModule->GetFPID();
    m_FootprintNameCtrl->SetValue( FROM_UTF8( m_currentFPID.GetLibItemName() ) );
    m_LibraryNicknameCtrl->SetValue( FROM_UTF8( m_currentFPID.GetLibNickname() ) );

    m_AttributsCtrl->SetItemToolTip( 0, _( "Use this attribute for most non SMD components" ) );
    m_AttributsCtrl->SetItemToolTip( 1,
                                    _( "Use this attribute for SMD components.\n"
                                       "Only components with this option are put in the footprint position list file" ) );
    m_AttributsCtrl->SetItemToolTip( 2,
                                    _( "Use this attribute for \"virtual\" components drawn on board\n"
                                       "like an edge connector (old ISA PC bus for instance)" ) );

    // Controls on right side of the dialog
    switch( m_currentModule->GetAttributes() & 255 )
    {
    case 0:
        m_AttributsCtrl->SetSelection( 0 );
        break;

    case MOD_CMS:
        m_AttributsCtrl->SetSelection( 1 );
        break;

    case MOD_VIRTUAL:
        m_AttributsCtrl->SetSelection( 2 );
        break;

    default:
        m_AttributsCtrl->SetSelection( 0 );
        break;
    }

    m_AutoPlaceCtrl->SetSelection( (m_currentModule->IsLocked()) ? 1 : 0 );
    m_AutoPlaceCtrl->SetItemToolTip( 0, _( "Enable hotkey move commands and Auto Placement" ) );
    m_AutoPlaceCtrl->SetItemToolTip( 1, _( "Disable hotkey move commands and Auto Placement" ) );

    m_CostRot90Ctrl->SetValue( m_currentModule->GetPlacementCost90() );
    m_CostRot180Ctrl->SetValue( m_currentModule->GetPlacementCost180() );

    // Initialize dialog relative to masks clearances
    m_NetClearanceUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_SolderMaskMarginUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    wxString  msg;
    PutValueInLocalUnits( *m_NetClearanceValueCtrl, m_currentModule->GetLocalClearance() );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl, m_currentModule->GetLocalSolderMaskMargin() );

    // These 2 parameters are usually < 0, so prepare entering a negative value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl, m_currentModule->GetLocalSolderPasteMargin() );

    if( m_currentModule->GetLocalSolderPasteMargin() == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) + m_SolderPasteMarginCtrl->GetValue() );

    if( m_currentModule->GetLocalSolderPasteMarginRatio() == 0.0 )
        msg.Printf( wxT( "-%f" ), m_currentModule->GetLocalSolderPasteMarginRatio() * 100.0 );
    else
        msg.Printf( wxT( "%f" ), m_currentModule->GetLocalSolderPasteMarginRatio() * 100.0 );

    m_SolderPasteMarginRatioCtrl->SetValue( msg );

    // Add solder paste margin ration in per cent
    // for the usual default value 0.0, display -0.0 (or -0,0 in some countries)
    msg.Printf( wxT( "%f" ), m_currentModule->GetLocalSolderPasteMarginRatio() * 100.0 );

    if( m_currentModule->GetLocalSolderPasteMarginRatio() == 0.0 &&
        msg[0] == '0')  // Sometimes Printf adds a sign if the value is very small (0.0)
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    // if m_3D_ShapeNameListBox is not empty, preselect first 3D shape
    if( m_3D_ShapeNameListBox->GetCount() > 0 )
    {
        m_lastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection( m_lastSelected3DShapeIndex );
        if( m_PreviewPane )
            m_PreviewPane->SetModelDataIdx( m_lastSelected3DShapeIndex, true );
    }
    else
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData( true );
    }

    // We have modified the UI, so call Fit() for m_Panel3D
    // to be sure the m_Panel3D sizers are initialized before opening the dialog
    m_Panel3D->GetSizer()->Fit( m_Panel3D );
}


void DIALOG_MODULE_MODULE_EDITOR::On3DShapeNameSelected(wxCommandEvent& event)
{
    m_lastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetSelection();

    if( m_lastSelected3DShapeIndex < 0 )    // happens under wxGTK when deleting an item in m_3D_ShapeNameListBox wxListBox
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData();
        return;
    }

    if( m_lastSelected3DShapeIndex >= (int)m_shapes3D_list.size() )
    {
        wxMessageBox( wxT( "On3DShapeNameSelected() error" ) );
        m_lastSelected3DShapeIndex = -1;

        if( m_PreviewPane )
            m_PreviewPane->ResetModelData();

        return;
    }

    m_PreviewPane->SetModelDataIdx( m_lastSelected3DShapeIndex );
}


void DIALOG_MODULE_MODULE_EDITOR::Remove3DShape(wxCommandEvent& event)
{
    int ii = m_3D_ShapeNameListBox->GetSelection();

    if( ii < 0 )
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData( true );

        return;
    }

    m_shapes3D_list.erase( m_shapes3D_list.begin() + ii );
    m_3D_ShapeNameListBox->Delete( ii );

    if( m_3D_ShapeNameListBox->GetCount() > 0 )
    {
        if( ii > 0 )
            m_lastSelected3DShapeIndex = ii - 1;
        else
            m_lastSelected3DShapeIndex = 0;

        m_3D_ShapeNameListBox->SetSelection( m_lastSelected3DShapeIndex );

        if( m_PreviewPane )
            m_PreviewPane->SetModelDataIdx( m_lastSelected3DShapeIndex, true );
    }
    else
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData( true );
    }

    return;
}


void DIALOG_MODULE_MODULE_EDITOR::Edit3DShapeFileName()
{
    int idx = m_3D_ShapeNameListBox->GetSelection();

    if( idx < 0 )
        return;

    // Edit filename
    wxString filename = m_3D_ShapeNameListBox->GetStringSelection();
    wxTextEntryDialog dlg( this, wxEmptyString, wxEmptyString, filename );

    bool hasAlias;
    S3D_FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();

    if( dlg.ShowModal() != wxID_OK )
        return;

    filename = dlg.GetValue();

    if( filename.empty() )
        return;

    if( !res->ValidateFileName( filename, hasAlias ) )
    {
        wxString msg = _( "Invalid filename: " );
        msg.append( filename );
        wxMessageBox( msg, _( "Edit 3D file name" ) );

        return;
    }

    m_3D_ShapeNameListBox->SetString( idx, filename );

    // if the user has specified an alias in the name then prepend ':'
    if( hasAlias )
        filename.insert( 0, wxT( ":" ) );

    #ifdef __WINDOWS__
    // In Kicad files, filenames and paths are stored using Unix notation
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
    #endif

    m_shapes3D_list[idx].m_Filename = filename;

    // This assumes that the index didn't change and will just update the filename
    if( m_PreviewPane )
        m_PreviewPane->UpdateModelName( filename );

    return;
}


void DIALOG_MODULE_MODULE_EDITOR::BrowseAndAdd3DShapeFile()
{
    PROJECT&        prj = Prj();
    S3D_INFO model;

    wxString initialpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );
    wxString sidx = prj.GetRString( PROJECT::VIEWER_3D_FILTER_INDEX );
    int filter = 0;

    if( !sidx.empty() )
    {
        long tmp;
        sidx.ToLong( &tmp );

        if( tmp > 0 && tmp <= 0x7FFFFFFF )
            filter = (int) tmp;
    }

    if( !S3D::Select3DModel( m_PreviewPane, Prj().Get3DCacheManager(),
        initialpath, filter, &model ) || model.m_Filename.empty() )
    {
        return;
    }

    prj.SetRString( PROJECT::VIEWER_3D_PATH, initialpath );
    sidx = wxString::Format( wxT( "%i" ), filter );
    prj.SetRString( PROJECT::VIEWER_3D_FILTER_INDEX, sidx );
    wxString origPath = model.m_Filename;
    wxString alias;
    wxString shortPath;
    S3D_FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();

    if( res && res->SplitAlias( origPath, alias, shortPath ) )
    {
        origPath = alias;
        origPath.append( wxT( ":" ) );
        origPath.append( shortPath );
    }

    m_3D_ShapeNameListBox->Append( origPath );

    #ifdef __WINDOWS__
    // In Kicad files, filenames and paths are stored using Unix notation
    model.m_Filename.Replace( wxT( "\\" ), wxT( "/" ) );
    #endif

    m_shapes3D_list.push_back( model );
    m_lastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetCount() - 1;
    m_3D_ShapeNameListBox->SetSelection( m_lastSelected3DShapeIndex );

    if( m_PreviewPane )
        m_PreviewPane->SetModelDataIdx( m_lastSelected3DShapeIndex, true );

    return;
}


bool DIALOG_MODULE_MODULE_EDITOR::TransferDataFromWindow()
{
    wxString msg;
    // First, test for invalid chars in module name
    wxString footprintName = m_FootprintNameCtrl->GetValue();

    if( ! footprintName.IsEmpty() )
    {
        if( ! MODULE::IsLibNameValid( footprintName ) )
        {
            msg.Printf( _( "Error:\n"
                           "one of invalid chars <%s> found\nin <%s>" ),
                        MODULE::StringLibNameInvalidChars( true ),
                        GetChars( footprintName ) );

            DisplayError( NULL, msg );

            return false;
        }
    }

    if( !m_PreviewPane->ValidateWithMessage( msg ) )
    {
        DisplayError( NULL, msg );
        return false;
    }

    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_currentModule );

    m_currentModule->SetLocked( m_AutoPlaceCtrl->GetSelection() == 1 );

    switch( m_AttributsCtrl->GetSelection() )
    {
    case 0:
        m_currentModule->SetAttributes( 0 );
        break;

    case 1:
        m_currentModule->SetAttributes( MOD_CMS );
        break;

    case 2:
        m_currentModule->SetAttributes( MOD_VIRTUAL );
        break;
    }

    m_currentModule->SetPlacementCost90( m_CostRot90Ctrl->GetValue() );
    m_currentModule->SetPlacementCost180( m_CostRot180Ctrl->GetValue() );
    m_currentModule->SetDescription( m_DocCtrl->GetValue() );
    m_currentModule->SetKeywords( m_KeywordCtrl->GetValue() );

    // Init footprint name in library
    if( ! footprintName.IsEmpty() )
    {
        m_currentFPID.SetLibItemName( footprintName, false );
        m_currentModule->SetFPID( m_currentFPID );
    }

    // Init Fields:
    TEXTE_MODULE& reference = m_currentModule->Reference();
    reference = *m_referenceCopy;
    TEXTE_MODULE& value = m_currentModule->Value();
    value = *m_valueCopy;

    // Initialize masks clearances
    m_currentModule->SetLocalClearance( ValueFromTextCtrl( *m_NetClearanceValueCtrl ) );
    m_currentModule->SetLocalSolderMaskMargin( ValueFromTextCtrl( *m_SolderMaskMarginCtrl ) );
    m_currentModule->SetLocalSolderPasteMargin( ValueFromTextCtrl( *m_SolderPasteMarginCtrl ) );
    double   dtmp;
    msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A  -50% margin ratio means no paste on a pad, the ratio must be >= -50 %
    if( dtmp < -50.0 )
        dtmp = -50.0;

    // A margin ratio is always <= 0
    if( dtmp > 0.0 )
        dtmp = 0.0;

    m_currentModule->SetLocalSolderPasteMarginRatio( dtmp / 100 );

    std::list<S3D_INFO>* draw3D  = &m_currentModule->Models();
    draw3D->clear();
    draw3D->insert( draw3D->end(), m_shapes3D_list.begin(), m_shapes3D_list.end() );

    m_currentModule->CalculateBoundingBox();

    commit.Push( _( "Modify module properties" ) );

    return true;
}


void DIALOG_MODULE_MODULE_EDITOR::OnEditReference(wxCommandEvent& event)
{
    wxPoint tmp = m_parent->GetCrossHairPosition();
    m_parent->SetCrossHairPosition( m_referenceCopy->GetTextPos() );
    m_parent->InstallTextModOptionsFrame( m_referenceCopy, NULL );
    m_parent->SetCrossHairPosition( tmp );
    m_ReferenceCtrl->SetValue( m_referenceCopy->GetText() );
}


void DIALOG_MODULE_MODULE_EDITOR::OnEditValue(wxCommandEvent& event)
{
    wxPoint tmp = m_parent->GetCrossHairPosition();
    m_parent->SetCrossHairPosition( m_valueCopy->GetTextPos() );
    m_parent->InstallTextModOptionsFrame( m_valueCopy, NULL );
    m_parent->SetCrossHairPosition( tmp );
    m_ValueCtrl->SetValue( m_valueCopy->GetText() );
}


void DIALOG_MODULE_MODULE_EDITOR::Cfg3DPath( wxCommandEvent& event )
{
    if( S3D::Configure3DPaths( this, Prj().Get3DCacheManager()->GetResolver() ) )
        if( m_lastSelected3DShapeIndex >= 0 )
            if( m_PreviewPane )
                m_PreviewPane->SetModelDataIdx( m_lastSelected3DShapeIndex, true );
}
