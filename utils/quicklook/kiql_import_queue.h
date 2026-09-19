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

#ifndef KIQL_IMPORT_QUEUE_H
#define KIQL_IMPORT_QUEUE_H

#include <plugins/3dapi/model_import_queue.h>

/**
 * The extension's single import queue.
 *
 * This is the same S3D::MODEL_IMPORT_QUEUE the Windows preview handler and the GTK file picker
 * use, so all three share one supersession, cancellation and generation policy.  Quick Look
 * reuses an extension process across files, so one queue per process is what makes swiping
 * through a folder cost one import rather than one per file.
 */
S3D::MODEL_IMPORT_QUEUE& KIQL_SharedImportQueue();

#endif // KIQL_IMPORT_QUEUE_H
