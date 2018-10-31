/*
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#include <printout.h>
#include <pcbplot.h>
#include <wx/config.h>

void PRINTOUT_SETTINGS::Save( wxConfigBase* aConfig )
{
    aConfig->Write( OPTKEY_PRINT_SCALE, m_scale );
    aConfig->Write( OPTKEY_PRINT_PAGE_FRAME, m_titleBlock );
    aConfig->Write( OPTKEY_PRINT_MONOCHROME_MODE, m_blackWhite );
}


void PRINTOUT_SETTINGS::Load( wxConfigBase* aConfig )
{
    aConfig->Read( OPTKEY_PRINT_SCALE, &m_scale, 1.0 );
    aConfig->Read( OPTKEY_PRINT_PAGE_FRAME, &m_titleBlock, false );
    aConfig->Read( OPTKEY_PRINT_MONOCHROME_MODE, &m_blackWhite, 1 );
}
