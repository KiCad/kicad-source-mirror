/**
 * Module editor: Dialog for editing module properties in the pcb editor.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <3d_struct.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>
#include <base_units.h>
#include <project.h>

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


DIALOG_MODULE_BOARD_EDITOR::DIALOG_MODULE_BOARD_EDITOR( PCB_EDIT_FRAME*  aParent,
                                                        MODULE*          aModule,
                                                        wxDC*            aDC ) :
    DIALOG_MODULE_BOARD_EDITOR_BASE( aParent )
{
    m_Parent = aParent;
    m_DC     = aDC;
    m_CurrentModule = aModule;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    m_PreviewPane = new PANEL_PREV_3D( m_Panel3D, aParent->Prj().Get3DCacheManager() );
    bLowerSizer3D->Add( m_PreviewPane, 1, wxEXPAND, 5 );

    InitModeditProperties();
    InitBoardProperties();

    m_NoteBook->SetSelection( m_page );
    m_sdbSizerStdButtonsOK->SetDefault();

    GetSizer()->SetSizeHints( this );
    Centre();
    Layout();
}



DIALOG_MODULE_BOARD_EDITOR::~DIALOG_MODULE_BOARD_EDITOR()
{
    m_page = m_NoteBook->GetSelection();

    for( unsigned ii = 0; ii < m_Shapes3D_list.size(); ii++ )
        delete m_Shapes3D_list[ii];

    m_Shapes3D_list.clear();

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    Prj().Get3DCacheManager()->FlushCache( false );

    delete m_ReferenceCopy;
    delete m_ValueCopy;
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

    bool select = false;
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
        select = true;
        break;
    }

    wxString msg;
    msg << m_CurrentModule->GetOrientation();
    m_OrientValue->SetValue( msg );
    m_OrientValue->Enable( select );

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


void DIALOG_MODULE_BOARD_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    EndModal( PRM_EDITOR_ABORT );
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
    switch( m_OrientCtrl->GetSelection() )
    {
    case 0:
        m_OrientValue->Enable( false );
        m_OrientValue->SetValue( wxT( "0" ) );
        break;

    case 1:
        m_OrientValue->Enable( false );
        m_OrientValue->SetValue( wxT( "900" ) );
        break;

    case 2:
        m_OrientValue->Enable( false );
        m_OrientValue->SetValue( wxT( "2700" ) );
        break;

    case 3:
        m_OrientValue->Enable( false );
        m_OrientValue->SetValue( wxT( "1800" ) );
        break;

    default:
        m_OrientValue->Enable( true );
        break;
    }
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
    S3D_MASTER* draw3D = m_CurrentModule->Models();
    wxString origPath;
    wxString alias;
    wxString shortPath;
    S3D_FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();

    while( draw3D )
    {
        if( !draw3D->GetShape3DName().IsEmpty() )
        {
            S3D_MASTER* draw3DCopy = new S3D_MASTER( NULL );
            draw3DCopy->Copy( draw3D );
            m_Shapes3D_list.push_back( draw3DCopy );
            origPath = draw3DCopy->GetShape3DName();

            if( res && res->SplitAlias( origPath, alias, shortPath ) )
            {
                origPath = alias;
                origPath.append( wxT( ":" ) );
                origPath.append( shortPath );
            }

            m_3D_ShapeNameListBox->Append( origPath );
        }

        draw3D = (S3D_MASTER*) draw3D->Next();
    }

    m_ReferenceCopy = new TEXTE_MODULE( NULL );
    m_ValueCopy     = new TEXTE_MODULE( NULL );
    m_ReferenceCopy->Copy( &m_CurrentModule->Reference() );
    m_ValueCopy->Copy( &m_CurrentModule->Value() );
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
           "(like a old ISA PC bus connector)" ) );

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
        Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
    }
    else
    {
        S3D_INFO params;
        params.scale.x = 1.0;
        params.scale.y = 1.0;
        params.scale.z = 1.0;
        m_PreviewPane->SetModelData( &params );
    }

    // We have modified the UI, so call Fit() for m_Panel3D
    // to be sure the m_Panel3D sizers are initiliazed before opening the dialog
    m_Panel3D->GetSizer()->Fit( m_Panel3D );
}


/* Initialize 3D info displayed in dialog box from values in aStruct3DSource
 */
void DIALOG_MODULE_BOARD_EDITOR::Transfert3DValuesToDisplay(
    S3D_MASTER* aStruct3DSource )
{
    S3D_INFO params;

    if( aStruct3DSource )
    {
        params.filename = aStruct3DSource->GetShape3DName();
        params.scale.x = aStruct3DSource->m_MatScale.x;
        params.scale.y = aStruct3DSource->m_MatScale.y;
        params.scale.z = aStruct3DSource->m_MatScale.z;

        params.offset.x = aStruct3DSource->m_MatPosition.x;
        params.offset.y = aStruct3DSource->m_MatPosition.y;
        params.offset.z = aStruct3DSource->m_MatPosition.z;

        params.rotation.x = aStruct3DSource->m_MatRotation.x;
        params.rotation.y = aStruct3DSource->m_MatRotation.y;
        params.rotation.z = aStruct3DSource->m_MatRotation.z;
    }
    else
    {
        params.scale.x = 1.0;
        params.scale.y = 1.0;
        params.scale.z = 1.0;

        params.offset.x = 0.0;
        params.offset.y = 0.0;
        params.offset.z = 0.0;

        params.rotation = params.offset;
    }

    m_PreviewPane->SetModelData( &params );
    return;
}


/** Copy 3D info displayed in dialog box to values in a item in m_Shapes3D_list
 * @param aIndexSelection = item index in m_Shapes3D_list
 */
void DIALOG_MODULE_BOARD_EDITOR::TransfertDisplayTo3DValues(
    int aIndexSelection  )
{
    if( aIndexSelection >= (int) m_Shapes3D_list.size() )
        return;

    S3D_MASTER* struct3DDest = m_Shapes3D_list[aIndexSelection];
    S3D_INFO params;
    m_PreviewPane->GetModelData( &params );

    struct3DDest->m_MatScale.x = params.scale.x;
    struct3DDest->m_MatScale.y = params.scale.y;
    struct3DDest->m_MatScale.z = params.scale.z;

    struct3DDest->m_MatRotation.x = params.rotation.x;
    struct3DDest->m_MatRotation.y = params.rotation.y;
    struct3DDest->m_MatRotation.z = params.rotation.z;

    struct3DDest->m_MatPosition.x = params.offset.x;
    struct3DDest->m_MatPosition.y = params.offset.y;
    struct3DDest->m_MatPosition.z = params.offset.z;

    return;
}


void DIALOG_MODULE_BOARD_EDITOR::On3DShapeNameSelected( wxCommandEvent& event )
{
    if( m_LastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_LastSelected3DShapeIndex );
    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetSelection();

    if( m_LastSelected3DShapeIndex < 0 )    // happens under wxGTK when
                                            // deleting an item in
                                            // m_3D_ShapeNameListBox wxListBox
        return;

    if( m_LastSelected3DShapeIndex >= (int) m_Shapes3D_list.size() )
    {
        wxMessageBox( wxT( "On3DShapeNameSelected() error" ) );
        m_LastSelected3DShapeIndex = -1;
        return;
    }

    Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
}



void DIALOG_MODULE_BOARD_EDITOR::Remove3DShape( wxCommandEvent& event )
{
    if( m_LastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_LastSelected3DShapeIndex );

    int ii = m_3D_ShapeNameListBox->GetSelection();
    if( ii < 0 )
        return;

    m_Shapes3D_list.erase( m_Shapes3D_list.begin() + ii );
    m_3D_ShapeNameListBox->Delete( ii );

    if( m_3D_ShapeNameListBox->GetCount() == 0 )
        Transfert3DValuesToDisplay( NULL );
    else
    {
        if( ii > 0 )
            m_LastSelected3DShapeIndex = ii - 1;
        else
            m_LastSelected3DShapeIndex = 0;

        m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );
        Transfert3DValuesToDisplay(
            m_Shapes3D_list[m_LastSelected3DShapeIndex] );
    }

    return;
}


void DIALOG_MODULE_BOARD_EDITOR::Edit3DShapeFileName()
{
    int idx = m_3D_ShapeNameListBox->GetSelection();

    if( idx < 0 )
        return;

    // ensure any updated parameters are not discarded
    TransfertDisplayTo3DValues( idx );

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
        wxMessageBox( msg, _T( "Edit 3D file name" ) );

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

    S3D_MASTER* new3DShape = new S3D_MASTER( NULL );
    new3DShape->SetShape3DName( filename );
    new3DShape->m_MatPosition = m_Shapes3D_list[idx]->m_MatPosition;
    new3DShape->m_MatRotation = m_Shapes3D_list[idx]->m_MatRotation;
    new3DShape->m_MatScale = m_Shapes3D_list[idx]->m_MatScale;
    delete m_Shapes3D_list[idx];
    m_Shapes3D_list[idx] = new3DShape;

    Transfert3DValuesToDisplay( m_Shapes3D_list[idx] );

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

        if( tmp > 0 && tmp <= 0x7FFFFFFF )
            filter = (int) tmp;
    }

    if( !S3D::Select3DModel( this, Prj().Get3DCacheManager(),
        initialpath, filter, &model ) || model.filename.empty() )
    {
        return;
    }

    prj.SetRString( PROJECT::VIEWER_3D_PATH, initialpath );
    sidx = wxString::Format( wxT( "%i" ), filter );
    prj.SetRString( PROJECT::VIEWER_3D_FILTER_INDEX, sidx );
    wxString origPath = model.filename;
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
    model.filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    S3D_MASTER* new3DShape = new S3D_MASTER( NULL );
    new3DShape->SetShape3DName( model.filename );
    new3DShape->m_MatScale.x = model.scale.x;
    new3DShape->m_MatScale.y = model.scale.y;
    new3DShape->m_MatScale.z = model.scale.z;
    new3DShape->m_MatRotation.x = model.rotation.x;
    new3DShape->m_MatRotation.y = model.rotation.y;
    new3DShape->m_MatRotation.z = model.rotation.z;
    new3DShape->m_MatPosition.x = model.offset.x;
    new3DShape->m_MatPosition.y = model.offset.y;
    new3DShape->m_MatPosition.z = model.offset.z;

    m_Shapes3D_list.push_back( new3DShape );
    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetCount() - 1;
    m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );
    Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );

    return;
}


void DIALOG_MODULE_BOARD_EDITOR::OnOkClick( wxCommandEvent& event )
{
    wxPoint  modpos;
    wxString msg;

    if( m_CurrentModule->GetFlags() == 0 )    // this is a simple edition, we
                                              // must create an undo entry
        m_Parent->SaveCopyInUndoList( m_CurrentModule, UR_CHANGED );

    if( m_DC )
    {
        m_Parent->GetCanvas()->CrossHairOff( m_DC );
        m_CurrentModule->Draw( m_Parent->GetCanvas(), m_DC, GR_XOR );
    }

    // Init Fields (should be first, because they can be moved or/and flipped later):
    m_CurrentModule->Reference().Copy( m_ReferenceCopy );
    m_CurrentModule->Value().Copy( m_ValueCopy );

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
    long orient = 0;
    msg = m_OrientValue->GetValue();
    msg.ToLong( &orient );

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

    // Update 3D shape list
    int ii = m_3D_ShapeNameListBox->GetSelection();

    if( ii >= 0 )
        TransfertDisplayTo3DValues( ii  );

    S3D_MASTER* draw3D = m_CurrentModule->Models();

    for( unsigned ii = 0; ii < m_Shapes3D_list.size(); ii++ )
    {
        S3D_MASTER* draw3DCopy = m_Shapes3D_list[ii];

        if( draw3DCopy->GetShape3DName().IsEmpty() )
            continue;

        if( draw3D == NULL )
        {
            draw3D = new S3D_MASTER( draw3D );
            m_CurrentModule->Models().Append( draw3D );
        }

        draw3D->SetShape3DName( draw3DCopy->GetShape3DName() );
        draw3D->m_MatScale    = draw3DCopy->m_MatScale;
        draw3D->m_MatRotation = draw3DCopy->m_MatRotation;
        draw3D->m_MatPosition = draw3DCopy->m_MatPosition;

        draw3D = draw3D->Next();
    }

    // Remove old extra 3D shapes
    S3D_MASTER* nextdraw3D;

    for( ; draw3D != NULL; draw3D = nextdraw3D )
    {
        nextdraw3D = (S3D_MASTER*) draw3D->Next();
        delete m_CurrentModule->Models().Remove( draw3D );
    }

    // Fill shape list with one void entry, if no entry
    if( m_CurrentModule->Models() == NULL )
        m_CurrentModule->Models().PushBack( new S3D_MASTER( m_CurrentModule ) );


    m_CurrentModule->CalculateBoundingBox();

    m_Parent->OnModify();

    EndModal( PRM_EDITOR_EDIT_OK );

    if( m_DC )
    {
        m_CurrentModule->Draw( m_Parent->GetCanvas(), m_DC, GR_OR );
        m_Parent->GetCanvas()->CrossHairOn( m_DC );
    }
}


void DIALOG_MODULE_BOARD_EDITOR::OnEditReference( wxCommandEvent& event )
{
    wxPoint tmp = m_Parent->GetCrossHairPosition();

    m_Parent->SetCrossHairPosition( m_ReferenceCopy->GetTextPosition() );
    m_ReferenceCopy->SetParent( m_CurrentModule );
    m_Parent->InstallTextModOptionsFrame( m_ReferenceCopy, NULL );
    m_Parent->SetCrossHairPosition( tmp );
    m_ReferenceCtrl->SetValue( m_ReferenceCopy->GetText() );
}


void DIALOG_MODULE_BOARD_EDITOR::OnEditValue( wxCommandEvent& event )
{
    wxPoint tmp = m_Parent->GetCrossHairPosition();

    m_Parent->SetCrossHairPosition( m_ValueCopy->GetTextPosition() );
    m_ValueCopy->SetParent( m_CurrentModule );
    m_Parent->InstallTextModOptionsFrame( m_ValueCopy, NULL );
    m_Parent->SetCrossHairPosition( tmp );
    m_ValueCtrl->SetValue( m_ValueCopy->GetText() );
}


void DIALOG_MODULE_BOARD_EDITOR::Cfg3DPath( wxCommandEvent& event )
{
    S3D::Configure3DPaths( this, Prj().Get3DCacheManager()->GetResolver() );
}
