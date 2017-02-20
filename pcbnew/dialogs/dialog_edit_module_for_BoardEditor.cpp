/**
 * Module editor: Dialog for editing module properties in the pcb editor.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pgm_base.h>
#include <gestfich.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>
#include <base_units.h>
#include <project.h>
#include <board_commit.h>
#include <bitmaps.h>

#include <class_module.h>
#include <class_text_mod.h>
#include <validators.h>

#include <dialog_edit_module_for_BoardEditor.h>
#include <wildcards_and_files_ext.h>
#include "3d_cache/dialogs/panel_prev_model.h"
#include "3d_cache/dialogs/3d_cache_dialogs.h"
#include "3d_cache/3d_cache.h"
#include "3d_cache/3d_filename_resolver.h"

size_t DIALOG_MODULE_BOARD_EDITOR::m_page = 0;     // remember the last open page during session

wxBEGIN_EVENT_TABLE( DIALOG_MODULE_BOARD_EDITOR, wxDialog )
    EVT_CLOSE( DIALOG_MODULE_BOARD_EDITOR::OnCloseWindow )
wxEND_EVENT_TABLE()

DIALOG_MODULE_BOARD_EDITOR::DIALOG_MODULE_BOARD_EDITOR( PCB_EDIT_FRAME*  aParent,
                                                        MODULE*          aModule,
                                                        wxDC*            aDC ) :
    DIALOG_MODULE_BOARD_EDITOR_BASE( aParent ),
    m_OrientValidator( 1, &m_OrientValue )
{
    wxASSERT( aParent != NULL );
    wxASSERT( aModule != NULL );

    m_Parent = aParent;
    m_DC     = aDC;
    m_CurrentModule = aModule;

    m_currentModuleCopy = new MODULE( *aModule );

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    m_OrientValidator.SetRange( -360.0, 360.0 );
    m_OrientValueCtrl->SetValidator( m_OrientValidator );
    m_OrientValidator.SetWindow( m_OrientValueCtrl );

    aParent->Prj().Get3DCacheManager()->GetResolver()->SetProgramBase( &Pgm() );

    m_PreviewPane = new PANEL_PREV_3D( m_Panel3D,
                                       aParent->Prj().Get3DCacheManager(),
                                       m_currentModuleCopy,
                                       &m_shapes3D_list );

    bLowerSizer3D->Add( m_PreviewPane, 1, wxEXPAND, 5 );

    m_NoteBook->SetSelection( m_page );
    m_sdbSizerStdButtonsOK->SetDefault();

    m_ReferenceCopy = NULL;
    m_ValueCopy = NULL;
    m_LastSelected3DShapeIndex = 0;
    m_OrientValue = 0;

    Layout();

}



DIALOG_MODULE_BOARD_EDITOR::~DIALOG_MODULE_BOARD_EDITOR()
{
    m_shapes3D_list.clear();

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    Prj().Get3DCacheManager()->FlushCache( false );

    // the GL canvas has to be visible before it is destroyed
    m_page = m_NoteBook->GetSelection();
    m_NoteBook->SetSelection( 1 );

    delete m_ReferenceCopy;
    m_ReferenceCopy = NULL;

    delete m_ValueCopy;
    m_ValueCopy = NULL;

    delete m_PreviewPane;
    m_PreviewPane = NULL;   // just in case, to avoid double-free

    // this is already deleted by the board used on preview pane so
    // no need to delete here
    // delete m_currentModuleCopy;
    // m_currentModuleCopy = NULL;
}


void DIALOG_MODULE_BOARD_EDITOR::OnCloseWindow( wxCloseEvent &event )
{
    m_PreviewPane->Close();

    event.Skip();
}


// Creation of the panel properties of the module editor.
void DIALOG_MODULE_BOARD_EDITOR::InitBoardProperties()
{
    PutValueInLocalUnits( *m_ModPositionX, m_CurrentModule->GetPosition().x );
    m_XPosUnit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    PutValueInLocalUnits( *m_ModPositionY, m_CurrentModule->GetPosition().y );
    m_YPosUnit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    m_LayerCtrl->SetSelection(
         (m_CurrentModule->GetLayer() == B_Cu) ? 1 : 0 );

    bool custom_orientation = false;
    switch( int( m_CurrentModule->GetOrientation() ) )
    {
    case 0:
        m_OrientCtrl->SetSelection( 0 );
        break;

    case 900:
    case -2700:
        m_OrientCtrl->SetSelection( 1 );
        break;

    case -900:
    case 2700:
        m_OrientCtrl->SetSelection( 2 );
        break;

    case -1800:
    case 1800:
        m_OrientCtrl->SetSelection( 3 );
        break;

    default:
        m_OrientCtrl->SetSelection( 4 );
        custom_orientation = true;
        break;
    }

    m_OrientValueCtrl->Enable( custom_orientation );
    m_OrientValue = m_CurrentModule->GetOrientation() / 10.0;
    m_OrientValidator.TransferToWindow();

    // Initialize dialog relative to masks clearances
    m_NetClearanceUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_SolderMaskMarginUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    PutValueInLocalUnits( *m_NetClearanceValueCtrl, m_CurrentModule->GetLocalClearance() );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl, m_CurrentModule->GetLocalSolderMaskMargin() );

    // These 2 parameters are usually < 0, so prepare entering a negative
    // value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl, m_CurrentModule->GetLocalSolderPasteMargin() );

    if( m_CurrentModule->GetLocalSolderPasteMargin() == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) +
                                           m_SolderPasteMarginCtrl->GetValue() );

    // Add solder paste margin ration in per cent
    // for the usual default value 0.0, display -0.0 (or -0,0 in some countries)
    wxString msg;
    msg.Printf( wxT( "%f" ),
                    m_CurrentModule->GetLocalSolderPasteMarginRatio() * 100.0 );

    if( m_CurrentModule->GetLocalSolderPasteMarginRatio() == 0.0 &&
        msg[0] == '0')  // Sometimes Printf adds a sign if the value is very small (0.0)
        m_SolderPasteMarginRatioCtrl->SetValue( wxT("-") + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    switch( m_CurrentModule->GetZoneConnection() )
    {
    default:
    case PAD_ZONE_CONN_INHERITED:
        m_ZoneConnectionChoice->SetSelection( 0 );
        break;

    case PAD_ZONE_CONN_FULL:
        m_ZoneConnectionChoice->SetSelection( 1 );
        break;

    case PAD_ZONE_CONN_THERMAL:
        m_ZoneConnectionChoice->SetSelection( 2 );
        break;

    case PAD_ZONE_CONN_NONE:
        m_ZoneConnectionChoice->SetSelection( 3 );
        break;
    }
}


void DIALOG_MODULE_BOARD_EDITOR::GotoModuleEditor( wxCommandEvent& event )
{
    if( m_CurrentModule->GetTimeStamp() == 0 )    // Module Editor needs a non null timestamp
    {
        m_CurrentModule->SetTimeStamp( GetNewTimeStamp() );
        m_Parent->OnModify();
    }

    EndModal( PRM_EDITOR_WANT_MODEDIT );
}


void DIALOG_MODULE_BOARD_EDITOR::ExchangeModule( wxCommandEvent& event )
{
    EndModal( PRM_EDITOR_WANT_EXCHANGE_FP );
}


void DIALOG_MODULE_BOARD_EDITOR::ModuleOrientEvent( wxCommandEvent& event )
{
    bool custom_orientation = false;

    switch( m_OrientCtrl->GetSelection() )
    {
    case 0:
        m_OrientValue = 0.0;
        break;

    case 1:
        m_OrientValue = 90.0;
        break;

    case 2:
        m_OrientValue = 270.0;
        break;

    case 3:
        m_OrientValue = 180.0;
        break;

    default:
        custom_orientation = true;
        break;
    }

    m_OrientValidator.TransferToWindow();
    m_OrientValueCtrl->Enable( custom_orientation );
}


void DIALOG_MODULE_BOARD_EDITOR::InitModeditProperties()
{
    wxString default_path;
    wxGetEnv( KISYS3DMOD, &default_path );
#ifdef __WINDOWS__
    default_path.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    m_LastSelected3DShapeIndex = -1;

    // Init 3D shape list
    m_3D_ShapeNameListBox->Clear();
    std::list<S3D_INFO>::iterator sM = m_CurrentModule->Models().begin();
    std::list<S3D_INFO>::iterator eM = m_CurrentModule->Models().end();
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

    m_ReferenceCopy = new TEXTE_MODULE( m_CurrentModule->Reference() );
    m_ReferenceCopy->SetParent( m_CurrentModule );
    m_ValueCopy = new TEXTE_MODULE( m_CurrentModule->Value() );
    m_ValueCopy->SetParent( m_CurrentModule );
    m_ReferenceCtrl->SetValue( m_ReferenceCopy->GetText() );
    m_ValueCtrl->SetValue( m_ValueCopy->GetText() );

    // Shows the footprint's schematic path.
    m_textCtrlSheetPath->SetValue( m_CurrentModule->GetPath() );

    m_AttributsCtrl->SetItemToolTip( 0,
        _( "Use this attribute for most non SMD components\n"
            "Components with this option are not put in the footprint position list file" ) );
    m_AttributsCtrl->SetItemToolTip( 1,
         _( "Use this attribute for SMD components.\n"
            "Only components with this option are put in the footprint position list file" ) );
    m_AttributsCtrl->SetItemToolTip( 2,
        _( "Use this attribute for \"virtual\" components drawn on board\n"
           "like an edge connector (old ISA PC bus for instance)" ) );

    // Controls on right side of the dialog
    switch( m_CurrentModule->GetAttributes() & 255 )
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

    if( m_CurrentModule->IsLocked() )
        m_AutoPlaceCtrl->SetSelection( 2 );
    else if( m_CurrentModule->PadsLocked() )
        m_AutoPlaceCtrl->SetSelection( 1 );
    else
        m_AutoPlaceCtrl->SetSelection( 0 );

    m_AutoPlaceCtrl->SetItemToolTip( 0,
                                    _( "Component can be freely moved and auto placed. User can arbitrarily select and edit component's pads." ) );
    m_AutoPlaceCtrl->SetItemToolTip( 1,
                                    _( "Component can be freely moved and auto placed, but its pads cannot be selected or edited." ) );
    m_AutoPlaceCtrl->SetItemToolTip( 2,
                                    _( "Component is locked: it cannot be freely moved or auto placed." ) );

    m_CostRot90Ctrl->SetValue( m_CurrentModule->GetPlacementCost90() );

    m_CostRot180Ctrl->SetValue( m_CurrentModule->GetPlacementCost180() );

    // if m_3D_ShapeNameListBox is not empty, preselect first 3D shape
    if( m_3D_ShapeNameListBox->GetCount() > 0 )
    {
        m_LastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );

        if( m_PreviewPane )
            m_PreviewPane->SetModelDataIdx( m_LastSelected3DShapeIndex, true );
    }
    else
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData( true );
    }

    // We have modified the UI, so call Fit() for m_Panel3D
    // to be sure the m_Panel3D sizers are initiliazed before opening the dialog
    m_Panel3D->GetSizer()->Fit( m_Panel3D );
}


void DIALOG_MODULE_BOARD_EDITOR::On3DShapeNameSelected( wxCommandEvent& event )
{
    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetSelection();

    if( m_LastSelected3DShapeIndex < 0 )    // happens under wxGTK when
                                            // deleting an item in
                                            // m_3D_ShapeNameListBox wxListBox
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData();

        return;
    }

    if( m_LastSelected3DShapeIndex >= (int) m_shapes3D_list.size() )
    {
        wxMessageBox( wxT( "On3DShapeNameSelected() error" ) );
        m_LastSelected3DShapeIndex = -1;

        if( m_PreviewPane )
            m_PreviewPane->ResetModelData();

        return;
    }

    if( m_PreviewPane )
        m_PreviewPane->SetModelDataIdx( m_LastSelected3DShapeIndex );
}



void DIALOG_MODULE_BOARD_EDITOR::Remove3DShape( wxCommandEvent& event )
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
            m_LastSelected3DShapeIndex = ii - 1;
        else
            m_LastSelected3DShapeIndex = 0;

        m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );

        if( m_PreviewPane )
            m_PreviewPane->SetModelDataIdx( m_LastSelected3DShapeIndex, true );
    }
    else
    {
        if( m_PreviewPane )
            m_PreviewPane->ResetModelData( true );
    }

    return;
}


void DIALOG_MODULE_BOARD_EDITOR::Edit3DShapeFileName()
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


void DIALOG_MODULE_BOARD_EDITOR::BrowseAndAdd3DShapeFile()
{
    PROJECT& prj = Prj();
    S3D_INFO model;

    wxString initialpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );
    wxString sidx = prj.GetRString( PROJECT::VIEWER_3D_FILTER_INDEX );
    int filter = 0;

    if( !sidx.empty() )
    {
        long tmp;
        sidx.ToLong( &tmp );

        if( tmp > 0 && tmp <= INT_MAX )
            filter = (int) tmp;
    }

    if( !S3D::Select3DModel( this, Prj().Get3DCacheManager(),
        initialpath, filter, &model ) || model.m_Filename.empty() )
    {
        return;
    }

    prj.SetRString( PROJECT::VIEWER_3D_PATH, initialpath );
    sidx = wxString::Format( wxT( "%i" ), filter );
    prj.SetRString( PROJECT::VIEWER_3D_FILTER_INDEX, sidx );
    S3D_FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();
    wxString alias;
    wxString shortPath;
    wxString filename = model.m_Filename;

    if( res && res->SplitAlias( filename, alias, shortPath ) )
    {
        alias.Append( wxT( ":" ) );
        alias.Append( shortPath );
        m_3D_ShapeNameListBox->Append( alias );
    }
    else
    {
        m_3D_ShapeNameListBox->Append( filename );
    }

#ifdef __WINDOWS__
    // In Kicad files, filenames and paths are stored using Unix notation
    model.m_Filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    m_shapes3D_list.push_back( model );
    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetCount() - 1;
    m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );

    if( m_PreviewPane )
        m_PreviewPane->SetModelDataIdx( m_LastSelected3DShapeIndex, true ) ;

    return;
}


bool DIALOG_MODULE_BOARD_EDITOR::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() ||
        !m_PanelProperties->TransferDataToWindow() )
    {
        wxMessageBox( _( "Error: invalid footprint parameter" ) );
        return false;
    }

    if( !m_Panel3D->TransferDataToWindow() )
    {
        wxMessageBox( _( "Error: invalid 3D parameter" ) );
        return false;
    }

    InitModeditProperties();
    InitBoardProperties();

    return true;
}


bool DIALOG_MODULE_BOARD_EDITOR::TransferDataFromWindow()
{
    wxPoint  modpos;
    wxString msg;

    BOARD_COMMIT commit( m_Parent );
    commit.Modify( m_CurrentModule );

    if( !Validate() || !DIALOG_MODULE_BOARD_EDITOR_BASE::TransferDataFromWindow() ||
        !m_PanelProperties->TransferDataFromWindow() )
    {
        wxMessageBox( _( "Error: invalid or missing footprint parameter" ) );
        return false;
    }

    if( !m_Panel3D->TransferDataFromWindow() )
    {
        wxMessageBox( _( "Error: invalid or missing 3D parameter" ) );
        return false;
    }

    if( m_DC )
    {
        m_Parent->GetCanvas()->CrossHairOff( m_DC );
        m_CurrentModule->Draw( m_Parent->GetCanvas(), m_DC, GR_XOR );
    }

    // Init Fields (should be first, because they can be moved or/and flipped later):
    TEXTE_MODULE& reference = m_CurrentModule->Reference();
    reference = *m_ReferenceCopy;
    TEXTE_MODULE& value = m_CurrentModule->Value();
    value = *m_ValueCopy;

    // Initialize masks clearances
    m_CurrentModule->SetLocalClearance( ValueFromTextCtrl( *m_NetClearanceValueCtrl ) );
    m_CurrentModule->SetLocalSolderMaskMargin( ValueFromTextCtrl( *m_SolderMaskMarginCtrl ) );
    m_CurrentModule->SetLocalSolderPasteMargin( ValueFromTextCtrl( *m_SolderPasteMarginCtrl ) );

    double dtmp = 0.0;
    msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A -50% margin ratio means no paste on a pad, the ratio must be >= -50%
    if( dtmp < -50.0 )
        dtmp = -50.0;
    // A margin ratio is always <= 0
    // 0 means use full pad copper area
    if( dtmp > 0.0 )
        dtmp = 0.0;

    m_CurrentModule->SetLocalSolderPasteMarginRatio( dtmp / 100 );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0:
        m_CurrentModule->SetZoneConnection( PAD_ZONE_CONN_INHERITED );
        break;

    case 1:
        m_CurrentModule->SetZoneConnection( PAD_ZONE_CONN_FULL );
        break;

    case 2:
        m_CurrentModule->SetZoneConnection( PAD_ZONE_CONN_THERMAL );
        break;

    case 3:
        m_CurrentModule->SetZoneConnection( PAD_ZONE_CONN_NONE );
        break;
    }

    // Set Module Position
    modpos.x = ValueFromTextCtrl( *m_ModPositionX );
    modpos.y = ValueFromTextCtrl( *m_ModPositionY );
    m_CurrentModule->SetPosition( modpos );
    m_CurrentModule->SetLocked( m_AutoPlaceCtrl->GetSelection() == 2 );
    m_CurrentModule->SetPadsLocked( m_AutoPlaceCtrl->GetSelection() == 1 );

    switch( m_AttributsCtrl->GetSelection() )
    {
    case 0:
        m_CurrentModule->SetAttributes( 0 );
        break;

    case 1:
        m_CurrentModule->SetAttributes( MOD_CMS );
        break;

    case 2:
        m_CurrentModule->SetAttributes( MOD_VIRTUAL );
        break;
    }

    m_CurrentModule->SetPlacementCost90( m_CostRot90Ctrl->GetValue() );
    m_CurrentModule->SetPlacementCost180( m_CostRot180Ctrl->GetValue() );

    /* Now, set orientation. must be made after others changes,
     * because rotation changes fields positions on board according to the new orientation
     * (relative positions are not modified)
     */
    int orient = KiROUND( m_OrientValue * 10.0 );

    if( m_CurrentModule->GetOrientation() != orient )
        m_CurrentModule->Rotate( m_CurrentModule->GetPosition(),
                                 orient - m_CurrentModule->GetOrientation() );

    // Set component side, that also have effect on the fields positions on board
    bool change_layer = false;
    if( m_LayerCtrl->GetSelection() == 0 )     // layer req = COMPONENT
    {
        if( m_CurrentModule->GetLayer() == B_Cu )
            change_layer = true;
    }
    else if( m_CurrentModule->GetLayer() == F_Cu )
        change_layer = true;

    if( change_layer )
        m_CurrentModule->Flip( m_CurrentModule->GetPosition() );

    // This will update the S3D_INFO list into the current module
    msg.Clear();

    if( !m_PreviewPane->ValidateWithMessage( msg ) )
    {
        DisplayError( this, msg );
        return false;
    }

    std::list<S3D_INFO>* draw3D = &m_CurrentModule->Models();
    draw3D->clear();
    draw3D->insert( draw3D->end(), m_shapes3D_list.begin(), m_shapes3D_list.end() );

    m_CurrentModule->CalculateBoundingBox();

    // This is a simple edition, we must create an undo entry
    if( m_CurrentModule->GetFlags() == 0 )
        commit.Push( _( "Modify module properties" ) );

    SetReturnCode( PRM_EDITOR_EDIT_OK );

    if( m_DC )
    {
        m_CurrentModule->Draw( m_Parent->GetCanvas(), m_DC, GR_OR );
        m_Parent->GetCanvas()->CrossHairOn( m_DC );
    }

    return true;
}


void DIALOG_MODULE_BOARD_EDITOR::OnEditReference( wxCommandEvent& event )
{
    wxPoint tmp = m_Parent->GetCrossHairPosition();

    m_Parent->SetCrossHairPosition( m_ReferenceCopy->GetTextPos() );
    m_ReferenceCopy->SetParent( m_CurrentModule );
    m_Parent->InstallTextModOptionsFrame( m_ReferenceCopy, NULL );
    m_Parent->SetCrossHairPosition( tmp );
    m_ReferenceCtrl->SetValue( m_ReferenceCopy->GetText() );
}


void DIALOG_MODULE_BOARD_EDITOR::OnEditValue( wxCommandEvent& event )
{
    wxPoint tmp = m_Parent->GetCrossHairPosition();

    m_Parent->SetCrossHairPosition( m_ValueCopy->GetTextPos() );
    m_ValueCopy->SetParent( m_CurrentModule );
    m_Parent->InstallTextModOptionsFrame( m_ValueCopy, NULL );
    m_Parent->SetCrossHairPosition( tmp );
    m_ValueCtrl->SetValue( m_ValueCopy->GetText() );
}


void DIALOG_MODULE_BOARD_EDITOR::Cfg3DPath( wxCommandEvent& event )
{
    if( S3D::Configure3DPaths( this, Prj().Get3DCacheManager()->GetResolver() ) )
        if( m_LastSelected3DShapeIndex >= 0 )
            if( m_PreviewPane )
                m_PreviewPane->SetModelDataIdx( m_LastSelected3DShapeIndex, true );
}
