/**
 * @file class_treeproject_item.cpp
 *
 * Class TREEPROJECT_ITEM is a derived  class from wxTreeItemData that
 * store info about a file or directory shown in the KiCad tree project files
 */

#include <fctsys.h>
#include <gestfich.h>

#include <kicad.h>
#include <tree_project_frame.h>
#include <class_treeprojectfiles.h>
#include <class_treeproject_item.h>
#include <wx/imaglist.h>

#include <wx/regex.h>
#include <wx/dir.h>

/* sort function for tree items.
 *  items are sorted :
 *  directory names first by alphabetic order
 *  root file names after
 *  file names last by alphabetic order
 */



TREEPROJECT_ITEM::TREEPROJECT_ITEM( enum TreeFileType type, const wxString& data,
                                  wxTreeCtrl* parent ) :
    wxTreeItemData()
{
    m_Type       = type;
    m_Parent     = parent;
    m_FileName   = data;
    m_IsRootFile = false;       // true only for the root item of the tree (the project name)
    m_WasPopulated = false;
}

// Set the state used in the icon list
void TREEPROJECT_ITEM::SetState( int state )
{
    wxImageList* imglist = m_Parent->GetImageList();

    if( !imglist || state < 0 || state >= imglist->GetImageCount() / ( TREE_MAX - 2 ) )
        return;
    m_State = state;
    int imgid = m_Type - 1 + state * ( TREE_MAX - 1 );
    m_Parent->SetItemImage( GetId(), imgid );
    m_Parent->SetItemImage( GetId(), imgid, wxTreeItemIcon_Selected );
}


/* Get the directory containing the file */
wxString TREEPROJECT_ITEM::GetDir() const
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


/* Called upon tree item rename */
void TREEPROJECT_ITEM::OnRename( wxTreeEvent& event, bool check )
{
    //this segfaults on linux (in wxEvtHandler::ProcessEvent), wx version 2.8.7
    //therefore, until it is fixed, we must cancel the rename.
    event.Veto();
    return;

    if( !Rename( event.GetLabel(), check ) )
        event.Veto();
}


// Move the object to dest
void TREEPROJECT_ITEM::Move( TREEPROJECT_ITEM* dest )
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
    if( dest == dynamic_cast<TREEPROJECT_ITEM*>( m_Parent->GetItemData( parent ) ) )
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
    if( !wxRenameFile( GetFileName(), destName, false ) )
    {
        wxMessageDialog( m_Parent, _( "Unable to move file ... " ),
                         _( "Permission error ?" ), wxICON_ERROR | wxOK );
        return;
    }

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
        dynamic_cast<TREEPROJECTFILES*>( m_Parent )->GetParent()->m_Parent->OnRefresh( dummy );
    }
}


/* rename the file checking if extension change occurs */
bool TREEPROJECT_ITEM::Rename( const wxString& name, bool check )
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

    wxString ext = TREE_PROJECT_FRAME::GetFileExt( GetType() );

    wxRegEx             reg( wxT ( "^.*\\" ) + ext + wxT( "$" ), wxRE_ICASE );

    if( check && !ext.IsEmpty() && !reg.Matches( newFile ) )
    {
        wxMessageDialog dialog( m_Parent,
                                _( "Changing file extension will change file \
type.\n Do you want to continue ?" ),
                                _( "Rename File" ),
                                wxYES_NO | wxICON_QUESTION );

        if( wxID_YES != dialog.ShowModal() )
            return false;
    }

#if ( ( wxMAJOR_VERSION < 2 ) || ( ( wxMAJOR_VERSION == 2 ) \
                                   && ( wxMINOR_VERSION < 7 ) ) )
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

    return true;
}


/*******************************************/
bool TREEPROJECT_ITEM::Delete( bool check )
/*******************************************/
/* delete a file */
{
    wxMessageDialog dialog( m_Parent,
                            _ ("Do you really want to delete ") + GetFileName(),
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

        return true;
    }
    return false;
}


/* Called under item activation */
void TREEPROJECT_ITEM::Activate( TREE_PROJECT_FRAME* prjframe )
{
    wxString     sep = wxFileName().GetPathSeparator();
    wxString     FullFileName = GetFileName();
    wxTreeItemId id = GetId();

    AddDelimiterString( FullFileName );
    switch( GetType() )
    {
    case TREE_PROJECT:
        break;

    case TREE_DIRECTORY:
        m_Parent->Toggle( id );
        break;

    case TREE_SCHEMA:
        ExecuteFile( m_Parent, EESCHEMA_EXE, FullFileName );
        break;

    case TREE_PCB:
        ExecuteFile( m_Parent, PCBNEW_EXE, FullFileName );
        break;

    case TREE_GERBER:
        ExecuteFile( m_Parent, GERBVIEW_EXE, FullFileName );
        break;

    case TREE_PDF:
        OpenPDF( FullFileName );
        break;

    case TREE_NET:
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

