/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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
#include <dialog_fp_lib_table_base.h>
#include <fp_lib_table.h>

/**
 * Class DIALOG_FP_LIB_TABLE
 * shows and edits the PCB library tables.  Two tables are expected, one global
 * and one project specific.
 */
class DIALOG_FP_LIB_TABLE : public DIALOG_FP_LIB_TABLE_BASE
{
    //-----<event handlers>----------------------------------

    void pageChangedHandler( wxAuiNotebookEvent& event )
    {
        int pageNdx = m_auinotebook->GetSelection();

        m_cur_grid = pageNdx ? m_global_grid : m_project_grid;
    }

    void appendRowHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void deleteRowHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void moveUpHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void moveDownHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void onCancelButtonClick( wxCommandEvent& event )
    {
        m_global->rows  = m_orig_global;
        m_project->rows = m_orig_project;

        // @todo reindex, or add member function for wholesale row replacement

        EndModal( wxID_CANCEL );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        EndModal( wxID_OK );
    }


    //-----</event handlers>---------------------------------


    // caller's tables are modified only on OK button.
    FP_LIB_TABLE*       m_global;
    FP_LIB_TABLE*       m_project;

    // local copies are saved and restored if Cancel button.
    FP_LIB_TABLE::ROWS  m_orig_global;
    FP_LIB_TABLE::ROWS  m_orig_project;

    wxGrid*             m_cur_grid;     ///< changed based on tab choice


public:
    DIALOG_FP_LIB_TABLE( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject ) :
        DIALOG_FP_LIB_TABLE_BASE( aParent ),
        m_global( aGlobal ),
        m_project( aProject ),
        m_orig_global( aGlobal->rows ),
        m_orig_project( aProject->rows )
    {
        /*
        GetSizer()->SetSizeHints( this );
        Centre();
        SetAutoLayout( true );
        Layout();
        */

#if 1 && defined(DEBUG)
        // put some dummy data into table(s)
        FP_LIB_TABLE::ROW   row( m_global );

        row.SetNickName( wxT( "passives" ) );
        row.SetType( wxT( "kicad" ) );
        row.SetFullURI( wxT( "%G/passives" ) );
        row.SetOptions( wxT( "speed=fast,purpose=testing" ) );
        m_global->InsertRow( row );

        row.SetNickName( wxT( "micros" ) );
        row.SetType( wxT( "legacy" ) );
        row.SetFullURI( wxT( "%P/micros" ) );
        row.SetOptions( wxT( "speed=fast,purpose=testing" ) );
        m_global->InsertRow( row );

        row.owner = m_project;
        row.SetFullURI( wxT( "%P/chips" ) );
        m_project->InsertRow( row );

#endif

        m_global_grid->SetTable( m_global );
        m_project_grid->SetTable( m_project );

        //m_global_grid->AutoSize();
        m_global_grid->AutoSizeColumns( false );

        //m_project_grid->AutoSize();
        m_project_grid->AutoSizeColumns( false );

        //m_path_subs_grid->AutoSize();
        m_path_subs_grid->AutoSizeColumns( false );
    }
};



int InvokePcbLibTableEditor( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject )
{
    DIALOG_FP_LIB_TABLE     dlg( aParent, aGlobal, aProject );

    int ret = dlg.ShowModal();
    switch( ret )
    {
    case wxID_OK:
        break;

    case wxID_CANCEL:
        break;
    }

    return 0;
}

