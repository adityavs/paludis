/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PALUDIS_GUARD_PALUDIS_MASK_REASONS_HH
#define PALUDIS_GUARD_PALUDIS_MASK_REASONS_HH 1

#include <paludis/util/options.hh>
#include <iosfwd>

/** \file
 * Declarations for MaskReasons and related classes.
 *
 * \ingroup grpmaskreasons
 */

namespace paludis
{

#include <paludis/mask_reasons-se.hh>

    /**
     * A collection of reasons for why a package is masked.
     *
     * \ingroup grpmaskreasons
     */
    typedef Options<MaskReason> MaskReasons;
}

#endif
