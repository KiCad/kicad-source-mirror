/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#import "kiql_import_queue.h"

S3D::MODEL_IMPORT_QUEUE& KIQL_SharedImportQueue()
{
    // Leaked deliberately: the queue joins its worker in its destructor, and an extension process
    // can be torn down while a callback is still in flight.
    static S3D::MODEL_IMPORT_QUEUE* queue = new S3D::MODEL_IMPORT_QUEUE();

    return *queue;
}
