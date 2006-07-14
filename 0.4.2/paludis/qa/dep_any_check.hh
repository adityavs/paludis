/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
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

#ifndef PALUDIS_GUARD_PALUDIS_QA_DEP_ANY_CHECK_HH
#define PALUDIS_GUARD_PALUDIS_QA_DEP_ANY_CHECK_HH 1

#include <paludis/qa/ebuild_check.hh>
#include <string>

namespace paludis
{
    namespace qa
    {
        class DepAnyCheck :
            public EbuildCheck
        {
            public:
                DepAnyCheck();

                CheckResult operator() (const EbuildCheckData &) const;

                static const std::string & identifier();

                virtual std::string describe() const
                {
                    return "Checks || ( ) block sanity in *DEPEND";
                }
        };

        static const EbuildCheckMaker::RegisterMaker register_dep_any_check(
                DepAnyCheck::identifier(), &MakeEbuildCheck<DepAnyCheck>::make_ebuild_check);
    }
}

#endif
