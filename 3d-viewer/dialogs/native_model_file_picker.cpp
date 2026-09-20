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

#include "native_model_file_picker.h"

#if defined( KICAD_NATIVE_MODEL_PREVIEW ) && defined( __WXGTK3__ )
#include "native_model_file_picker_gtk.h"
#elif defined( KICAD_NATIVE_MODEL_PREVIEW ) && defined( __WINDOWS__ )
#include "native_model_file_picker_windows.h"
#else
#include <widgets/filedlg_hook_embed_file.h>
#include <wx/filedlg.h>
#endif


NATIVE_MODEL_FILE_PICKER_RESULT ShowNativeModelFilePicker( const NATIVE_MODEL_FILE_PICKER_OPTIONS& aOptions )
{
#if defined( KICAD_NATIVE_MODEL_PREVIEW ) && defined( __WXGTK3__ )
    return ShowNativeModelFilePickerGtk( aOptions );
#elif defined( KICAD_NATIVE_MODEL_PREVIEW ) && defined( __WINDOWS__ )
    return ShowNativeModelFilePickerWindows( aOptions );
#else
    wxFileDialog dialog( aOptions.parent, aOptions.title, aOptions.directory, aOptions.filename, aOptions.wildcard,
                         aOptions.style );
    FILEDLG_HOOK_EMBED_FILE embedHook( aOptions.embedFile );
    dialog.SetCustomizeHook( embedHook );

    dialog.SetFilterIndex( aOptions.filterIndex );
    NATIVE_MODEL_FILE_PICKER_RESULT selected;

    if( dialog.ShowModal() == wxID_OK )
    {
        selected.accepted = true;
        selected.path = dialog.GetPath();
        selected.filterIndex = dialog.GetFilterIndex();
        selected.embedFile = embedHook.GetEmbed();
    }

    return selected;
#endif
}
