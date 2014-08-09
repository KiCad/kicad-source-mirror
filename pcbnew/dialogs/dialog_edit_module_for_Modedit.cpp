/**
 * @file dialog_edit_module_for_Modedit.cpp
 *
 * @brief Dialog for editing a module properties in module editor (modedit)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2014 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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
#include <3d_struct.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>
#include <base_units.h>
#include <macros.h>
#include <validators.h>
#include <kicad_string.h>

#include <class_module.h>
#include <class_text_mod.h>
#include <module_editor_frame.h>
#include <dialog_edit_module_for_Modedit.h>
#include <wildcards_and_files_ext.h>


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

    m_FootprintNameCtrl->SetValidator( FOOTPRINT_NAME_VALIDATOR() );
    initModeditProperties();
    m_sdbSizerStdButtonsOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_MODULE_MODULE_EDITOR::~DIALOG_MODULE_MODULE_EDITOR()
{
    for( unsigned ii = 0; ii < m_shapes3D_list.size(); ii++ )
        delete m_shapes3D_list[ii];

    m_shapes3D_list.clear();

    delete m_referenceCopy;
    delete m_valueCopy;
    delete m_3D_Scale;
    delete m_3D_Offset;
    delete m_3D_Rotation;
}


void DIALOG_MODULE_MODULE_EDITOR::initModeditProperties()
{
    SetFocus();

    // Display the default path, given by environment variable KISYS3DMOD
    wxString default_path;
    wxGetEnv( wxT( KISYS3DMOD ), &default_path );
#ifdef __WINDOWS__
    default_path.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
    m_textCtrl3DDefaultPath->SetValue( default_path );

    m_lastSelected3DShapeIndex = -1;

    // Init 3D shape list
    S3D_MASTER* draw3D = m_currentModule->Models();

    while( draw3D )
    {
        if( !draw3D->GetShape3DName().IsEmpty() )
        {
            S3D_MASTER* draw3DCopy = new S3D_MASTER(NULL);
            draw3DCopy->Copy( draw3D );
            m_shapes3D_list.push_back( draw3DCopy );
            m_3D_ShapeNameListBox->Append( draw3DCopy->GetShape3DName() );
        }
        draw3D = (S3D_MASTER*) draw3D->Next();
    }

    m_DocCtrl->SetValue( m_currentModule->GetDescription() );
    m_KeywordCtrl->SetValue( m_currentModule->GetKeywords() );
    m_referenceCopy = new TEXTE_MODULE( NULL );
    m_valueCopy = new TEXTE_MODULE( NULL );
    m_referenceCopy->Copy( &m_currentModule->Reference() );
    m_valueCopy->Copy( &m_currentModule->Value() );
    m_ReferenceCtrl->SetValue( m_referenceCopy->GetText() );
    m_ValueCtrl->SetValue( m_valueCopy->GetText() );
    m_ValueCtrl->SetValue( m_valueCopy->GetText() );
    m_FootprintNameCtrl->SetValue( m_currentModule->GetFPID().Format() );

    m_AttributsCtrl->SetItemToolTip( 0, _( "Use this attribute for most non SMD components" ) );
    m_AttributsCtrl->SetItemToolTip( 1,
                                    _( "Use this attribute for SMD components.\nOnly components with this option are put in the footprint position list file" ) );
    m_AttributsCtrl->SetItemToolTip( 2,
                                    _( "Use this attribute for \"virtual\" components drawn on board (like a old ISA PC bus connector)" ) );

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

    // Initialize 3D parameters
    m_3D_Scale = new S3DPOINT_VALUE_CTRL( m_Panel3D, m_bSizerShapeScale );
    m_3D_Offset = new S3DPOINT_VALUE_CTRL( m_Panel3D, m_bSizerShapeOffset );
    m_3D_Rotation = new S3DPOINT_VALUE_CTRL( m_Panel3D, m_bSizerShapeRotation );

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
        Transfert3DValuesToDisplay( m_shapes3D_list[m_lastSelected3DShapeIndex] );
    }

    // We have modified the UI, so call Fit() for m_Panel3D
    // to be sure the m_Panel3D sizers are initialized before opening the dialog
    m_Panel3D->GetSizer()->Fit( m_Panel3D );
}


// Initialize 3D info displayed in dialog box from values in aStruct3DSource
void DIALOG_MODULE_MODULE_EDITOR::Transfert3DValuesToDisplay( S3D_MASTER * aStruct3DSource )
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


/** Copy 3D info displayed in dialog box to values in a item in m_shapes3D_list
 * @param aIndexSelection = item index in m_shapes3D_list
 */
void DIALOG_MODULE_MODULE_EDITOR::TransfertDisplayTo3DValues( int aIndexSelection  )
{
    if( aIndexSelection >= (int)m_shapes3D_list.size() )
        return;

    S3D_MASTER * struct3DDest = m_shapes3D_list[aIndexSelection];
    struct3DDest->m_MatScale    = m_3D_Scale->GetValue();
    struct3DDest->m_MatRotation = m_3D_Rotation->GetValue();
    struct3DDest->m_MatPosition = m_3D_Offset->GetValue();
}


void DIALOG_MODULE_MODULE_EDITOR::On3DShapeNameSelected(wxCommandEvent& event)
{
    if( m_lastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_lastSelected3DShapeIndex );

    m_lastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetSelection();

    if( m_lastSelected3DShapeIndex < 0 )    // happens under wxGTK when deleting an item in m_3D_ShapeNameListBox wxListBox
       return;

    if( m_lastSelected3DShapeIndex >= (int)m_shapes3D_list.size() )
    {
        wxMessageBox( wxT( "On3DShapeNameSelected() error" ) );
        m_lastSelected3DShapeIndex = -1;
        return;
    }

    Transfert3DValuesToDisplay( m_shapes3D_list[m_lastSelected3DShapeIndex] );
}


void DIALOG_MODULE_MODULE_EDITOR::Remove3DShape(wxCommandEvent& event)
{
    if( m_lastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_lastSelected3DShapeIndex );

    int ii = m_3D_ShapeNameListBox->GetSelection();

    if( ii < 0 )
        return;

    m_shapes3D_list.erase( m_shapes3D_list.begin() + ii );
    m_3D_ShapeNameListBox->Delete( ii );

    if( m_3D_ShapeNameListBox->GetCount() == 0)
        Transfert3DValuesToDisplay( NULL );
    else
    {
        m_lastSelected3DShapeIndex = 0;
        m_3D_ShapeNameListBox->SetSelection( m_lastSelected3DShapeIndex );
        Transfert3DValuesToDisplay( m_shapes3D_list[m_lastSelected3DShapeIndex] );
    }
}


void DIALOG_MODULE_MODULE_EDITOR::BrowseAndAdd3DLib( wxCommandEvent& event )
{
    PROJECT&        prj = Prj();
    SEARCH_STACK&   search = Kiface().KifaceSearch();

    wxString    fullpath;
    wxString    kisys3dmod = wxGetenv( wxT( KISYS3DMOD ) );

    if( !kisys3dmod || !wxFileName::IsDirReadable( kisys3dmod ) )
    {
        fullpath = search.FindValidPath( LIB3D_PATH );
    }

    if( !fullpath )
    {
        fullpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );
        if( !fullpath )
            fullpath = search.LastVisitedPath( LIB3D_PATH );
    }

#ifdef __WINDOWS__
    fullpath.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    wxString fullfilename, shortfilename;
    wxString fileFilters = wxGetTranslation( Shapes3DFileWildcard );

    fileFilters += wxChar(  '|' );
    fileFilters += wxGetTranslation( IDF3DFileWildcard );

    fullfilename = EDA_FileSelector( _( "3D Shape:" ),
                                     fullpath,
                                     wxEmptyString,
                                     wxEmptyString,
                                     wxGetTranslation( fileFilters ),
                                     this,
                                     wxFD_OPEN,
                                     true
                                     );

    if( fullfilename.IsEmpty() )
        return;

    wxFileName fn = fullfilename;

    prj.SetRString( PROJECT::VIEWER_3D_PATH, fn.GetPath() );

    /* If the file path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of these default paths
     */
    shortfilename = search.FilenameWithRelativePathInSearchList( fullfilename );

    wxFileName aux = shortfilename;

    if( aux.IsAbsolute() )
    {   // Absolute path, ask if the user wants a relative one
        int diag = wxMessageBox(
            _( "Use a relative path?" ),
            _( "Path type" ),
            wxYES_NO | wxICON_QUESTION, this );

        if( diag == wxYES )
        {   // Make it relative
            aux.MakeRelativeTo( wxT( "." ) );
            shortfilename = aux.GetPathWithSep() + aux.GetFullName();
        }
    }

    S3D_MASTER* new3DShape = new S3D_MASTER(NULL);

#ifdef __WINDOWS__
    // Store filename in Unix notation
    shortfilename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    new3DShape->SetShape3DName( shortfilename );
    m_shapes3D_list.push_back( new3DShape );
    m_3D_ShapeNameListBox->Append( shortfilename );

    if( m_lastSelected3DShapeIndex >= 0 )
        TransfertDisplayTo3DValues( m_lastSelected3DShapeIndex );

    m_lastSelected3DShapeIndex = m_3D_ShapeNameListBox->GetCount() - 1;
    m_3D_ShapeNameListBox->SetSelection( m_lastSelected3DShapeIndex );
    Transfert3DValuesToDisplay( m_shapes3D_list[m_lastSelected3DShapeIndex] );
}


void DIALOG_MODULE_MODULE_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


void DIALOG_MODULE_MODULE_EDITOR::OnOkClick( wxCommandEvent& event )
{
    // First, test for invalid chars in module name
    wxString footprintName = m_FootprintNameCtrl->GetValue();

    if( ! footprintName.IsEmpty() )
    {
        if( ! MODULE::IsLibNameValid( footprintName ) )
        {
            wxString msg;
            msg.Printf( _( "Error:\none of invalid chars <%s> found\nin <%s>" ),
                        MODULE::StringLibNameInvalidChars( true ),
                        GetChars( footprintName ) );

            DisplayError( NULL, msg );
                return;
        }
    }

    m_parent->SaveCopyInUndoList( m_currentModule, UR_MODEDIT );
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
        m_currentModule->SetFPID( FPID( footprintName ) );

    // Init Fields:
    m_currentModule->Reference().Copy( m_referenceCopy );
    m_currentModule->Value().Copy( m_valueCopy );

    // Initialize masks clearances
    m_currentModule->SetLocalClearance( ValueFromTextCtrl( *m_NetClearanceValueCtrl ) );
    m_currentModule->SetLocalSolderMaskMargin( ValueFromTextCtrl( *m_SolderMaskMarginCtrl ) );
    m_currentModule->SetLocalSolderPasteMargin( ValueFromTextCtrl( *m_SolderPasteMarginCtrl ) );
    double   dtmp;
    wxString msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A  -50% margin ratio means no paste on a pad, the ratio must be >= -50 %
    if( dtmp < -50.0 )
        dtmp = -50.0;

    // A margin ratio is always <= 0
    if( dtmp > 0.0 )
        dtmp = 0.0;

    m_currentModule->SetLocalSolderPasteMarginRatio( dtmp / 100 );

    // Update 3D shape list
    int ii = m_3D_ShapeNameListBox->GetSelection();

    if ( ii >= 0 )
        TransfertDisplayTo3DValues( ii );

    S3D_MASTER*   draw3D  = m_currentModule->Models();

    for( unsigned ii = 0; ii < m_shapes3D_list.size(); ii++ )
    {
        S3D_MASTER*   draw3DCopy = m_shapes3D_list[ii];

        if( draw3DCopy->GetShape3DName().IsEmpty() )
            continue;

        if( draw3D == NULL )
        {
            draw3D = new S3D_MASTER( draw3D );
            m_currentModule->Models().Append( draw3D );
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
        delete m_currentModule->Models().Remove( draw3D );
    }

    // Fill shape list with one void entry, if no entry
    if( m_currentModule->Models() == NULL )
        m_currentModule->Models().PushBack( new S3D_MASTER( m_currentModule ) );


    m_currentModule->CalculateBoundingBox();

    m_parent->OnModify();

    EndModal( 1 );
}


void DIALOG_MODULE_MODULE_EDITOR::OnEditReference(wxCommandEvent& event)
{
    wxPoint tmp = m_parent->GetCrossHairPosition();
    m_parent->SetCrossHairPosition( m_referenceCopy->GetTextPosition() );
    m_parent->InstallTextModOptionsFrame( m_referenceCopy, NULL );
    m_parent->SetCrossHairPosition( tmp );
    m_ReferenceCtrl->SetValue( m_referenceCopy->GetText() );
}


void DIALOG_MODULE_MODULE_EDITOR::OnEditValue(wxCommandEvent& event)
{
    wxPoint tmp = m_parent->GetCrossHairPosition();
    m_parent->SetCrossHairPosition( m_valueCopy->GetTextPosition() );
    m_parent->InstallTextModOptionsFrame( m_valueCopy, NULL );
    m_parent->SetCrossHairPosition( tmp );
    m_ValueCtrl->SetValue( m_valueCopy->GetText() );
}
