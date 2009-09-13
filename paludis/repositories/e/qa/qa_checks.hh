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

#ifndef PALUDIS_GUARD_PALUDIS_REPOSITORIES_E_QA_QA_CHECKS_HH
#define PALUDIS_GUARD_PALUDIS_REPOSITORIES_E_QA_QA_CHECKS_HH 1

#include <paludis/util/attributes.hh>
#include <paludis/util/private_implementation_pattern.hh>
#include <paludis/util/fs_entry-fwd.hh>

#include <paludis/environment-fwd.hh>
#include <paludis/qa-fwd.hh>
#include <paludis/package_id-fwd.hh>

#include <paludis/repositories/e/qa/qa_checks_group.hh>
#include <paludis/repositories/e/e_repository_id.hh>
#include <paludis/repositories/e/e_repository.hh>

#include <tr1/functional>
#include <tr1/memory>

namespace paludis
{
    namespace erepository
    {
        typedef std::tr1::function<bool (
                const FSEntry &,
                QAReporter &,
                const Environment * const,
                const std::tr1::shared_ptr<const ERepository> &
                )> TreeCheckFunction;

        typedef std::tr1::function<bool (
                const FSEntry &,
                QAReporter &,
                const Environment * const,
                const std::tr1::shared_ptr<const ERepository> &,
                const std::string &
                )> EclassFileContentsCheckFunction;

        typedef std::tr1::function<bool (
                const FSEntry &,
                QAReporter &,
                const Environment * const,
                const std::tr1::shared_ptr<const ERepository> &,
                const CategoryNamePart &
                )> CategoryDirCheckFunction;

        typedef std::tr1::function<bool (
                const FSEntry &,
                QAReporter &,
                const Environment * const,
                const std::tr1::shared_ptr<const ERepository> &,
                const QualifiedPackageName &
                )> PackageDirCheckFunction;

        typedef std::tr1::function<bool (
                const FSEntry &,
                QAReporter &,
                const Environment * const,
                const std::tr1::shared_ptr<const ERepository> &,
                const std::tr1::shared_ptr<const ERepositoryID> &
                )> PackageIDCheckFunction;

        typedef std::tr1::function<bool (
                const FSEntry &,
                QAReporter &,
                const Environment * const,
                const std::tr1::shared_ptr<const ERepository> &,
                const std::tr1::shared_ptr<const ERepositoryID> &,
                const std::string &
                )> PackageIDFileContentsCheckFunction;

        class QAChecks :
            private PrivateImplementationPattern<QAChecks>
        {
            public:
                QAChecks(const Environment * const);
                ~QAChecks();

                const std::tr1::shared_ptr<QAChecksGroup<TreeCheckFunction> >
                    tree_checks_group() PALUDIS_ATTRIBUTE((warn_unused_result));

                const std::tr1::shared_ptr<QAChecksGroup<EclassFileContentsCheckFunction> >
                    eclass_file_contents_checks_group() PALUDIS_ATTRIBUTE((warn_unused_result));

                const std::tr1::shared_ptr<QAChecksGroup<CategoryDirCheckFunction> >
                    category_dir_checks_group() PALUDIS_ATTRIBUTE((warn_unused_result));

                const std::tr1::shared_ptr<QAChecksGroup<PackageDirCheckFunction> >
                    package_dir_checks_group() PALUDIS_ATTRIBUTE((warn_unused_result));

                const std::tr1::shared_ptr<QAChecksGroup<PackageIDCheckFunction> >
                    package_id_checks_group() PALUDIS_ATTRIBUTE((warn_unused_result));

                const std::tr1::shared_ptr<QAChecksGroup<PackageIDFileContentsCheckFunction> >
                    package_id_file_contents_checks_group() PALUDIS_ATTRIBUTE((warn_unused_result));
        };
    }
}

#endif