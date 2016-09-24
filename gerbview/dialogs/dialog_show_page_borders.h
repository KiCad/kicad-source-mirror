/**
 * @file dialog_show_page_borders.h
 * Dialog to show/hide frame reference and select paper size for printing
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

#include <dialog_show_page_borders_base.h>


class DIALOG_PAGE_SHOW_PAGE_BORDERS : public DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE
{
private:
    GERBVIEW_FRAME* m_Parent;

public:

    DIALOG_PAGE_SHOW_PAGE_BORDERS( GERBVIEW_FRAME* parent );
    ~DIALOG_PAGE_SHOW_PAGE_BORDERS() {};

private:
    void OnOKBUttonClick( wxCommandEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;
};

