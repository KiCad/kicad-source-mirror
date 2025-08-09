/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dialog_eeschema_page_settings.h"
#include <kiface_base.h>
#include <general.h>
#include <sch_edit_frame.h>
#include <sch_screen.h>
#include <schematic.h>
#include <eeschema_settings.h>


DIALOG_EESCHEMA_PAGE_SETTINGS::DIALOG_EESCHEMA_PAGE_SETTINGS( EDA_DRAW_FRAME* aParent,
                                                              EMBEDDED_FILES* aEmbeddedFiles,
                                                              VECTOR2I        aMaxUserSizeMils ) :
        DIALOG_PAGES_SETTINGS( aParent, aEmbeddedFiles, schIUScale.IU_PER_MILS, aMaxUserSizeMils )
{
}


DIALOG_EESCHEMA_PAGE_SETTINGS::~DIALOG_EESCHEMA_PAGE_SETTINGS()
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxCHECK( cfg, /* void */ );

    cfg->m_PageSettings.export_paper = m_PaperExport->GetValue();

    if( !m_TextRevision->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_revision = m_RevisionExport->GetValue();

    if( !m_TextDate->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_date = m_DateExport->GetValue();

    if( !m_TextTitle->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_title = m_TitleExport->GetValue();

    if( !m_TextCompany->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_company = m_CompanyExport->GetValue();

    if( !m_TextComment1->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment1 = m_Comment1Export->GetValue();

    if( !m_TextComment2->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment2 = m_Comment2Export->GetValue();

    if( !m_TextComment3->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment3 = m_Comment3Export->GetValue();

    if( !m_TextComment4->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment4 = m_Comment4Export->GetValue();

    if( !m_TextComment5->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment5 = m_Comment5Export->GetValue();

    if( !m_TextComment6->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment6 = m_Comment6Export->GetValue();

    if( !m_TextComment7->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment7 = m_Comment7Export->GetValue();

    if( !m_TextComment8->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment8 = m_Comment8Export->GetValue();

    if( !m_TextComment9->GetValue().IsEmpty() )
        cfg->m_PageSettings.export_comment9 = m_Comment9Export->GetValue();
}


void DIALOG_EESCHEMA_PAGE_SETTINGS::onTransferDataToWindow()
{
    m_TextSheetCount->Show( true );
    m_TextSheetNumber->Show( true );
    m_PaperExport->Show( true );
    m_RevisionExport->Show( true );
    m_DateExport->Show( true );
    m_TitleExport->Show( true );
    m_CompanyExport->Show( true );
    m_Comment1Export->Show( true );
    m_Comment2Export->Show( true );
    m_Comment3Export->Show( true );
    m_Comment4Export->Show( true );
    m_Comment5Export->Show( true );
    m_Comment6Export->Show( true );
    m_Comment7Export->Show( true );
    m_Comment8Export->Show( true );
    m_Comment9Export->Show( true );

    // Init display value for schematic sub-sheet number
    m_TextSheetCount->SetLabel( wxString::Format( m_TextSheetCount->GetLabel(), m_screen->GetPageCount() ) );
    m_TextSheetNumber->SetLabel( wxString::Format( m_TextSheetNumber->GetLabel(), m_screen->GetVirtualPageNumber() ) );

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxCHECK( cfg, /* void */ );

    m_PaperExport->SetValue( cfg->m_PageSettings.export_paper );
    m_RevisionExport->SetValue( m_TextRevision->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_revision );
    m_DateExport->SetValue( m_TextDate->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_date );
    m_TitleExport->SetValue( m_TextTitle->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_title );
    m_CompanyExport->SetValue( m_TextCompany->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_company );
    m_Comment1Export->SetValue( m_TextComment1->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment1 );
    m_Comment2Export->SetValue( m_TextComment2->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment2 );
    m_Comment3Export->SetValue( m_TextComment3->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment3 );
    m_Comment4Export->SetValue( m_TextComment4->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment4 );
    m_Comment5Export->SetValue( m_TextComment5->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment5 );
    m_Comment6Export->SetValue( m_TextComment6->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment6 );
    m_Comment7Export->SetValue( m_TextComment7->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment7 );
    m_Comment8Export->SetValue( m_TextComment8->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment8 );
    m_Comment9Export->SetValue( m_TextComment9->GetValue().IsEmpty() ? false : cfg->m_PageSettings.export_comment9 );
}


bool DIALOG_EESCHEMA_PAGE_SETTINGS::onSavePageSettings()
{
    wxCHECK_MSG( dynamic_cast<SCH_EDIT_FRAME*>( m_parent ), true,
                 "DIALOG_PAGES_SETTINGS::OnDateApplyClick frame is not a schematic frame!" );

    // Exports settings to other sheets if requested:
    SCH_SCREENS ScreenList( dynamic_cast<SCH_EDIT_FRAME*>( m_parent )->Schematic().Root() );

    // Update page info and/or title blocks for all screens
    for( SCH_SCREEN* screen = ScreenList.GetFirst(); screen; screen = ScreenList.GetNext() )
    {
        if( screen == m_screen )
            continue;

        if( m_PaperExport->IsChecked() )
            screen->SetPageSettings( m_pageInfo );

        TITLE_BLOCK tb2 = screen->GetTitleBlock();

        if( m_RevisionExport->IsChecked() )
            tb2.SetRevision( m_tb.GetRevision() );

        if( m_DateExport->IsChecked() )
            tb2.SetDate( m_tb.GetDate() );

        if( m_TitleExport->IsChecked() )
            tb2.SetTitle( m_tb.GetTitle() );

        if( m_CompanyExport->IsChecked() )
            tb2.SetCompany( m_tb.GetCompany() );

        if( m_Comment1Export->IsChecked() )
            tb2.SetComment( 0, m_tb.GetComment( 0 ) );

        if( m_Comment2Export->IsChecked() )
            tb2.SetComment( 1, m_tb.GetComment( 1 ) );

        if( m_Comment3Export->IsChecked() )
            tb2.SetComment( 2, m_tb.GetComment( 2 ) );

        if( m_Comment4Export->IsChecked() )
            tb2.SetComment( 3, m_tb.GetComment( 3 ) );

        if( m_Comment5Export->IsChecked() )
            tb2.SetComment( 4, m_tb.GetComment( 4 ) );

        if( m_Comment6Export->IsChecked() )
            tb2.SetComment( 5, m_tb.GetComment( 5 ) );

        if( m_Comment7Export->IsChecked() )
            tb2.SetComment( 6, m_tb.GetComment( 6 ) );

        if( m_Comment8Export->IsChecked() )
            tb2.SetComment( 7, m_tb.GetComment( 7 ) );

        if( m_Comment9Export->IsChecked() )
            tb2.SetComment( 8, m_tb.GetComment( 8 ) );

        screen->SetTitleBlock( tb2 );
    }

    return true;
}
