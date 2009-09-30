/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ciaran McCreesh
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

#include <paludis/resolver/resolver_test.hh>
#include <paludis/resolver/constraint.hh>
#include <paludis/resolver/resolvent.hh>
#include <paludis/resolver/resolver.hh>
#include <paludis/resolver/resolution.hh>
#include <paludis/resolver/resolutions.hh>
#include <paludis/resolver/resolver_functions.hh>
#include <paludis/resolver/suggest_restart.hh>
#include <paludis/resolver/decision.hh>
#include <paludis/util/map.hh>
#include <paludis/util/wrapped_forward_iterator-impl.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/indirect_iterator-impl.hh>
#include <paludis/util/accept_visitor.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/make_named_values.hh>
#include <paludis/repositories/fake/fake_installed_repository.hh>
#include <paludis/repository_factory.hh>
#include <paludis/package_database.hh>
#include <paludis/user_dep_spec.hh>
#include <paludis/filter.hh>
#include <paludis/filtered_generator.hh>
#include <paludis/generator.hh>
#include <algorithm>
#include "config.h"

using namespace paludis;
using namespace paludis::resolver;
using namespace paludis::resolver::resolver_test;
using namespace test;

std::string
paludis::resolver::resolver_test::from_keys(const std::tr1::shared_ptr<const Map<std::string, std::string> > & m,
        const std::string & k)
{
    Map<std::string, std::string>::ConstIterator mm(m->find(k));
    if (m->end() == mm)
        return "";
    else
        return mm->second;
}

bool
paludis::resolver::resolver_test::care_about_dep_fn(const Resolvent &, const std::tr1::shared_ptr<const Resolution> &, const SanitisedDependency &)
{
    return true;
}

const std::tr1::shared_ptr<Constraints>
paludis::resolver::resolver_test::initial_constraints_for_fn(
        const InitialConstraints & initial_constraints,
        const Resolvent & resolvent)
{
    InitialConstraints::const_iterator i(initial_constraints.find(resolvent));
    if (i == initial_constraints.end())
        return make_shared_ptr(new Constraints);
    else
        return i->second;
}

std::tr1::shared_ptr<Resolvents>
paludis::resolver::resolver_test::get_resolvents_for_fn(const PackageDepSpec & spec,
        const std::tr1::shared_ptr<const SlotName> & slot,
        const std::tr1::shared_ptr<const Reason> &)
{
    std::tr1::shared_ptr<Resolvents> result(new Resolvents);
    result->push_back(Resolvent(spec, slot ? slot : make_shared_ptr(new const SlotName("0")), dt_install_to_slash));
    return result;
}

FilteredGenerator
paludis::resolver::resolver_test::make_destination_filtered_generator_fn(const Generator & g, const Resolvent & resolvent)
{
    switch (resolvent.destination_type())
    {
        case dt_install_to_slash:
            return g | filter::InstalledAtRoot(FSEntry("/"));

        case dt_create_binary:
            throw InternalError(PALUDIS_HERE, "no dt_create_binary yet");

        case last_dt:
            break;
    }

    throw InternalError(PALUDIS_HERE, "unhandled dt");
}

DestinationTypes
paludis::resolver::resolver_test::get_destination_types_for_fn(const PackageDepSpec &,
        const std::tr1::shared_ptr<const PackageID> &,
        const std::tr1::shared_ptr<const Reason> &)
{
    return DestinationTypes() + dt_install_to_slash;
}

namespace
{
    struct IsSuggestionVisitor
    {
        bool is_suggestion;

        IsSuggestionVisitor() :
            is_suggestion(true)
        {
        }

        void visit(const DependencyRequiredLabel &)
        {
            is_suggestion = false;
        }

        void visit(const DependencyRecommendedLabel &)
        {
            is_suggestion = false;
        }

        void visit(const DependencySuggestedLabel &)
        {
        }
    };

#ifdef ENABLE_VIRTUALS_REPOSITORY
    std::string virtuals_repo_keys(const std::string & k)
    {
        if (k == "format")
            return "virtuals";
        else if (k == "root")
            return "/";
        else
            return "";
    }

    std::string installed_virtuals_repo_keys(const std::string & k)
    {
        if (k == "format")
            return "installed_virtuals";
        else if (k == "root")
            return "/";
        else
            return "";
    }
#endif
}

bool
paludis::resolver::resolver_test::is_suggestion(const SanitisedDependency & dep)
{
    if (dep.active_dependency_labels()->suggest_labels()->empty())
        return false;

    IsSuggestionVisitor v;
    std::for_each(indirect_iterator(dep.active_dependency_labels()->suggest_labels()->begin()),
            indirect_iterator(dep.active_dependency_labels()->suggest_labels()->end()),
            accept_visitor(v));
    return v.is_suggestion;
}

bool
paludis::resolver::resolver_test::take_dependency_fn(
        const Resolvent &,
        const SanitisedDependency & dep,
        const std::tr1::shared_ptr<const Reason> &)
{
    return ! is_suggestion(dep);
}

UseExisting
paludis::resolver::resolver_test::get_use_existing_fn(
        const Resolvent &,
        const PackageDepSpec &,
        const std::tr1::shared_ptr<const Reason> &)
{
    return ue_never;
}

const std::tr1::shared_ptr<const Repository>
paludis::resolver::resolver_test::find_repository_for_fn(
        const Environment * const env,
        const Resolvent &,
        const std::tr1::shared_ptr<const Resolution> &,
        const ChangesToMakeDecision &)
{
    return env->package_database()->fetch_repository(RepositoryName("installed"));
}

ResolverTestCase::ResolverTestCase(const std::string & t, const std::string & s, const std::string & e,
        const std::string & l) :
    TestCase(s)
{
    std::tr1::shared_ptr<Map<std::string, std::string> > keys(new Map<std::string, std::string>);
    keys->insert("format", "exheres");
    keys->insert("names_cache", "/var/empty");
    keys->insert("location", stringify(FSEntry::cwd() / ("resolver_TEST_" + t + "_dir") / "repo"));
    keys->insert("profiles", stringify(FSEntry::cwd() / ("resolver_TEST_" + t + "_dir") / "repo/profiles/profile"));
    keys->insert("layout", l);
    keys->insert("eapi_when_unknown", e);
    keys->insert("eapi_when_unspecified", e);
    keys->insert("profile_eapi", e);
    keys->insert("distdir", stringify(FSEntry::cwd() / ("resolver_TEST_" + t + "_dir") / "distdir"));
    keys->insert("builddir", stringify(FSEntry::cwd() / ("resolver_TEST_" + t + "_dir") / "build"));
    repo = RepositoryFactory::get_instance()->create(&env,
            std::tr1::bind(from_keys, keys, std::tr1::placeholders::_1));
    env.package_database()->add_repository(1, repo);

    keys.reset(new Map<std::string, std::string>);
    keys->insert("format", "vdb");
    keys->insert("names_cache", "/var/empty");
    keys->insert("provides_cache", "/var/empty");
    keys->insert("location", stringify(FSEntry::cwd() / ("resolver_TEST_" + t + "_dir") / "installed"));
    keys->insert("builddir", stringify(FSEntry::cwd() / ("resolver_TEST_" + t + "_dir") / "build"));
    inst_repo = RepositoryFactory::get_instance()->create(&env,
            std::tr1::bind(from_keys, keys, std::tr1::placeholders::_1));
    env.package_database()->add_repository(1, inst_repo);

    fake_inst_repo.reset(new FakeInstalledRepository(&env, RepositoryName("fake-inst"), true, false));
    env.package_database()->add_repository(1, fake_inst_repo);

#ifdef ENABLE_VIRTUALS_REPOSITORY
    env.package_database()->add_repository(0, RepositoryFactory::get_instance()->create(&env, virtuals_repo_keys));
    env.package_database()->add_repository(0, RepositoryFactory::get_instance()->create(&env, installed_virtuals_repo_keys));
#endif
}

const std::tr1::shared_ptr<const ResolutionLists>
ResolverTestCase::get_resolutions(const PackageDepSpec & target)
{
    InitialConstraints initial_constraints;

    while (true)
    {
        try
        {
            Resolver resolver(&env, make_named_values<ResolverFunctions>(
                        value_for<n::care_about_dep_fn>(&care_about_dep_fn),
                        value_for<n::find_repository_for_fn>(std::tr1::bind(&find_repository_for_fn,
                                &env, std::tr1::placeholders::_1, std::tr1::placeholders::_2,
                                std::tr1::placeholders::_3)),
                        value_for<n::get_destination_types_for_fn>(&get_destination_types_for_fn),
                        value_for<n::get_initial_constraints_for_fn>(
                            std::tr1::bind(&initial_constraints_for_fn, std::tr1::ref(initial_constraints),
                                std::tr1::placeholders::_1)),
                        value_for<n::get_resolvents_for_fn>(&get_resolvents_for_fn),
                        value_for<n::get_use_existing_fn>(&get_use_existing_fn),
                        value_for<n::make_destination_filtered_generator_fn>(&make_destination_filtered_generator_fn),
                        value_for<n::take_dependency_fn>(&take_dependency_fn)
                        ));
            resolver.add_target(target);
            resolver.resolve();
            return resolver.resolution_lists();
        }
        catch (const SuggestRestart & e)
        {
            initial_constraints.insert(std::make_pair(e.resolvent(), make_shared_ptr(new Constraints))).first->second->add(e.suggested_preset());
        }
    }
}

const std::tr1::shared_ptr<const ResolutionLists>
ResolverTestCase::get_resolutions(const std::string & target)
{
    PackageDepSpec target_spec(parse_user_package_dep_spec(target, &env, UserPackageDepSpecOptions()));
    return get_resolutions(target_spec);
}

namespace
{
    struct ChosenIDVisitor
    {
        const std::tr1::shared_ptr<const PackageID> visit(const ChangesToMakeDecision & decision) const
        {
            return decision.origin_id();
        }

        const std::tr1::shared_ptr<const PackageID> visit(const ExistingNoChangeDecision & decision) const
        {
            return decision.existing_id();
        }

        const std::tr1::shared_ptr<const PackageID> visit(const NothingNoChangeDecision &) const
        {
            return make_null_shared_ptr();
        }

        const std::tr1::shared_ptr<const PackageID> visit(const UnableToMakeDecision &) const
        {
            return make_null_shared_ptr();
        }
    };

    struct KindNameVisitor
    {
        const std::string visit(const UnableToMakeDecision &) const
        {
            return "unable_to_make_decision";
        }

        const std::string visit(const NothingNoChangeDecision &) const
        {
            return "nothing_no_change";
        }

        const std::string visit(const ExistingNoChangeDecision &) const
        {
            return "existing_no_change";
        }

        const std::string visit(const ChangesToMakeDecision &) const
        {
            return "changes_to_make";
        }
    };
}

bool
ResolverTestCase::ResolutionListChecks::check_qpn(const QualifiedPackageName & q, const std::tr1::shared_ptr<const Resolution> & r)
{
    const std::tr1::shared_ptr<const PackageID> id(r->decision()->accept_returning<
            std::tr1::shared_ptr<const PackageID> >(ChosenIDVisitor()));
    if ((! r) || (! r->decision()) || ! id)
        return false;

    return id->name() == q;
}

bool
ResolverTestCase::ResolutionListChecks::check_kind(const std::string & k,
        const QualifiedPackageName & q, const std::tr1::shared_ptr<const Resolution> & r)
{
    if ((! r) || (! r->decision()))
        return false;

    return r->decision()->accept_returning<std::string>(KindNameVisitor()) == k && r->resolvent().package() == q;
}

std::string
ResolverTestCase::ResolutionListChecks::check_kind_msg(const std::string & k,
        const QualifiedPackageName & q, const std::tr1::shared_ptr<const Resolution> & r)
{
    if (! r)
        return "Expected " + stringify(k) + " " + stringify(q) + " but got finished";
    else if (! r->decision())
        return "Expected " + stringify(k) + " " + stringify(q) + " but got undecided for " + stringify(r->resolvent());
    else
        return "Expected " + stringify(k) + " " + stringify(q) + " but got "
            + r->decision()->accept_returning<std::string>(KindNameVisitor()) + " "
            + stringify(r->resolvent().package());
}

std::string
ResolverTestCase::ResolutionListChecks::check_generic_msg(const std::string & q, const std::tr1::shared_ptr<const Resolution> & r)
{
    if (! r)
        return "Expected " + stringify(q) + " but got finished";
    else if (! r->decision())
        return "Expected " + stringify(q) + " but got undecided for " + stringify(r->resolvent());
    else if (! r->decision()->accept_returning<std::tr1::shared_ptr<const PackageID> >(ChosenIDVisitor()))
        return "Expected " + stringify(q) + " but got decided nothing (kind "
            + stringify(r->decision()->accept_returning<std::string>(KindNameVisitor())) + ") for "
            + stringify(r->resolvent());
    else
        return "Expected " + stringify(q) + " but got " + stringify(
                r->decision()->accept_returning<std::tr1::shared_ptr<const PackageID> >(
                    ChosenIDVisitor())->name()) + " for "
            + stringify(r->resolvent());
}

std::string
ResolverTestCase::ResolutionListChecks::check_qpn_msg(const QualifiedPackageName & q, const std::tr1::shared_ptr<const Resolution> & r)
{
    return check_generic_msg(stringify(q), r);
}

bool
ResolverTestCase::ResolutionListChecks::check_finished(const std::tr1::shared_ptr<const Resolution> & r)
{
    return ! r;
}

std::string
ResolverTestCase::ResolutionListChecks::check_finished_msg(const std::tr1::shared_ptr<const Resolution> & r)
{
    return check_generic_msg("finished", r);
}

ResolverTestCase::ResolutionListChecks &
ResolverTestCase::ResolutionListChecks::qpn(const QualifiedPackageName & q)
{
    checks.push_back(std::make_pair(
                std::tr1::bind(&check_qpn, q, std::tr1::placeholders::_1),
                std::tr1::bind(&check_qpn_msg, q, std::tr1::placeholders::_1)
                ));
    return *this;
}

ResolverTestCase::ResolutionListChecks &
ResolverTestCase::ResolutionListChecks::kind(const std::string & k, const QualifiedPackageName & q)
{
    checks.push_back(std::make_pair(
                std::tr1::bind(&check_kind, k, q, std::tr1::placeholders::_1),
                std::tr1::bind(&check_kind_msg, k, q, std::tr1::placeholders::_1)
                ));
    return *this;
}

ResolverTestCase::ResolutionListChecks &
ResolverTestCase::ResolutionListChecks::finished()
{
    checks.push_back(std::make_pair(
                &check_finished,
                &check_finished_msg
                ));
    return *this;
}

void
ResolverTestCase::check_resolution_list(
        const std::tr1::shared_ptr<const Resolutions> & list, const ResolutionListChecks & checks)
{
    Resolutions::ConstIterator r(list->begin()), r_end(list->end());
    for (ResolutionListChecks::List::const_iterator c(checks.checks.begin()), c_end(checks.checks.end()) ;
            c != c_end ; ++c)
    {
        if (r == r_end)
            TEST_CHECK_MESSAGE(c->first(make_null_shared_ptr()), c->second(make_null_shared_ptr()));
        else
            TEST_CHECK_MESSAGE(c->first(*r), c->second(*r));
        ++r;
    }
}

void
ResolverTestCase::install(const std::string & c, const std::string & p, const std::string & v)
{
    fake_inst_repo->add_version(c, p, v);
}
