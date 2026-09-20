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

#ifndef NATIVE_MODEL_FILE_PICKER_GTK_H
#define NATIVE_MODEL_FILE_PICKER_GTK_H

#ifdef __WXGTK3__

#include "native_model_file_picker.h"

NATIVE_MODEL_FILE_PICKER_RESULT
ShowNativeModelFilePickerGtk( const NATIVE_MODEL_FILE_PICKER_OPTIONS& aOptions );

void ShutdownNativeModelFilePickerGtk();

#endif // __WXGTK3__

#endif // NATIVE_MODEL_FILE_PICKER_GTK_H
