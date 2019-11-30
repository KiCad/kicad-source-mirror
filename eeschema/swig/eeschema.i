/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file eeschema.i
 * @brief Access eeschema functions
 */

%module eeschema

%feature("autodoc", "1");
#ifdef ENABLE_DOCSTRINGS_FROM_DOXYGEN
%include docstrings.i
#endif

%include cwstring.i

%ignore EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon;
%ignore COLORS_DESIGN_SETTINGS;
%ignore SCH_RENDER_SETTINGS::ImportLegacyColors;

%include kicad.i

%include kiway.i

//%include project.h
%include plotter.h

%{
#include <project.h>
#include <plotter.h>
#include <general.h>
#include <sch_text.h>
#include <sch_sheet_path.h>
#include <sch_sheet.h>
#include <sch_legacy_plugin.h>
#include <sch_io_mgr.h>
#include <sch_screen.h>
%}

%include general.h
%include sch_text.h

%ignore SCH_SHEET::ChangeFileName;

%include sch_sheet_path.h
%include sch_sheet.h
%include enum_vector.h
%include import_export.h
%include sch_io_mgr.h
%include sch_legacy_plugin.h

%include base_screen.h
%include sch_screen.h
