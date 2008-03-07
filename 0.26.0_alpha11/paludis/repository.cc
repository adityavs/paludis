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

#include <paludis/repository.hh>
#include <paludis/util/log.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/instantiation_policy-impl.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/indirect_iterator-impl.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/options.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/sequence-impl.hh>
#include <paludis/util/set.hh>
#include <paludis/util/set-impl.hh>
#include <paludis/util/config_file.hh>
#include <paludis/util/wrapped_forward_iterator-impl.hh>
#include <paludis/util/tr1_functional.hh>
#include <paludis/metadata_key.hh>
#include <map>
#include <list>
#include <utility>
#include <algorithm>
#include <ctype.h>

using namespace paludis;

template class Set<tr1::shared_ptr<Repository> >;
template class WrappedForwardIterator<Set<tr1::shared_ptr<Repository> >::ConstIteratorTag, const tr1::shared_ptr<Repository> >;
template class WrappedOutputIterator<Set<tr1::shared_ptr<Repository> >::InserterTag, tr1::shared_ptr<Repository> >;

template class Sequence<RepositoryVirtualsEntry>;
template class WrappedForwardIterator<Sequence<RepositoryVirtualsEntry>::ConstIteratorTag, const RepositoryVirtualsEntry>;
template class WrappedOutputIterator<Sequence<RepositoryVirtualsEntry>::InserterTag, RepositoryVirtualsEntry>;

template class Sequence<RepositoryProvidesEntry>;
template class WrappedForwardIterator<Sequence<RepositoryProvidesEntry>::ConstIteratorTag, const RepositoryProvidesEntry>;
template class WrappedOutputIterator<Sequence<RepositoryProvidesEntry>::InserterTag, RepositoryProvidesEntry>;

template class WrappedForwardIterator<RepositoryMirrorsInterface::MirrorsConstIteratorTag,
         const std::pair<const std::string, std::string> >;
template class WrappedForwardIterator<RepositoryEInterface::ProfilesConstIteratorTag, const RepositoryEInterface::ProfilesDescLine>;

NoSuchSetError::NoSuchSetError(const std::string & our_name) throw () :
    Exception("Could not find '" + our_name + "'"),
    _name(our_name)
{
}

RecursivelyDefinedSetError::RecursivelyDefinedSetError(const std::string & our_name) throw () :
    Exception("Set '" + our_name + "' is recursively defined"),
    _name(our_name)
{
}

namespace
{
    struct RepositoryBlacklist :
        InstantiationPolicy<RepositoryBlacklist, instantiation_method::SingletonTag>
    {
        std::map<std::string, std::string> items;

        RepositoryBlacklist()
        {
            if (! (FSEntry(DATADIR) / "paludis" / "repository_blacklist.txt").exists())
                return;

            LineConfigFile f(FSEntry(DATADIR) / "paludis" / "repository_blacklist.txt", LineConfigFileOptions());
            for (LineConfigFile::ConstIterator line(f.begin()), line_end(f.end()) ;
                    line != line_end ; ++line)
            {
                std::string::size_type p(line->find(" - "));
                if (std::string::npos != p)
                    items.insert(std::make_pair(line->substr(0, p), line->substr(p + 3)));
            }
        }
    };
}

namespace paludis
{
    template <>
    struct Implementation<Repository>
    {
        const RepositoryName name;

        Implementation(const RepositoryName & n) :
            name(n)
        {
        }
    };
}

Repository::Repository(
        const RepositoryName & our_name,
        const RepositoryCapabilities & caps) :
    PrivateImplementationPattern<Repository>(new Implementation<Repository>(our_name)),
    RepositoryCapabilities(caps),
    _imp(PrivateImplementationPattern<Repository>::_imp)
{
    std::map<std::string, std::string>::const_iterator i(
            RepositoryBlacklist::get_instance()->items.find(stringify(name())));
    if (RepositoryBlacklist::get_instance()->items.end() != i)
        Log::get_instance()->message(ll_warning, lc_no_context, "Repository '" + stringify(name()) +
                "' is blacklisted with reason '" + i->second + "'. Consult the FAQ for more details.");
}

Repository::~Repository()
{
}

const RepositoryName
Repository::name() const
{
    return _imp->name;
}

tr1::shared_ptr<const CategoryNamePartSet>
Repository::category_names_containing_package(const PackageNamePart & p) const
{
    Context context("When finding category names containing package '" + stringify(p) + "':");

    tr1::shared_ptr<CategoryNamePartSet> result(new CategoryNamePartSet);
    tr1::shared_ptr<const CategoryNamePartSet> cats(category_names());
    for (CategoryNamePartSet::ConstIterator c(cats->begin()), c_end(cats->end()) ;
            c != c_end ; ++c)
        if (has_package_named(*c + p))
            result->insert(*c);

    return result;
}

void
Repository::regenerate_cache() const
{
}

RepositorySetsInterface::~RepositorySetsInterface()
{
}

RepositorySyncableInterface::~RepositorySyncableInterface()
{
}

RepositoryUseInterface::~RepositoryUseInterface()
{
}

RepositoryWorldInterface::~RepositoryWorldInterface()
{
}

RepositoryEnvironmentVariableInterface::~RepositoryEnvironmentVariableInterface()
{
}

RepositoryMirrorsInterface::~RepositoryMirrorsInterface()
{
}

RepositoryProvidesInterface::~RepositoryProvidesInterface()
{
}

RepositoryVirtualsInterface::~RepositoryVirtualsInterface()
{
}

RepositoryDestinationInterface::~RepositoryDestinationInterface()
{
}

RepositoryEInterface::~RepositoryEInterface()
{
}

RepositoryHookInterface::~RepositoryHookInterface()
{
}

RepositoryMakeVirtualsInterface::~RepositoryMakeVirtualsInterface()
{
}

RepositoryQAInterface::~RepositoryQAInterface()
{
}

RepositoryManifestInterface::~RepositoryManifestInterface()
{
}

bool
RepositoryMirrorsInterface::is_mirror(const std::string & s) const
{
    return begin_mirrors(s) != end_mirrors(s);
}

bool
Repository::can_be_favourite_repository() const
{
    return true;
}

tr1::shared_ptr<const CategoryNamePartSet>
Repository::unimportant_category_names() const
{
    return make_shared_ptr(new CategoryNamePartSet);
}

