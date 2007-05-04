/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_REPOSITORIES_FAKE_FAKE_REPOSITORY_HH
#define PALUDIS_GUARD_PALUDIS_REPOSITORIES_FAKE_FAKE_REPOSITORY_HH 1

#include <paludis/repositories/fake/fake_repository_base.hh>

namespace paludis
{
    /**
     * Fake repository for use in test cases.
     *
     * \ingroup grpfakerepository
     */
    class PALUDIS_VISIBLE FakeRepository :
        public FakeRepositoryBase,
        public RepositoryInstallableInterface,
        public RepositoryVirtualsInterface
    {
        private:
            std::tr1::shared_ptr<VirtualsCollection> _virtual_packages;

        protected:
            virtual void do_install(const QualifiedPackageName &, const VersionSpec &,
                    const InstallOptions &) const;

            virtual std::tr1::shared_ptr<const VirtualsCollection> virtual_packages() const;

            virtual std::tr1::shared_ptr<const VersionMetadata> virtual_package_version_metadata(
                    const RepositoryVirtualsEntry &, const VersionSpec & v) const;

        public:
            ///\name Basic operations
            ///\{

            FakeRepository(const Environment * const, const RepositoryName &);

            ///\}

            /**
             * Add a virtual package.
             */
            void add_virtual_package(const QualifiedPackageName &, std::tr1::shared_ptr<const PackageDepSpec>);
    };
}

#endif
