/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007, 2008 Ciaran McCreesh
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

#include "fake_installed_repository.hh"
#include <paludis/util/fs_entry.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/set.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/package_id.hh>
#include <paludis/metadata_key.hh>
#include <paludis/environment.hh>
#include <paludis/dep_spec_flattener.hh>
#include <paludis/dep_spec.hh>
#include <paludis/action.hh>
#include <paludis/literal_metadata_key.hh>

using namespace paludis;

namespace paludis
{
    template <>
    struct Implementation<FakeInstalledRepository>
    {
        tr1::shared_ptr<const MetadataValueKey<std::string> > format_key;
        tr1::shared_ptr<const MetadataValueKey<FSEntry> > installed_root_key;

        Implementation() :
            format_key(new LiteralMetadataValueKey<std::string> (
                        "format", "format", mkt_significant, "installed_fake")),
            installed_root_key(new LiteralMetadataValueKey<FSEntry> (
                        "installed_root", "installed_root", mkt_normal, FSEntry("/")))
        {
        }
    };
}

FakeInstalledRepository::FakeInstalledRepository(const Environment * const e, const RepositoryName & our_name) :
    FakeRepositoryBase(e, our_name, RepositoryCapabilities::named_create()
            (k::sets_interface(), this)
            (k::syncable_interface(), static_cast<RepositorySyncableInterface *>(0))
            (k::use_interface(), this)
            (k::mirrors_interface(), static_cast<RepositoryMirrorsInterface *>(0))
            (k::environment_variable_interface(), static_cast<RepositoryEnvironmentVariableInterface *>(0))
            (k::provides_interface(), this)
            (k::virtuals_interface(), static_cast<RepositoryVirtualsInterface *>(0))
            (k::destination_interface(), this)
            (k::e_interface(), static_cast<RepositoryEInterface *>(0))
            (k::make_virtuals_interface(), static_cast<RepositoryMakeVirtualsInterface *>(0))
            (k::qa_interface(), static_cast<RepositoryQAInterface *>(0))
            (k::hook_interface(), static_cast<RepositoryHookInterface *>(0))
            (k::manifest_interface(), static_cast<RepositoryManifestInterface *>(0)),
            "0"),
    PrivateImplementationPattern<FakeInstalledRepository>(new Implementation<FakeInstalledRepository>),
    _imp(PrivateImplementationPattern<FakeInstalledRepository>::_imp)
{
    add_metadata_key(_imp->format_key);
    add_metadata_key(_imp->installed_root_key);
}

FakeInstalledRepository::~FakeInstalledRepository()
{
}

bool
FakeInstalledRepository::is_suitable_destination_for(const PackageID &) const
{
    return true;
}

tr1::shared_ptr<const FakeInstalledRepository::ProvidesSequence>
FakeInstalledRepository::provided_packages() const
{
    tr1::shared_ptr<ProvidesSequence> result(new ProvidesSequence);

    tr1::shared_ptr<const CategoryNamePartSet> cats(category_names());
    for (CategoryNamePartSet::ConstIterator c(cats->begin()), c_end(cats->end()) ;
            c != c_end ; ++c)
    {
        tr1::shared_ptr<const QualifiedPackageNameSet> pkgs(package_names(*c));
        for (QualifiedPackageNameSet::ConstIterator p(pkgs->begin()), p_end(pkgs->end()) ;
                p != p_end ; ++p)
        {
            tr1::shared_ptr<const PackageIDSequence> vers(package_ids(*p));
            for (PackageIDSequence::ConstIterator v(vers->begin()), v_end(vers->end()) ;
                    v != v_end ; ++v)
            {
                if (! (*v)->provide_key())
                    continue;

                DepSpecFlattener<ProvideSpecTree, PackageDepSpec> f(environment());
                (*v)->provide_key()->value()->accept(f);

                for (DepSpecFlattener<ProvideSpecTree, PackageDepSpec>::ConstIterator q(f.begin()), q_end(f.end()) ; q != q_end ; ++q)
                    result->push_back(RepositoryProvidesEntry::named_create()
                            (k::virtual_name(), QualifiedPackageName((*q)->text()))
                            (k::provided_by(), *v));
            }
        }
    }

    return result;
}

bool
FakeInstalledRepository::is_default_destination() const
{
    return environment()->root() == installed_root_key()->value();
}

bool
FakeInstalledRepository::want_pre_post_phases() const
{
    return false;
}

void
FakeInstalledRepository::merge(const MergeParams &)
{
}

namespace
{
    struct SupportsActionQuery :
        ConstVisitor<SupportsActionTestVisitorTypes>
    {
        bool result;

        SupportsActionQuery() :
            result(false)
        {
        }

        void visit(const SupportsActionTest<InstalledAction> &)
        {
            result = true;
        }

        void visit(const SupportsActionTest<InstallAction> &)
        {
        }

        void visit(const SupportsActionTest<ConfigAction> &)
        {
        }

        void visit(const SupportsActionTest<PretendAction> &)
        {
        }

        void visit(const SupportsActionTest<FetchAction> &)
        {
        }

        void visit(const SupportsActionTest<PretendFetchAction> &)
        {
        }

        void visit(const SupportsActionTest<InfoAction> &)
        {
        }

        void visit(const SupportsActionTest<UninstallAction> &)
        {
        }
    };
}

bool
FakeInstalledRepository::some_ids_might_support_action(const SupportsActionTestBase & a) const
{
    SupportsActionQuery q;
    a.accept(q);
    return q.result;
}

const tr1::shared_ptr<const MetadataValueKey<std::string> >
FakeInstalledRepository::format_key() const
{
    return _imp->format_key;
}

const tr1::shared_ptr<const MetadataValueKey<FSEntry> >
FakeInstalledRepository::installed_root_key() const
{
    return _imp->installed_root_key;
}

