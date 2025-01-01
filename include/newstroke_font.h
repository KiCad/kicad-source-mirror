/*
 * newstroke_font.h - header for automatically converted font
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 vladimir uryvaev <vovanius@bk.ru>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef __NEWSTROKE_FONT_H__
#define __NEWSTROKE_FONT_H__

#include <kicommon.h>

/**
 * Array containing strokes for unicode glyphs
 */
extern KICOMMON_API const char* const newstroke_font[];        //The font
extern KICOMMON_API const int         newstroke_font_bufsize; //font buffer size

#endif /* __NEWSTROKE_FONT_H__ */
