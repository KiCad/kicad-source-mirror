/***************/
/* treeprj.cpp */
/***************/

#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "kicad.h"

#include "wx/image.h"
#include "wx/imaglist.h"
#include "wx/treectrl.h"
#include "wx/regex.h"
#include "wx/dir.h"

#include "bitmaps.h"

#include "id.h"


/********************************************/
/* Methodes pour l'arbre  gestion du projet */
/********************************************/

IMPLEMENT_ABSTRACT_CLASS( WinEDA_TreePrj, wxTreeCtrl )

WinEDA_TreePrj::WinEDA_TreePrj( WinEDA_PrjFrame* parent ) :
    wxTreeCtrl( parent, ID_PROJECT_TREE,
                wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS, wxDefaultValidator,
                wxT( "EDATreeCtrl" ) )
{
    m_Parent = parent;

    // Make an image list containing small icons
    m_ImageList = new wxImageList( 16, 16, TRUE, TREE_MAX );

    m_ImageList->Add( wxBitmap( kicad_icon_small_xpm ) );       // TREE_PROJECT
    m_ImageList->Add( wxBitmap( eeschema_xpm ) );               // TREE_SCHEMA
    m_ImageList->Add( wxBitmap( pcbnew_xpm ) );                 // TREE_PCB
    m_ImageList->Add( wxBitmap( icon_python_small_xpm ) );      // TREE_PY
    m_ImageList->Add( wxBitmap( icon_gerbview_small_xpm ) );    // TREE_GERBER
    m_ImageList->Add( wxBitmap( datasheet_xpm ) );              // TREE_PDF
    m_ImageList->Add( wxBitmap( icon_txt_xpm ) );               // TREE_TXT
    m_ImageList->Add( wxBitmap( icon_cvpcb_small_xpm ) );       // TREE_NET
    m_ImageList->Add( wxBitmap( unknown_xpm ) );                // TREE_UNKNOWN
    m_ImageList->Add( wxBitmap( directory_xpm ) );              // TREE_DIRECTORY

    SetImageList( m_ImageList );
}


WinEDA_TreePrj::~WinEDA_TreePrj()
{
    if( m_ImageList )
        delete m_ImageList;
}


/***************************************************************************************/
int WinEDA_TreePrj::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
/***************************************************************************************/

/* sort function for tree items.
 *  items are sorted :
 *  directory names first by alphabetic order
 *  root file names after
 *  file names last by alphabetic order
 */
{
    TreePrjItemData* myitem1 = (TreePrjItemData*) GetItemData( item1 );
    TreePrjItemData* myitem2 = (TreePrjItemData*) GetItemData( item2 );

    if( (myitem1->m_Type == TREE_DIRECTORY) && ( myitem2->m_Type != TREE_DIRECTORY ) )
        return -1;
    if( (myitem2->m_Type == TREE_DIRECTORY) && ( myitem1->m_Type != TREE_DIRECTORY ) )
        return 1;

    if( myitem1->m_IsRootFile  && !myitem2->m_IsRootFile )
        return -1;
    if( myitem2->m_IsRootFile && !myitem1->m_IsRootFile )
        return 1;

    return myitem1->m_FileName.CmpNoCase( myitem2->m_FileName );
}


/****************************************************************************************************/
TreePrjItemData::TreePrjItemData( enum TreeFileType type, const wxString& data,
                                  wxTreeCtrl* parent ) :
    wxTreeItemData()
/****************************************************************************************************/
{
    m_Type       = type;
    m_Parent     = parent;
    m_FileName   = data;
    m_IsRootFile = false;
}


#ifdef KICAD_PYTHON
using namespace boost::python;

/**************************************/
object TreePrjItemData::GetIdPy() const
/**************************************/

// Convert the data to an id
{
    wxTreeItemId* id = new wxTreeItemId();

    *id = GetId();
    return object( handle<>( borrowed( wxPyConstructObject( id, wxT( "wxTreeItemId" ), true ) ) ) );
}


#endif

/*******************************************/
void TreePrjItemData::SetState( int state )
/*******************************************/

// Set the state used in the icon list
{
    wxImageList* imglist = m_Parent->GetImageList();

    if( !imglist || state < 0 || state >= imglist->GetImageCount() / ( TREE_MAX - 2 ) )
        return;
    m_State = state;
    int imgid = m_Type - 1 + state * ( TREE_MAX - 1 );
    m_Parent->SetItemImage( GetId(), imgid );
    m_Parent->SetItemImage( GetId(), imgid, wxTreeItemIcon_Selected );
}


/*******************************************/
wxString TreePrjItemData::GetDir() const
/*******************************************/
/* Get the directory containing the file */
{
    if( TREE_DIRECTORY == m_Type )
        return m_FileName;

    wxFileName filename = wxFileName( m_FileName );

    filename.MakeRelativeTo( wxGetCwd() );

    wxArrayString dirs = filename.GetDirs();

    wxString      dir;
    for( unsigned int i = 0; i < dirs.Count(); i++ )
    {
        dir += dirs[i] + filename.GetPathSeparator();
    }

    return dir;
}


/****************************************************************/
void TreePrjItemData::OnRename( wxTreeEvent& event, bool check )
/****************************************************************/
/* Called upon tree item rename */
{
    //this segfaults on linux (in wxEvtHandler::ProcessEvent), wx version 2.8.7
    //therefore, until it is fixed, we must cancel the rename.
    event.Veto();
    return;
    if( !Rename( event.GetLabel(), check ) )
        event.Veto();
}


/****************************************************/
void TreePrjItemData::Move( TreePrjItemData* dest )
/****************************************************/

// Move the object to dest
{
    //function not safe.
    return;
    const wxString sep = wxFileName().GetPathSeparator();

    if( m_Type == TREE_DIRECTORY )
        return;
    if( !dest )
        return;
    if( m_Parent != dest->m_Parent )
        return; // Can not cross move!
    if( dest == this )
        return; // Can not move to ourself...

    wxTreeItemId parent = m_Parent->GetItemParent( GetId() );
    if( dest == dynamic_cast<TreePrjItemData*>( m_Parent->GetItemData( parent ) ) )
        return; // same parent ?

    // We need to create a new item from us, and move
    // data to there ...

    // First move file on the disk
    wxFileName fname( m_FileName );

    wxString destName;
    if( !dest->GetDir().IsEmpty() )
        destName = dest->GetDir() + sep;
    destName += fname.GetFullName();

    if( destName == GetFileName() )
        return; // Same place ??

    // Move the file on the disk:
#if ( ( wxMAJOR_VERSION < 2) || ( ( wxMAJOR_VERSION == 2)&& (wxMINOR_VERSION < 7 )  ) )
    if( !wxRenameFile( GetFileName(), destName ) )
#else
    if( !wxRenameFile( GetFileName(), destName, false ) )
#endif
    {
        wxMessageDialog( m_Parent, _( "Unable to move file ... " ),
                         _( "Permission error ?" ), wxICON_ERROR | wxOK );
        return;
    }

#ifdef KICAD_PYTHON
    object param = make_tuple( PyHandler::Convert( m_FileName ),
                              PyHandler::Convert( destName ) );
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::MoveFile" ), param );
#endif

    SetFileName( destName );

    if( TREE_DIRECTORY != GetType() )
    {
        // Move the tree item itself now:
        wxTreeItemId oldId = GetId();
        int          i    = m_Parent->GetItemImage( oldId );
        wxString     text = m_Parent->GetItemText( oldId );

        // Bye bye old Id :'(
        wxTreeItemId newId = m_Parent->AppendItem( dest->GetId(), text, i );
        m_Parent->SetItemData( newId, this );
        m_Parent->SetItemData( oldId, NULL );
        m_Parent->Delete( oldId );
    }
    else
    {
        // We should move recursively all files, but that's quite boring
        // let's just refresh that's all ... TODO (change this to a better code ...)
        wxCommandEvent dummy;
        dynamic_cast<WinEDA_TreePrj*>( m_Parent )->GetParent()->m_Parent->OnRefresh( dummy );
    }
}


/****************************************************************/
bool TreePrjItemData::Rename( const wxString& name, bool check )
/****************************************************************/
/* rename the file checking if extension change occurs */
{
    //this is broken & unsafe to use on linux.
    if( m_Type == TREE_DIRECTORY )
        return false;

    if( name.IsEmpty() )
        return false;

    const wxString sep = wxFileName().GetPathSeparator();
    wxString       newFile;
    wxString       dirs = GetDir();

    if( !dirs.IsEmpty() && GetType() != TREE_DIRECTORY )
        newFile = dirs + sep + name;
    else
        newFile = name;

    if( newFile == m_FileName )
        return false;

    wxString ext = WinEDA_PrjFrame::GetFileExt( GetType() );

    wxRegEx             reg( wxT ( "^.*\\" ) + ext + wxT( "$" ), wxRE_ICASE );

    if( check && !ext.IsEmpty() && !reg.Matches( newFile ) )
    {
        wxMessageDialog dialog( m_Parent,
          _( "Changing file extension will change file type.\n Do you want to continue ?" ),
          _( "Rename File" ),
          wxYES_NO | wxICON_QUESTION );

        if( wxID_YES != dialog.ShowModal() )
            return false;
    }

#if ( ( wxMAJOR_VERSION < 2) || ( ( wxMAJOR_VERSION == 2) && (wxMINOR_VERSION < 7 )  ) )
    if( !wxRenameFile( m_FileName, newFile ) )
#else
    if( !wxRenameFile( m_FileName, newFile, false ) )
#endif
    {
        wxMessageDialog( m_Parent, _( "Unable to rename file ... " ),
                         _( "Permission error ?" ), wxICON_ERROR | wxOK );
        return false;
    }
    SetFileName( newFile );

#ifdef KICAD_PYTHON
    object param = make_tuple( PyHandler::Convert( m_FileName ),
                              PyHandler::Convert( newFile ) );
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::RenameFile" ), param );
#endif
    return true;
}


/*******************************************/
bool TreePrjItemData::Delete( bool check )
/*******************************************/
/* delete a file */
{
    wxMessageDialog dialog( m_Parent, _ ("Do you really want to delete ") + GetFileName(),
                            _( "Delete File" ), wxYES_NO | wxICON_QUESTION );

    if( !check || wxID_YES == dialog.ShowModal() )
    {
        if( !wxDirExists( m_FileName ) )
        {
            wxRemoveFile( m_FileName );
        }
        else
        {
            wxArrayString filelist;

            wxDir::GetAllFiles( m_FileName, &filelist );

            for( unsigned int i = 0; i < filelist.Count(); i++ )
                wxRemoveFile( filelist[i] );

            wxRmdir( m_FileName );
        }
        m_Parent->Delete( GetId() );

#ifdef KICAD_PYTHON
        PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::DeleteFile" ),
                                               PyHandler::Convert( m_FileName ) );
#endif
        return true;
    }
    return false;
}


/**********************************/
void TreePrjItemData::Activate( WinEDA_PrjFrame* prjframe )
/**********************************/
/* Called under item activation */
{
    wxString     sep = wxFileName().GetPathSeparator();
    wxString     FullFileName = GetFileName();
    wxDir*       dir;
    wxString     dir_filename;
    wxTreeItemId id = GetId();
    int          count;

    switch( GetType() )
    {
    case TREE_PROJECT:
        break;

    case TREE_DIRECTORY:
        if( prjframe )
        {
            dir = new wxDir( FullFileName );

            count = 0;
            if( dir && dir->IsOpened() && dir->GetFirst( &dir_filename ) )
            {
                do
                {
                    wxString fil = FullFileName + sep + dir_filename;

                    if( prjframe->AddFile( fil, id ) )
                    {
                        count++;
                    }
                } while( dir->GetNext( &dir_filename ) );
            }

            if( count == 0 )
            {
                /*  The AddFile() text below should match the filter added to handle
                    it in treeprj_frame.cpp in the line looking like this:
                    m_Filters.push_back( wxT( "^no kicad files found" ) );
                */
				prjframe->AddFile(
                    _( "no kicad files found in this directory" ), id );
            }

            /* Sort filenames by alphabetic order */
            m_Parent->SortChildren( id );
            delete dir;
        }
        m_Parent->Toggle( id );
        break;

    case TREE_SCHEMA:
        AddDelimiterString( FullFileName );
        ExecuteFile( m_Parent, EESCHEMA_EXE, FullFileName );
        break;

    case TREE_PCB:
        AddDelimiterString( FullFileName );
        ExecuteFile( m_Parent, PCBNEW_EXE, FullFileName );
        break;

#ifdef KICAD_PYTHON
    case TREE_PY:
        PyHandler::GetInstance()->RunScript( FullFileName );
        break;
#endif

    case TREE_GERBER:
        AddDelimiterString( FullFileName );
        ExecuteFile( m_Parent, GERBVIEW_EXE, FullFileName );
        break;

    case TREE_PDF:
        OpenPDF( FullFileName );
        break;

    case TREE_NET:
        AddDelimiterString( FullFileName );
        ExecuteFile( m_Parent, CVPCB_EXE, FullFileName );
        break;

    case TREE_TXT:
    {
        wxString editorname = wxGetApp().GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( m_Parent, editorname, FullFileName );
        break;
    }

    default:
        OpenFile( FullFileName );
        break;
    }
}


/***************************************************/
TreePrjItemData* WinEDA_PrjFrame::GetSelectedData()
/***************************************************/
{
    return dynamic_cast<TreePrjItemData*>( m_TreeProject->GetItemData( m_TreeProject->GetSelection() ) );
}
