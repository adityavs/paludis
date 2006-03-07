/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006 Ciaran McCreesh <ciaranm@gentoo.org>
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

#include <paludis/dep_atom.hh>
#include <paludis/dep_atom_flattener.hh>
#include <paludis/dep_list.hh>
#include <paludis/dep_parser.hh>
#include <paludis/match_package.hh>
#include <paludis/util/container_entry.hh>
#include <paludis/util/filter_insert_iterator.hh>
#include <paludis/util/indirect_iterator.hh>
#include <paludis/util/iterator_utilities.hh>
#include <paludis/util/join.hh>
#include <paludis/util/log.hh>
#include <paludis/util/save.hh>
#include <paludis/util/stringify.hh>

#include <algorithm>
#include <functional>
#include <deque>
#include <vector>

using namespace paludis;

std::ostream &
paludis::operator<< (std::ostream & s, const DepListEntry & e)
{
    s << e.get<dle_name>() << "-" << e.get<dle_version>() << ":"
        << e.get<dle_metadata>()->get(vmk_slot) << "::" << e.get<dle_repository>();
    return s;
}

DepListError::DepListError(const std::string & m) throw () :
    Exception(m)
{
}

DepListStackTooDeepError::DepListStackTooDeepError(int level) throw () :
    DepListError("DepList stack too deep (" + stringify(level) + " entries)")
{
}

NoResolvableOptionError::NoResolvableOptionError() throw () :
    DepListError("No resolvable || ( ) option")
{
}

template <typename I_>
NoResolvableOptionError::NoResolvableOptionError(I_ i, I_ end) throw () :
    DepListError("No resolvable || ( ) option." + (i == end ?
                std::string("") :  " Failure messages are '" + join(i, end, "', '") + "'"))
{
}

AllMaskedError::AllMaskedError(const std::string & query) throw () :
    DepListError("Error searching for '" + query + "': no available versions")
{
}

BlockError::BlockError(const std::string & msg) throw () :
    DepListError("Block: " + msg)
{
}

namespace paludis
{
    /**
     * Implementation data for DepList.
     */
    template<>
    struct Implementation<DepList> :
        InstantiationPolicy<Implementation<DepList>, instantiation_method::NonCopyableTag>,
        InternalCounted<Implementation<DepList> >
    {
        ///\name Provided data
        ///{
        const Environment * const environment;
        ///}

        ///\name Generated data
        ///{
        std::list<DepListEntry> merge_list;
        std::list<DepListEntry>::iterator merge_list_insert_pos;
        bool check_existing_only;
        bool match_found;
        const DepListEntry * current_package;
        ///}

        ///\name Settings
        ///{
        DepListRdependOption rdepend_post;
        bool recursive_deps;
        bool drop_circular;
        bool drop_self_circular;
        bool dont_ignore_patch_dep;
        bool drop_all;
        bool ignore_installed;
        ///}

        ///\name Stack
        ///{
        int stack_depth;
        int max_stack_depth;
        ///}

        /// Constructor.
        Implementation(const Environment * const e) :
            environment(e),
            check_existing_only(false),
            match_found(false),
            current_package(0),
            rdepend_post(dlro_as_needed),
            recursive_deps(true),
            drop_circular(false),
            drop_self_circular(false),
            dont_ignore_patch_dep(false),
            drop_all(false),
            ignore_installed(false),
            stack_depth(0),
            max_stack_depth(100)
        {
        }
    };
}

DepList::DepList(const Environment * const e) :
    PrivateImplementationPattern<DepList>(new Implementation<DepList>(e))
{
}

DepList::~DepList()
{
}

void
DepList::add(DepAtom::ConstPointer atom)
{
    std::list<DepListEntry> save_merge_list(_imp->merge_list.begin(),
            _imp->merge_list.end());

    _imp->merge_list_insert_pos = _imp->merge_list.end();
    _add(atom);

    try
    {
        std::list<DepListEntry>::iterator i(_imp->merge_list.begin());
        _imp->merge_list_insert_pos = _imp->merge_list.end();
        while (i != _imp->merge_list.end())
        {
            if (! i->get<dle_has_predeps>() && ! _imp->drop_all)
                throw InternalError(PALUDIS_HERE, "dle_has_predeps not set for " + stringify(*i));

            else if (! i->get<dle_has_trypredeps>() && ! _imp->drop_all)
            {
                Save<const DepListEntry *> save_current_package(
                        &_imp->current_package, &*i);
                _add_in_role(DepParser::parse(
                            _imp->environment->package_database()->fetch_metadata(
                                PackageDatabaseEntry(i->get<dle_name>(), i->get<dle_version>(),
                                    i->get<dle_repository>()))->get(vmk_rdepend)), "RDEPEND");
                i->set<dle_has_trypredeps>(true);
            }

            else if (! i->get<dle_has_postdeps>() && ! _imp->drop_all)
            {
                Save<const DepListEntry *> save_current_package(
                        &_imp->current_package, &*i);
                _add_in_role(DepParser::parse(
                            _imp->environment->package_database()->fetch_metadata(
                                PackageDatabaseEntry(i->get<dle_name>(), i->get<dle_version>(),
                                    i->get<dle_repository>()))->get(vmk_pdepend)), "PDEPEND");
                i->set<dle_has_postdeps>(true);
            }
            else
                ++i;
        }
    }
    catch (...)
    {
        _imp->merge_list.swap(save_merge_list);
        throw;
    }
}

void
DepList::_add_raw(const DepAtom * const atom)
{
#if 0
    /// \bug VV this is debug code. remove it once we're sure this works
    std::list<DepListEntry> backup_merge_list(_imp->merge_list.begin(),
            _imp->merge_list.end());
#endif

    /* keep track of stack depth */
    Save<int> old_stack_depth(&_imp->stack_depth,
            _imp->stack_depth + 1);
    if (_imp->stack_depth > _imp->max_stack_depth)
        throw DepListStackTooDeepError(_imp->stack_depth);

    /* we need to make sure that merge_list doesn't get h0rked in the
     * event of a failure. */
    bool merge_list_was_empty(_imp->merge_list.empty()), irange_begin_is_begin(false);
    std::list<DepListEntry>::iterator save_last, save_first, save_irange_begin, save_irange_end;
    if (! merge_list_was_empty)
    {
        save_first = _imp->merge_list.begin();
        save_last = previous(_imp->merge_list.end());

        save_irange_end = _imp->merge_list_insert_pos;
        if (_imp->merge_list_insert_pos == _imp->merge_list.begin())
            irange_begin_is_begin = true;
        else
            save_irange_begin = previous(_imp->merge_list_insert_pos);
    }

    try
    {
        atom->accept(this);
    }
    catch (const InternalError &)
    {
        throw;
    }
    catch (...)
    {
        if (merge_list_was_empty)
            _imp->merge_list.clear();
        else
        {
            _imp->merge_list.erase(next(save_last), _imp->merge_list.end());
            _imp->merge_list.erase(_imp->merge_list.begin(), save_first);
            _imp->merge_list.erase(
                    irange_begin_is_begin ? _imp->merge_list.begin() : next(save_irange_begin),
                    save_irange_end);
        }

#if 0
        /// \bug VV this is debug code. remove it once we're sure this works
        if (backup_merge_list != _imp->merge_list)
        {
            Log::get_instance()->message(ll_warning, "Old merge_list: " + join(backup_merge_list.begin(),
                        backup_merge_list.end(), " -> "));
            Log::get_instance()->message(ll_warning, "New merge_list: " + join(_imp->merge_list.begin(),
                        _imp->merge_list.end(), " -> "));
            throw InternalError(PALUDIS_HERE, "merge list restore failed");
        }
#endif
        throw;
    }
}

void
DepList::_add_in_role_raw(const DepAtom * const atom, const std::string & role)
{
    Context context("When adding " + role + ":");
    _add_raw(atom);
}

DepList::Iterator
DepList::begin() const
{
    return _imp->merge_list.begin();
}

DepList::Iterator
DepList::end() const
{
    return _imp->merge_list.end();
}

void
DepList::visit(const AllDepAtom * const v)
{
    std::for_each(v->begin(), v->end(), accept_visitor(
                static_cast<DepAtomVisitorTypes::ConstVisitor *>(this)));
}

#ifndef DOXYGEN
struct DepListEntryMatcher :
    public std::unary_function<bool, const DepListEntry &>
{
    const PackageDatabase * const db;
    const PackageDepAtom & atom;

    DepListEntryMatcher(const PackageDatabase * const d, const PackageDepAtom & p) :
        db(d),
        atom(p)
    {
    }

    bool operator() (const DepListEntry & e) const
    {
        return match_package(db, atom, e);
    }
};
#endif

void
DepList::visit(const PackageDepAtom * const p)
{
    Context context("When resolving package dependency '" + stringify(*p) + "':");

    /// \todo check installed db

    /* are we already there? */
    std::list<DepListEntry>::iterator i;
    if (_imp->merge_list.end() != ((i = std::find_if(
                        _imp->merge_list.begin(),
                        _imp->merge_list.end(),
                        DepListEntryMatcher(
                            _imp->environment->package_database().raw_pointer(), *p)))))
    {
        /* what's our status? */
        if (! i->get<dle_has_predeps>())
        {
            if (_imp->drop_circular)
                return;
            else if (_imp->merge_list_insert_pos == i && (
                        _imp->drop_self_circular || (
                            ! _imp->dont_ignore_patch_dep && p->package() ==
                            QualifiedPackageName("sys-devel/patch"))))
                return;
            else
                throw CircularDependencyError(i, next(i));
        }

        return;
    }

    /* are we allowed to install things? */
    if (_imp->check_existing_only)
    {
        _imp->match_found = false;
        return;
    }

    /* find the matching package */
    const PackageDatabaseEntry * match(0);
    VersionMetadata::ConstPointer metadata(0);
    PackageDatabaseEntryCollection::Pointer matches(0);

    matches = _imp->environment->package_database()->query(p);
    for (PackageDatabaseEntryCollection::ReverseIterator e(matches->rbegin()),
            e_end(matches->rend()) ; e != e_end ; ++e)
    {
        /* check masks */
        if (_imp->environment->mask_reasons(*e).any())
            continue;

        metadata = _imp->environment->package_database()->fetch_metadata(*e);
        match = &*e;
        break;
    }

    if (! match)
        throw AllMaskedError(stringify(*p));

    /* make merge entry */
    std::list<DepListEntry>::iterator merge_entry(
            _imp->merge_list.insert(_imp->merge_list_insert_pos,
                DepListEntry(match->get<pde_name>(), match->get<pde_version>(),
                    metadata, match->get<pde_repository>(),
                    false, false, false)));

    /* if we provide things, also insert them. */
    if (! metadata->get(vmk_provide).empty())
    {
        DepAtom::ConstPointer provide(DepParser::parse(metadata->get(vmk_provide),
                DepParserPolicy<PackageDepAtom, false>::get_instance()));

        CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag> e(0);

        if (_imp->current_package)
            e = CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag>(
                    new PackageDatabaseEntry(
                        _imp->current_package->get<dle_name>(),
                        _imp->current_package->get<dle_version>(),
                        _imp->current_package->get<dle_repository>()));

        DepAtomFlattener f(_imp->environment, e.raw_pointer(), provide);

        for (DepAtomFlattener::Iterator p(f.begin()), p_end(f.end()) ; p != p_end ; ++p)
        {
            PackageDepAtom pp(QualifiedPackageName((*p)->text()));
            if (_imp->merge_list.end() != std::find_if(
                        _imp->merge_list.begin(), _imp->merge_list.end(),
                        DepListEntryMatcher(
                            _imp->environment->package_database().raw_pointer(), pp)))
                continue;

            VersionMetadata::Pointer p_metadata(new VersionMetadata);
            p_metadata->set(vmk_slot, merge_entry->get<dle_metadata>()->get(vmk_slot));
            p_metadata->set(vmk_virtual, stringify(merge_entry->get<dle_name>()));
            _imp->merge_list.insert(next(merge_entry),
                    DepListEntry(pp.package(), merge_entry->get<dle_version>(),
                        p_metadata, merge_entry->get<dle_repository>(), true, true, true));
        }
    }


    Save<std::list<DepListEntry>::iterator> old_merge_list_insert_pos(
            &_imp->merge_list_insert_pos, merge_entry);

    context.change_context("When resolving package dependency '" + stringify(*p) +
            "' -> '" + stringify(*merge_entry) + "':");

    /* new current package */
    Save<const DepListEntry *> old_current_package(&_imp->current_package,
            &*merge_entry);

    /* merge depends */
    if ((! merge_entry->get<dle_has_predeps>()) && ! (_imp->drop_all))
    {
        _add_in_role(DepParser::parse(metadata->get(vmk_depend)), "DEPEND");
        merge_entry->set<dle_has_predeps>(true);
    }

    /* merge rdepends */
    if (! merge_entry->get<dle_has_trypredeps>() && dlro_always != _imp->rdepend_post
            && ! _imp->drop_all)
    {
        try
        {
            _add_in_role(DepParser::parse(metadata->get(vmk_rdepend)), "RDEPEND");
            merge_entry->set<dle_has_trypredeps>(true);
        }
        catch (const CircularDependencyError &)
        {
            if (dlro_never == _imp->rdepend_post)
                throw;
        }
    }
}

void
DepList::visit(const UseDepAtom * const u)
{
    CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag> e(0);

    if (_imp->current_package)
        e = CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag>(
                new PackageDatabaseEntry(
                    _imp->current_package->get<dle_name>(),
                    _imp->current_package->get<dle_version>(),
                    _imp->current_package->get<dle_repository>()));

    if (_imp->environment->query_use(u->flag(), e.raw_pointer()) ^ u->inverse())
        std::for_each(u->begin(), u->end(), std::bind1st(std::mem_fun(&DepList::_add), this));
}

#ifndef DOXYGEN
struct IsViable :
    public std::unary_function<bool, DepAtom::ConstPointer>
{
    const Implementation<DepList> & _impl;
    CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag> e;

    IsViable(const Implementation<DepList> & impl) :
        _impl(impl),
        e(0)
    {
        if (_impl.current_package)
            e = CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag>(
                    new PackageDatabaseEntry(
                        _impl.current_package->get<dle_name>(),
                        _impl.current_package->get<dle_version>(),
                        _impl.current_package->get<dle_repository>()));
    }

    bool operator() (DepAtom::ConstPointer a)
    {
        const UseDepAtom * const u(a->as_use_dep_atom());
        if (0 != u)
            return _impl.environment->query_use(u->flag(), e.raw_pointer()) ^ u->inverse();
        else
            return true;
    }
};
#endif

void
DepList::visit(const AnyDepAtom * const a)
{
    /* try to resolve each of our children in return. note the annoying
     * special case for use? () flags:
     *
     *   || ( ) -> nothing
     *   || ( off1? ( blah1 ) off2? ( blah2 ) blah3 ) -> blah3
     *   || ( off1? ( blah1 ) off2? ( blah2 ) ) -> nothing
     *   || ( ( off1? ( blah1 ) ) blah2 ) -> nothing
     *
     * we handle this by keeping a list of 'viable children'.
     */

    std::list<DepAtom::ConstPointer> viable_children;
    std::copy(a->begin(), a->end(), filter_inserter(
                std::back_inserter(viable_children), IsViable(*_imp)));

    if (viable_children.empty())
    {
        if (_imp->current_package)
            Log::get_instance()->message(ll_qa, "Package '" + stringify(*_imp->current_package)
                    + "' has suspicious || ( ) block that resolves to empty");
        return;
    }

    bool found(false);
    for (std::list<DepAtom::ConstPointer>::iterator i(viable_children.begin()),
            i_end(viable_children.end()) ; i != i_end ; ++i)
    {
        Save<bool> save_check(&_imp->check_existing_only, true);
        Save<bool> save_match(&_imp->match_found, true);
        _add(*i);
        if ((found = _imp->match_found))
            break;
    }
    if (found)
        return;

    if (_imp->check_existing_only)
    {
        _imp->match_found = false;
        return;
    }

    /* try to merge each of our viable children in turn. */
    std::deque<std::string> errors;
    for (CompositeDepAtom::Iterator i(viable_children.begin()), i_end(viable_children.end()) ;
            i != i_end ; ++i)
    {
        try
        {
            _add(*i);
            return;
        }
        catch (const DepListStackTooDeepError &)
        {
            /* don't work around a stack too deep error. our item may be
             * resolvable with a deeper stack. */
            throw;
        }
        catch (const DepListError & e)
        {
            errors.push_back(e.message() + " (" + e.what() + ")");
        }
    }

    /* no match */
    throw NoResolvableOptionError(errors.begin(), errors.end());
}

void
DepList::visit(const BlockDepAtom * const d)
{
    Context context("When checking block '!" + stringify(*(d->blocked_atom())) + "':");

    /* special case: the provider of virtual/blah can DEPEND upon !virtual/blah. */
    /// \bug This may have issues if a virtual is provided by a virtual...
    //
    /* special case: foo/bar can DEPEND upon !foo/bar. */

    /* will we be installed by this point? */
    std::list<DepListEntry>::iterator m(_imp->merge_list.begin());
    while (m != _imp->merge_list.end())
    {
        if (_imp->merge_list.end() != ((m = std::find_if(m, _imp->merge_list.end(),
                    DepListEntryMatcher(_imp->environment->package_database().raw_pointer(),
                        *(d->blocked_atom()))))))
        {
            if (! _imp->current_package)
                throw BlockError("'" + stringify(*(d->blocked_atom())) + "' blocked by pending package '"
                        + stringify(*m) + " (no current package)");

            if (*_imp->current_package == *m)
            {
                Log::get_instance()->message(ll_qa, "Package '" + stringify(*_imp->current_package)
                        + "' has suspicious block upon '!" + stringify(*d->blocked_atom()) + "'");
                ++m;
                continue;
            }

            DepAtom::ConstPointer provide(DepParser::parse(
                        _imp->current_package->get<dle_metadata>()->get(vmk_provide),
                        DepParserPolicy<PackageDepAtom, false>::get_instance()));

            CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag> e(0);

            if (_imp->current_package)
                e = CountedPtr<PackageDatabaseEntry, count_policy::ExternalCountTag>(
                        new PackageDatabaseEntry(
                            _imp->current_package->get<dle_name>(),
                            _imp->current_package->get<dle_version>(),
                            _imp->current_package->get<dle_repository>()));

            DepAtomFlattener f(_imp->environment, e.raw_pointer(), provide);

            bool skip(false);
            for (IndirectIterator<DepAtomFlattener::Iterator, const StringDepAtom> i(f.begin()),
                    i_end(f.end()) ; i != i_end ; ++i)
                if (QualifiedPackageName(i->text()) == d->blocked_atom()->package())
                {
                    skip = true;
                    break;
                }

            if (! skip)
                throw BlockError("'" + stringify(*(d->blocked_atom())) + "' blocked by pending package '"
                        + stringify(*m));

            ++m;
        }
    }
}

void
DepList::set_rdepend_post(const DepListRdependOption value)
{
    _imp->rdepend_post = value;
}

void
DepList::set_drop_circular(const bool value)
{
    _imp->drop_circular = value;
}

void
DepList::set_drop_self_circular(const bool value)
{
    _imp->drop_self_circular = value;
}

void
DepList::set_dont_ignore_patch_dep(const bool value)
{
    _imp->dont_ignore_patch_dep = value;
}

void
DepList::set_drop_all(const bool value)
{
    _imp->drop_all = value;
}

void
DepList::set_ignore_installed(const bool value)
{
    _imp->ignore_installed = value;
}

void
DepList::set_recursive_deps(const bool value)
{
    _imp->recursive_deps = value;
}

void
DepList::set_max_stack_depth(const int value)
{
    _imp->max_stack_depth = value;
}

void
DepList::visit(const PlainTextDepAtom * const t)
{
    throw InternalError(PALUDIS_HERE, "Got unexpected PlainTextDepAtom '" + t->text() + "'");
}

