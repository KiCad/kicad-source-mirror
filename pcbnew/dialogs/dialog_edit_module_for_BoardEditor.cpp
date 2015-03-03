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

#include <class_module.h>
#include <class_text_mod.h>
#include <validators.h>

#include <dialog_edit_module_for_BoardEditor.h>
#include <wildcards_and_files_ext.h>

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

    InitModeditProperties();
    InitBoardProperties();

    m_NoteBook->SetSelection( m_page );

    m_sdbSizerStdButtonsOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}



DIALOG_MODULE_BOARD_EDITOR::~DIALOG_MODULE_BOARD_EDITOR()
{
    m_page = m_NoteBook->GetSelection();

    for( unsigned ii = 0; ii < m_Shapes3D_list.size(); ii++ )
        delete m_Shapes3D_list[ii];

    m_Shapes3D_list.clear();

    delete m_ReferenceCopy;
    delete m_ValueCopy;
    delete m_3D_Scale;
    delete m_3D_Offset;
    delete m_3D_Rotation;
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
    case UNDEFINED_CONNECTION:
        m_ZoneConnectionChoice->SetSelection( 0 );
        break;

    case PAD_IN_ZONE:
        m_ZoneConnectionChoice->SetSelection( 1 );
        break;

    case THERMAL_PAD:
        m_ZoneConnectionChoice->SetSelection( 2 );
        break;

    case PAD_NOT_IN_ZONE:
        m_ZoneConnectionChoice->SetSelection( 3 );
        break;
    }
}


void DIALOG_MODULE_BOARD_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    ENDQUASIMODAL( -1 );
}


void DIALOG_MODULE_BOARD_EDITOR::GotoModuleEditor( wxCommandEvent& event )
{
    if( m_CurrentModule->GetTimeStamp() == 0 )    // Module Editor needs a non null timestamp
    {
        m_CurrentModule->SetTimeStamp( GetNewTimeStamp() );
        m_Parent->OnModify();
    }

    ENDQUASIMODAL( 2 );
}


void DIALOG_MODULE_BOARD_EDITOR::ExchangeModule( wxCommandEvent& event )
{
    m_Parent->InstallExchangeModuleFrame( m_CurrentModule );

    // Warning: m_CurrentModule was deleted by exchange module
    m_Parent->SetCurItem( NULL );
    ENDQUASIMODAL( 0 );
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
    m_textCtrl3DDefaultPath->SetValue( default_path );

    m_LastSelected3DShapeIndex = -1;

    // Init 3D shape list
    S3D_MASTER* draw3D = m_CurrentModule->Models();

    while( draw3D )
    {
        if( !draw3D->GetShape3DName().IsEmpty() )
        {
            S3D_MASTER* draw3DCopy = new S3D_MASTER( NULL );
            draw3DCopy->Copy( draw3D );
            m_Shapes3D_list.push_back( draw3DCopy );
            m_3D_ShapeNameListBox->Append( draw3DCopy->GetShape3DName() );
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

    // Initialize 3D parameters
    m_3D_Scale = new S3DPOINT_VALUE_CTRL( m_Panel3D, m_bSizerShapeScale );
    m_3D_Offset = new S3DPOINT_VALUE_CTRL( m_Panel3D, m_bSizerShapeOffset );
    m_3D_Rotation = new S3DPOINT_VALUE_CTRL( m_Panel3D, m_bSizerShapeRotation );

    // if m_3D_ShapeNameListBox is not empty, preselect first 3D shape
    if( m_3D_ShapeNameListBox->GetCount() > 0 )
    {
        m_LastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );
        Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
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
    if( aStruct3DSource )
    {
        m_3D_Scale->SetValue( aStruct3DSource->m_MatScale );

        m_3D_Offset->SetValue( aStruct3DSource->m_MatPosition );

        m_3D_Rotation->SetValue( aStruct3DSource->m_MatRotation );
    }
    else
    {
        S3DPOINT dummy_vertex( 1.0, 1.0, 1.0 );
        m_3D_Scale->SetValue( dummy_vertex );
    }
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
    struct3DDest->m_MatScale    = m_3D_Scale->GetValue();
    struct3DDest->m_MatRotation = m_3D_Rotation->GetValue();
    struct3DDest->m_MatPosition = m_3D_Offset->GetValue();
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
        m_LastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );
        Transfert3DValuesToDisplay(
            m_Shapes3D_list[m_LastSelected3DShapeIndex] );
    }
}

void DIALOG_MODULE_BOARD_EDITOR::Edit3DShapeFileName()
{
    int idx = m_3D_ShapeNameListBox->GetSelection();

    if( idx < 0 )
        return;

    // Edit filename
    wxString filename = m_3D_ShapeNameListBox->GetStringSelection();

    wxTextEntryDialog dlg( this, wxEmptyString, wxEmptyString, filename );
    dlg.SetTextValidator( FILE_NAME_WITH_PATH_CHAR_VALIDATOR( &filename ) );

    if( dlg.ShowModal() != wxID_OK || filename.IsEmpty() )
        return;    //Aborted by user

    m_3D_ShapeNameListBox->SetString( idx, filename );
}


void DIALOG_MODULE_BOARD_EDITOR::BrowseAndAdd3DShapeFile()
{
    PROJECT&        prj = Prj();

    // here, the KISYS3DMOD default path for 3D shape files is expected
    // to be already defined (when starting Pcbnew, it is defined
    // from the user defined env variable, or set to a default value)
    wxFileName fn( wxGetenv( KISYS3DMOD ), wxEmptyString );
    wxString default3DPath = fn.GetPathWithSep();

    wxString initialpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );

    if( !initialpath )
        initialpath = default3DPath;

    wxString    fileFilters = wxGetTranslation( Shapes3DFileWildcard );

    fileFilters += wxChar( '|' );
    fileFilters += wxGetTranslation( IDF3DFileWildcard );

    wxString filename = EDA_FileSelector( _( "3D Shape:" ), initialpath,
                                wxEmptyString, wxEmptyString,
                                fileFilters, this, wxFD_OPEN, true );

    if( filename.IsEmpty() )
        return;

    fn = filename;

    prj.SetRString( PROJECT::VIEWER_3D_PATH, fn.GetPath() );

    /* If the file path is already in the 3D shape file default path
     * just add the file name relative to this path to the list.
     * Otherwise, add the file name with a full or relative path.
     * The relative path, when possible, is preferable
     * because it preserve use of default path, when the path is a sub path of this path
     */
    wxString rootpath = filename.SubString( 0, default3DPath.Length()-1 );
    bool useRelPath = rootpath.IsSameAs( default3DPath, wxFileName::IsCaseSensitive() );

    if( useRelPath )
        fn.MakeRelativeTo( default3DPath );
    else    // Absolute path given, not a subpath of the default path,
            // therefore ask if the user wants a relative (to the default path) one
    {
        wxString msg;
        msg.Printf( _( "Use a path relative to '%s'?" ), GetChars( default3DPath ) );
        int diag = wxMessageBox( msg, _( "Path type" ),
                                 wxYES_NO | wxICON_QUESTION, this );

        if( diag == wxYES )     // Make it relative to the default 3D path
            fn.MakeRelativeTo( default3DPath );
    }

    filename = fn.GetFullPath();

    S3D_MASTER* new3DShape = new S3D_MASTER( NULL );

#ifdef __WINDOWS__
    // In Kicad files, filenames and paths are stored using Unix notation
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    new3DShape->SetShape3DName( filename );
    m_Shapes3D_list.push_back( new3DShape );
    m_3D_ShapeNameListBox->Append( filename );

    if( m_LastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_LastSelected3DShapeIndex );

    m_LastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetCount() - 1;
    m_3D_ShapeNameListBox->SetSelection( m_LastSelected3DShapeIndex );
    Transfert3DValuesToDisplay( m_Shapes3D_list[m_LastSelected3DShapeIndex] );
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
        m_CurrentModule->SetZoneConnection( UNDEFINED_CONNECTION );
        break;

    case 1:
        m_CurrentModule->SetZoneConnection( PAD_IN_ZONE );
        break;

    case 2:
        m_CurrentModule->SetZoneConnection( THERMAL_PAD );
        break;

    case 3:
        m_CurrentModule->SetZoneConnection( PAD_NOT_IN_ZONE );
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

    ENDQUASIMODAL( 1 );

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

