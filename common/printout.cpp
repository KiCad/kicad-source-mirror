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

#include <pcbplot.h>
#include <printout.h>
#include <settings/app_settings.h>


void PRINTOUT_SETTINGS::Save( APP_SETTINGS_BASE* aConfig )
{
    aConfig->m_Printing.monochrome  = m_blackWhite;
    aConfig->m_Printing.title_block = m_titleBlock;
    aConfig->m_Printing.scale       = m_scale;
}


void PRINTOUT_SETTINGS::Load( APP_SETTINGS_BASE* aConfig )
{
    m_blackWhite = aConfig->m_Printing.monochrome;
    m_titleBlock = aConfig->m_Printing.title_block;
    m_scale      = aConfig->m_Printing.scale;
}
