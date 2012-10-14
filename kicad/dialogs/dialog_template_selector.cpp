/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_template_selector.h"

#include <wx/dir.h>
#include <wx/settings.h>


TEMPLATE_SELECTION_PANEL::TEMPLATE_SELECTION_PANEL( wxWindow* aParent ) :
    TEMPLATE_SELECTION_PANEL_BASE( aParent )
{
    parent = aParent;
}


TEMPLATE_SELECTION_PANEL::~TEMPLATE_SELECTION_PANEL()
{

}


TEMPLATE_WIDGET::TEMPLATE_WIDGET( wxWindow* aParent, wxDialog* aDialog ) :
    TEMPLATE_WIDGET_BASE( aParent )
{
    parent = aParent;
    dialog = aDialog;

    // wxWidgets 2.9.x way of doing the same...
    // Bind(wxEVT_LEFT_DOWN, &TEMPLATE_WIDGET::OnMouse, this );

    m_bitmapIcon->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), NULL, this );
    m_staticTitle->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), NULL, this );

    // We're not selected until we're clicked
    Unselect();

    // Start with template being NULL
    templ = NULL;
}


TEMPLATE_WIDGET::~TEMPLATE_WIDGET()
{

}

void TEMPLATE_WIDGET::Select()
{
    DIALOG_TEMPLATE_SELECTOR* d = (DIALOG_TEMPLATE_SELECTOR*)dialog;
    d->SetWidget( this );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );
    selected = true;
    Refresh();
}


void TEMPLATE_WIDGET::Unselect()
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    selected = false;
    Refresh();
}


bool TEMPLATE_WIDGET::IsSelected()
{
    return selected;
}


PROJECT_TEMPLATE* TEMPLATE_WIDGET::GetTemplate()
{
    return templ;
}


void TEMPLATE_WIDGET::SetTemplate(PROJECT_TEMPLATE* aTemplate)
{
    templ = aTemplate;
    m_staticTitle->SetLabel( *(aTemplate->GetTitle()) );
    m_bitmapIcon->SetBitmap( *(aTemplate->GetIcon()) );
}


void TEMPLATE_WIDGET::OnMouse( wxMouseEvent& event )
{
    /* Toggle selection here */
    Select();
    event.Skip();
}


void DIALOG_TEMPLATE_SELECTOR::onNotebookResize(wxSizeEvent& event)
{
    for ( size_t i=0; i < m_notebook->GetPageCount(); i++ )
    {
        m_panels[i]->SetSize( m_notebook->GetSize().GetWidth() - 6, 140 );
        m_panels[i]->m_SizerBase->FitInside( m_panels[i] );
        m_panels[i]->m_scrolledWindow1->SetSize( m_panels[i]->GetSize().GetWidth() - 6,
                                                 m_panels[i]->GetSize().GetHeight() - 6 );
        m_panels[i]->m_SizerChoice->FitInside( m_panels[i]->m_scrolledWindow1 );
    }
    m_notebook->Refresh();

    event.Skip();
}


DIALOG_TEMPLATE_SELECTOR::DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent ) :
    DIALOG_TEMPLATE_SELECTOR_BASE( aParent )
{
    m_htmlWin->SetPage( _( "<html><h1>Template Selector</h1></html>" ) );
    m_notebook->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_TEMPLATE_SELECTOR::onNotebookResize ), NULL, this );
    m_selectedWidget = NULL;
}


void DIALOG_TEMPLATE_SELECTOR::SetHtml( wxFileName aFilename )
{
    m_htmlWin->LoadPage( aFilename.GetFullPath() );
}


DIALOG_TEMPLATE_SELECTOR::~DIALOG_TEMPLATE_SELECTOR()
{

}


TEMPLATE_WIDGET* DIALOG_TEMPLATE_SELECTOR::GetWidget()
{
    return m_selectedWidget;
}


void DIALOG_TEMPLATE_SELECTOR::SetWidget( TEMPLATE_WIDGET* aWidget )
{
    if( m_selectedWidget != NULL )
        m_selectedWidget->Unselect();

    m_selectedWidget = aWidget;
    SetHtml( m_selectedWidget->GetTemplate()->GetHtmlFile() );
}

void DIALOG_TEMPLATE_SELECTOR::AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate )
{
    TEMPLATE_WIDGET* w = new TEMPLATE_WIDGET( m_panels[aPage]->m_scrolledWindow1, this  );
    w->SetTemplate( aTemplate );

    m_panels[aPage]->m_SizerChoice->Add( w );
    m_panels[aPage]->m_SizerChoice->Layout();
    m_panels[aPage]->SetSize( m_notebook->GetSize().GetWidth() - 6, 140 );
    m_panels[aPage]->m_SizerBase->FitInside( m_panels[aPage] );
    m_panels[aPage]->m_scrolledWindow1->SetSize( m_panels[aPage]->GetSize().GetWidth() - 6,
                                                 m_panels[aPage]->GetSize().GetHeight() - 6 );
    m_panels[aPage]->m_SizerChoice->FitInside( m_panels[aPage]->m_scrolledWindow1 );

    m_notebook->Refresh();
}


void DIALOG_TEMPLATE_SELECTOR::AddPage( const wxString& aTitle, wxFileName& aPath )
{
    wxNotebookPage* newPage = new wxNotebookPage( m_notebook, wxID_ANY );
    m_notebook->AddPage( newPage, aTitle );

    TEMPLATE_SELECTION_PANEL* p = new TEMPLATE_SELECTION_PANEL( newPage );
    m_panels.push_back( p );

    // Get a list of files under the template path to include as choices...
    wxArrayString files;
    wxDir dir, sub;

    if ( dir.Open( aPath.GetPath() ) )
    {
        wxString filename;
        bool cont = dir.GetFirst( &filename, wxEmptyString, wxDIR_FILES | wxDIR_DIRS );

        while( cont )
        {
            if( sub.Open( aPath.GetPathWithSep() + filename ) )
            {
                files.Add( filename );
                PROJECT_TEMPLATE* pt = new PROJECT_TEMPLATE( aPath.GetPathWithSep() + filename );
                AddTemplate( m_notebook->GetPageCount() - 1, pt );
            }

            cont = dir.GetNext( &filename );
        }
    }
}
