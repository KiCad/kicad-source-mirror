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

#ifndef KIQL_PREVIEW_VIEW_CONTROLLER_H
#define KIQL_PREVIEW_VIEW_CONTROLLER_H

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

/**
 * Quick Look preview extension principal class.
 */
@interface KIQL_PREVIEW_VIEW_CONTROLLER : NSViewController <QLPreviewingController>
@end

#endif // KIQL_PREVIEW_VIEW_CONTROLLER_H
