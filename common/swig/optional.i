/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Haemmerling <dev@markh.de>
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

/**
 * @file optional.i
 * @brief typemaps for std::optional<int> and std::optional<double>
 */

/* Handle std::optional<int> */
%typemap(in) (std::optional<int>) {
    if ($input == Py_None)
        $1 = std::nullopt;
    else if (PyLong_Check($input))
        $1 = std::optional<int>(PyLong_AsLong($input));
    else
    {
        PyErr_SetString(PyExc_TypeError, "Need an int or None");
        SWIG_fail;
    }
}

%typemap(out) std::optional<int> {
    if ($1)
        $result = PyLong_FromLong(*$1);
    else
    {
        Py_INCREF(Py_None);
        $result = Py_None;
    }
}

/* Handle std::optional<double> */
%typemap(in) (std::optional<double>) {
    if ($input == Py_None)
        $1 = std::nullopt;
    else if (PyFloat_Check($input))
        $1 = std::optional<double>(PyFloat_AsDouble($input));
    else
    {
        PyErr_SetString(PyExc_TypeError, "Need a float or None");
        SWIG_fail;
    }
}

%typemap(out) std::optional<double> {
    if ($1)
        $result = PyFloat_FromDouble(*$1);
    else
    {
        Py_INCREF(Py_None);
        $result = Py_None;
    }
}
