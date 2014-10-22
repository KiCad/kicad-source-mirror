/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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


#ifndef MAIL_TYPE_H_
#define MAIL_TYPE_H_

/**
 * Enum MAIL_T
 * is the set of mail types sendable via KIWAY::ExpressMail() and supplied as
 * the @a aCommand parameter to that function.  Such mail will be received in
 * KIWAY_PLAYER::KiwayMailIn( KIWAY_EXPRESS& aEvent ) and aEvent.Command() will
 * match aCommand to KIWAY::ExpressMail().
 */
enum MAIL_T
{
    MAIL_CROSS_PROBE,               ///< PCB<->SCH, CVPCB->SCH cross-probing.
    MAIL_BACKANNOTATE_FOOTPRINTS,   ///< CVPCB->SCH footprint stuffing at cvpcb termination

};

#endif  // MAIL_TYPE_H_
