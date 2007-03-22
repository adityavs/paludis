/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#include "make_ebin_repository.hh"
#include <paludis/util/log.hh>
#include <paludis/util/collection_concrete.hh>
#include <paludis/util/tokeniser.hh>
#include <paludis/repositories/gentoo/portage_repository_exceptions.hh>
#include <paludis/environment.hh>

using namespace paludis;

std::tr1::shared_ptr<PortageRepository>
paludis::make_ebin_repository(
        Environment * const env,
        std::tr1::shared_ptr<const AssociativeCollection<std::string, std::string> > m)
{
    std::string repo_file(m->end() == m->find("repo_file") ? std::string("?") :
            m->find("repo_file")->second);

    Context context("When making ebin repository from repo_file '" + repo_file + "':");

    std::string location;
    if (m->end() == m->find("location") || ((location = m->find("location")->second)).empty())
        throw PortageRepositoryConfigurationError("Key 'location' not specified or empty");

    std::tr1::shared_ptr<const RepositoryName> master_repository_name;
    std::tr1::shared_ptr<const PortageRepository> master_repository;
    if (m->end() != m->find("master_repository") && ! m->find("master_repository")->second.empty())
    {
        Context context_local("When finding configuration information for master_repository '"
                + stringify(m->find("master_repository")->second) + "':");

        master_repository_name.reset(new RepositoryName(m->find("master_repository")->second));

        std::tr1::shared_ptr<const Repository> master_repository_uncasted(
                env->package_database()->fetch_repository(*master_repository_name));

        if (master_repository_uncasted->format() != "ebuild" && master_repository_uncasted->format() != "ebin")
            throw PortageRepositoryConfigurationError("Master repository format is '" +
                    stringify(master_repository_uncasted->format()) + "', not 'ebuild' or 'ebin'");

        master_repository = std::tr1::static_pointer_cast<const PortageRepository>(master_repository_uncasted);

        if (master_repository->params().master_repository)
            throw PortageRepositoryConfigurationError("Requested master repository has a master_repository of '" +
                    stringify(master_repository->params().master_repository->name()) + "', so it cannot "
                    "be used as a master repository");
    }

    std::tr1::shared_ptr<FSEntryCollection> profiles(new FSEntryCollection::Concrete);
    if (m->end() != m->find("profiles"))
        WhitespaceTokeniser::get_instance()->tokenise(m->find("profiles")->second,
                create_inserter<FSEntry>(std::back_inserter(*profiles)));

    if (profiles->empty())
    {
        if (master_repository)
            std::copy(master_repository->params().profiles->begin(),
                    master_repository->params().profiles->end(), profiles->inserter());
        else
            throw PortageRepositoryConfigurationError("No profiles have been specified");
    }

    std::tr1::shared_ptr<FSEntryCollection> eclassdirs(new FSEntryCollection::Concrete);

    std::string pkgdir;
    if (m->end() == m->find("pkgdir") || ((pkgdir = m->find("pkgdir")->second)).empty())
    {
        if (master_repository)
            pkgdir = stringify(master_repository->params().pkgdir);
        else
            pkgdir = location + "/packages";
    }
    if (pkgdir == "/var/empty" || pkgdir.empty())
        throw PortageRepositoryConfigurationError("Key 'pkgdir' not specified or empty");

    std::string setsdir;
    if (m->end() == m->find("setsdir") || ((setsdir = m->find("setsdir")->second)).empty())
        setsdir = location + "/sets";

    std::string securitydir;
    if (m->end() == m->find("securitydir") || ((securitydir = m->find("securitydir")->second)).empty())
        securitydir = location + "/metadata/glsa";

    std::string newsdir;
    if (m->end() == m->find("newsdir") || ((newsdir = m->find("newsdir")->second)).empty())
        newsdir = location + "/metadata/news";

    std::string names_cache;
    if (m->end() == m->find("names_cache") || ((names_cache = m->find("names_cache")->second)).empty())
    {
        Log::get_instance()->message(ll_warning, lc_no_context, "The names_cache key is not set in '"
                + repo_file + "'. You should read http://paludis.pioto.org/cachefiles.html and select an "
                "appropriate value.");
        names_cache = "/var/empty";
    }

    std::string sync;
    if (m->end() != m->find("sync"))
        sync = m->find("sync")->second;

    std::string sync_options;
    if (m->end() != m->find("sync_options"))
        sync_options = m->find("sync_options")->second;

    if (m->end() != m->find("sync_exclude"))
    {
        Log::get_instance()->message(ll_warning, lc_no_context, "The sync_exclude key in '"
                + repo_file + "' is deprecated in favour of sync_options = --exclude-from=");
        if (! sync_options.empty())
            sync_options += " ";
        sync_options += "--exclude-from='" + m->find("sync_exclude")->second + "'";
    }

    std::string buildroot;
    if (m->end() == m->find("buildroot") || ((buildroot = m->find("buildroot")->second)).empty())
        buildroot = "/var/tmp/paludis";

    return std::tr1::shared_ptr<PortageRepository>(new PortageRepository(PortageRepositoryParams::create()
                .entry_format("ebin")
                .environment(env)
                .location(location)
                .profiles(profiles)
                .cache(FSEntry("/var/empty"))
                .write_cache(FSEntry("/var/empty"))
                .names_cache(names_cache)
                .eclassdirs(std::tr1::shared_ptr<const FSEntryCollection>(new FSEntryCollection::Concrete))
                .distdir(FSEntry("/var/empty"))
                .pkgdir(pkgdir)
                .securitydir(securitydir)
                .setsdir(setsdir)
                .newsdir(newsdir)
                .sync(sync)
                .sync_options(sync_options)
                .master_repository(master_repository)
                .enable_destinations(true)
                .buildroot(buildroot)));
}

std::tr1::shared_ptr<Repository>
paludis::make_ebin_repository_wrapped(
        Environment * const env,
        std::tr1::shared_ptr<const AssociativeCollection<std::string, std::string> > m)
{
    return make_ebin_repository(env, m);
}


