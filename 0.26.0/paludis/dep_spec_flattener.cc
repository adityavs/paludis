/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007, 2008 Ciaran McCreesh
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

#include <paludis/dep_spec_flattener.hh>
#include <paludis/dep_spec.hh>
#include <paludis/dep_tree.hh>
#include <paludis/environment.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/log.hh>
#include <paludis/util/stringify.hh>
#include <list>
#include <algorithm>
#include <set>

using namespace paludis;

namespace paludis
{
#ifndef PALUDIS_NO_DOUBLE_TEMPLATE
    template <>
#endif
    template <typename Heirarchy_, typename Item_>
    struct Implementation<DepSpecFlattener<Heirarchy_, Item_> >
    {
        const Environment * const env;

        std::list<tr1::shared_ptr<const Item_> > specs;

        Implementation(const Environment * const e) :
            env(e)
        {
        }
    };
}

template <typename Heirarchy_, typename Item_>
DepSpecFlattener<Heirarchy_, Item_>::DepSpecFlattener(
        const Environment * const env) :
    PrivateImplementationPattern<DepSpecFlattener<Heirarchy_, Item_> >(
            new Implementation<DepSpecFlattener<Heirarchy_, Item_> >(env)),
    _imp(PrivateImplementationPattern<DepSpecFlattener<Heirarchy_, Item_> >::_imp)
{
}

template <typename Heirarchy_, typename Item_>
DepSpecFlattener<Heirarchy_, Item_>::~DepSpecFlattener()
{
}

template <typename Heirarchy_, typename Item_>
typename DepSpecFlattener<Heirarchy_, Item_>::ConstIterator
DepSpecFlattener<Heirarchy_, Item_>::begin() const
{
    return ConstIterator(_imp->specs.begin());
}

template <typename Heirarchy_, typename Item_>
typename DepSpecFlattener<Heirarchy_, Item_>::ConstIterator
DepSpecFlattener<Heirarchy_, Item_>::end() const
{
    return ConstIterator(_imp->specs.end());
}

template <typename Heirarchy_, typename Item_>
void
dep_spec_flattener_internals::VisitConditionalDepSpec<Heirarchy_, Item_, true>::visit_sequence(const ConditionalDepSpec & u,
        typename Heirarchy_::ConstSequenceIterator cur,
        typename Heirarchy_::ConstSequenceIterator e)
{
    DepSpecFlattener<Heirarchy_, Item_> * const f(static_cast<DepSpecFlattener<Heirarchy_, Item_> *>(this));

    if (u.condition_met())
        std::for_each(cur, e, accept_visitor(*f));
}

namespace paludis
{
#ifndef PALUDIS_NO_DOUBLE_TEMPLATE
    template <>
#endif
    template <typename Heirarchy_, typename Item_>
    struct Implementation<dep_spec_flattener_internals::VisitNamedSetDepSpec<Heirarchy_, Item_, true> >
    {
        std::set<SetName> recursing_sets;
    };
}

template <typename Heirarchy_, typename Item_>
dep_spec_flattener_internals::VisitNamedSetDepSpec<Heirarchy_, Item_, true>::VisitNamedSetDepSpec() :
    PrivateImplementationPattern<dep_spec_flattener_internals::VisitNamedSetDepSpec<Heirarchy_, Item_, true> >(
            new Implementation<dep_spec_flattener_internals::VisitNamedSetDepSpec<Heirarchy_, Item_, true> >)
{
}

template <typename Heirarchy_, typename Item_>
dep_spec_flattener_internals::VisitNamedSetDepSpec<Heirarchy_, Item_, true>::~VisitNamedSetDepSpec()
{
}

template <typename Heirarchy_, typename Item_>
void
dep_spec_flattener_internals::VisitNamedSetDepSpec<Heirarchy_, Item_, true>::visit_leaf(const NamedSetDepSpec & s)
{
    DepSpecFlattener<Heirarchy_, Item_> * const f(static_cast<DepSpecFlattener<Heirarchy_, Item_> *>(this));

    Context context("When expanding named set '" + stringify(s) + "':");

    tr1::shared_ptr<const SetSpecTree::ConstItem> set(f->_imp->env->set(s.name()));

    if (! set)
    {
        Log::get_instance()->message("flattener.unknown_set", ll_warning, lc_context) << "Unknown set '" << s.name() << "'";
        return;
    }

    if (! this->_imp->recursing_sets.insert(s.name()).second)
    {
        Log::get_instance()->message("flattener.recursive_set", ll_warning, lc_context) << "Recursively defined set '" << s.name() << "'";
        return;
    }

    set->accept(*f);

    this->_imp->recursing_sets.erase(s.name());
}

template <typename Heirarchy_, typename Item_>
void
DepSpecFlattener<Heirarchy_, Item_>::visit_leaf(const Item_ & p)
{
    _imp->specs.push_back(tr1::static_pointer_cast<const Item_>(p.clone()));
}

template class DepSpecFlattener<ProvideSpecTree, PackageDepSpec>;
template class DepSpecFlattener<SetSpecTree, PackageDepSpec>;
template class DepSpecFlattener<RestrictSpecTree, PlainTextDepSpec>;
template class DepSpecFlattener<SimpleURISpecTree, SimpleURIDepSpec>;

