/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007, 2008 Ciaran McCreesh
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

#ifndef PALUDIS_GUARD_PALUDIS_DISTRIBUTION_FWD_HH
#define PALUDIS_GUARD_PALUDIS_DISTRIBUTION_FWD_HH 1

/** \file
 * Forward declarations for paludis/distribution.hh .
 *
 * \ingroup g_dep_spec
 */

#include <paludis/util/kc-fwd.hh>
#include <paludis/util/keys.hh>
#include <string>

namespace paludis
{
    class DistributionData;
    class DistributionConfigurationError;

    /**
     * Information about a distribution.
     *
     * \see DistributionData
     * \ingroup g_distribution
     * \since 0.26
     * \nosubgrouping
     */
    typedef kc::KeyedClass<
        kc::Field<k::default_environment, std::string>,
        kc::Field<k::fallback_environment, std::string>,
        kc::Field<k::support_old_style_virtuals, bool>,
        kc::Field<k::default_ebuild_distdir, std::string>,
        kc::Field<k::default_ebuild_write_cache, std::string>,
        kc::Field<k::default_ebuild_names_cache, std::string>,
        kc::Field<k::default_ebuild_builddir, std::string>,
        kc::Field<k::default_ebuild_layout, std::string>,
        kc::Field<k::default_ebuild_eapi_when_unknown, std::string>,
        kc::Field<k::default_ebuild_eapi_when_unspecified, std::string>,
        kc::Field<k::default_ebuild_profile_eapi, std::string>,
        kc::Field<k::default_vdb_provides_cache, std::string>,
        kc::Field<k::default_vdb_names_cache, std::string>,
        kc::Field<k::paludis_environment_use_conf_filename, std::string>,
        kc::Field<k::paludis_environment_keywords_conf_filename, std::string>,
        kc::Field<k::concept_use, std::string>,
        kc::Field<k::concept_keyword, std::string>,
        kc::Field<k::paludis_package, std::string>
            > Distribution;
}

#endif
