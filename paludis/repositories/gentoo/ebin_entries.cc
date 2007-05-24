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

#include "ebin_entries.hh"
#include <paludis/repositories/gentoo/ebin.hh>
#include <paludis/repositories/gentoo/portage_repository.hh>
#include <paludis/config_file.hh>
#include <paludis/portage_dep_parser.hh>
#include <paludis/dep_spec_flattener.hh>
#include <paludis/environment.hh>
#include <paludis/util/strip.hh>
#include <paludis/util/log.hh>
#include <paludis/util/system.hh>
#include <paludis/util/join.hh>
#include <paludis/util/is_file_with_extension.hh>
#include <paludis/dep_spec_pretty_printer.hh>
#include <set>
#include <fstream>

using namespace paludis;

namespace paludis
{
    template<>
    struct Implementation<EbinEntries>
    {
        const Environment * const environment;
        PortageRepository * const portage_repository;
        const PortageRepositoryParams params;

        Implementation(const Environment * const e, PortageRepository * const p,
                const PortageRepositoryParams & k) :
            environment(e),
            portage_repository(p),
            params(k)
        {
        }
    };
}

EbinEntries::EbinEntries(
        const Environment * const e, PortageRepository * const p, const PortageRepositoryParams & k) :
    PrivateImplementationPattern<EbinEntries>(new Implementation<EbinEntries>(e, p, k))
{
}

EbinEntries::~EbinEntries()
{
}

tr1::shared_ptr<VersionMetadata>
EbinEntries::generate_version_metadata(const QualifiedPackageName & q,
        const VersionSpec & v) const
{
    Context context("When generating version metadata for '" + stringify(q) + "-" + stringify(v) + "':");

    tr1::shared_ptr<EbinVersionMetadata> result(new EbinVersionMetadata(SlotName("unset")));

    KeyValueConfigFile f(_imp->params.location / stringify(q.category) /
            stringify(q.package) / (stringify(q.package) + "-" + stringify(v) + ".ebin"),
            KeyValueConfigFileOptions() + kvcfo_disallow_continuations + kvcfo_disallow_comments +
            kvcfo_disallow_space_around_equals + kvcfo_disallow_source);

    result->set_run_depend(f.get("RDEPEND"));
    result->set_post_depend(f.get("PDEPEND"));
    result->set_suggested_depend(f.get("SDEPEND"));

    result->set_license(f.get("LICENSE"));

    result->source.reset();
    result->binary.reset();

    result->slot = SlotName(f.get("SLOT"));
    result->set_homepage(f.get("HOMEPAGE"));
    result->description = f.get("DESCRIPTION");
    result->eapi = EAPIData::get_instance()->eapi_from_string(f.get("EAPI"));

    result->set_provide(f.get("PROVIDE"));
    result->set_src_uri(f.get("SRC_URI"));
    result->set_restrictions(f.get("RESTRICT"));
    result->set_keywords(f.get("KEYWORDS"));
    result->set_iuse(f.get("IUSE"));
    result->set_inherited(f.get("INHERITED"));

    result->set_bin_uri(f.get("BIN_URI"));

    return result;
}

namespace
{
    FSEntry
    get_root(tr1::shared_ptr<const DestinationsCollection> destinations)
    {
        if (destinations)
            for (DestinationsCollection::Iterator d(destinations->begin()), d_end(destinations->end()) ;
                    d != d_end ; ++d)
                if ((*d)->installed_interface)
                    return (*d)->installed_interface->root();

        return FSEntry("/");
    }
}

void
EbinEntries::install(const QualifiedPackageName & q, const VersionSpec & v,
        const InstallOptions & o, tr1::shared_ptr<const PortageRepositoryProfile>) const
{
    if (! _imp->portage_repository->has_version(q, v))
        throw PackageInstallActionError("Can't install '" + stringify(q) + "-"
                + stringify(v) + "' since has_version failed");

    tr1::shared_ptr<const VersionMetadata> metadata(_imp->portage_repository->version_metadata(q, v));
    PackageDatabaseEntry e(q, v, _imp->portage_repository->name());

    std::string binaries, flat_bin_uri;
    {
        std::set<std::string> already_in_binaries;

        /* make B and FLAT_BIN_URI */
        tr1::shared_ptr<const DepSpec> b_spec(metadata->ebin_interface->bin_uri());
        DepSpecFlattener f(_imp->params.environment, &e, b_spec);

        for (DepSpecFlattener::Iterator ff(f.begin()), ff_end(f.end()) ; ff != ff_end ; ++ff)
        {
            std::string::size_type pos((*ff)->text().rfind('/'));
            if (std::string::npos == pos)
            {
                if (already_in_binaries.end() == already_in_binaries.find((*ff)->text()))
                {
                    binaries.append((*ff)->text());
                    already_in_binaries.insert((*ff)->text());
                }
            }
            else
            {
                if (already_in_binaries.end() == already_in_binaries.find((*ff)->text().substr(pos + 1)))
                {
                    binaries.append((*ff)->text().substr(pos + 1));
                    already_in_binaries.insert((*ff)->text().substr(pos + 1));
                }
            }
            binaries.append(" ");

            /* add * mirror entries */
            tr1::shared_ptr<const MirrorsCollection> star_mirrors(_imp->params.environment->mirrors("*"));
            for (MirrorsCollection::Iterator m(star_mirrors->begin()), m_end(star_mirrors->end()) ; m != m_end ; ++m)
                flat_bin_uri.append(*m + "/" + (*ff)->text().substr(pos + 1) + " ");

            if (0 == (*ff)->text().compare(0, 9, "mirror://"))
            {
                std::string mirror((*ff)->text().substr(9));
                std::string::size_type spos(mirror.find('/'));

                if (std::string::npos == spos)
                    throw PackageInstallActionError("Can't install '" + stringify(q) + "-"
                            + stringify(v) + "' since BIN_URI is broken");

                tr1::shared_ptr<const MirrorsCollection> mirrors(_imp->params.environment->mirrors(mirror.substr(0, spos)));
                if (! _imp->portage_repository->is_mirror(mirror.substr(0, spos)) &&
                        mirrors->empty())
                    throw PackageInstallActionError("Can't install '" + stringify(q) + "-"
                            + stringify(v) + "' since BIN_URI references unknown mirror:// '" +
                            mirror.substr(0, spos) + "'");

                for (MirrorsCollection::Iterator m(mirrors->begin()), m_end(mirrors->end()) ; m != m_end ; ++m)
                    flat_bin_uri.append(*m + "/" + mirror.substr(spos + 1) + " ");

                for (RepositoryMirrorsInterface::MirrorsIterator
                        m(_imp->portage_repository->begin_mirrors(mirror.substr(0, spos))),
                        m_end(_imp->portage_repository->end_mirrors(mirror.substr(0, spos))) ;
                        m != m_end ; ++m)
                    flat_bin_uri.append(m->second + "/" + mirror.substr(spos + 1) + " ");
            }
            else
                flat_bin_uri.append((*ff)->text());
            flat_bin_uri.append(" ");

            /* add mirror://gentoo/ entries */
            std::string master_mirror(strip_trailing_string(stringify(_imp->portage_repository->name()), "x-"));
            if (_imp->portage_repository->is_mirror(master_mirror))
            {
                tr1::shared_ptr<const MirrorsCollection> repo_mirrors(_imp->params.environment->mirrors(master_mirror));
                for (MirrorsCollection::Iterator m(repo_mirrors->begin()), m_end(repo_mirrors->end()) ; m != m_end ; ++m)
                    flat_bin_uri.append(*m + "/" + (*ff)->text().substr(pos + 1) + " ");

                for (RepositoryMirrorsInterface::MirrorsIterator
                        m(_imp->portage_repository->begin_mirrors(master_mirror)),
                        m_end(_imp->portage_repository->end_mirrors(master_mirror)) ;
                        m != m_end ; ++m)
                    flat_bin_uri.append(m->second + "/" + (*ff)->text().substr(pos + 1) + " ");
            }
        }
    }

    EbinCommandParams command_params(EbinCommandParams::create()
            .environment(_imp->params.environment)
            .db_entry(&e)
            .portdir(_imp->params.master_repository ? _imp->params.master_repository->params().location :
                _imp->params.location)
            .pkgdir(_imp->params.pkgdir)
            .buildroot(_imp->params.buildroot));

    bool fetch_userpriv_ok(_imp->environment->reduced_gid() != getgid());
    if (fetch_userpriv_ok)
    {
        FSEntry f(_imp->params.pkgdir);
        Context c("When checking permissions on '" + stringify(f) + "' for userpriv:");

        if (f.exists())
        {
            if (f.group() != _imp->environment->reduced_gid())
            {
                Log::get_instance()->message(ll_warning, lc_context, "Directory '" +
                        stringify(f) + "' owned by group '" +
                        stringify(get_group_name(f.group())) + "', not '" +
                        stringify(get_group_name(_imp->environment->reduced_gid())) +
                        "', so cannot enable userpriv");
                fetch_userpriv_ok = false;
            }
            else if (! f.has_permission(fs_ug_group, fs_perm_write))
            {
                Log::get_instance()->message(ll_warning, lc_context, "Directory '" +
                        stringify(f) + "' does not group write permission," +
                        "cannot enable userpriv");
                fetch_userpriv_ok = false;
            }
        }
    }

    EbinFetchCommand fetch_cmd(command_params,
            EbinFetchCommandParams::create()
            .b(binaries)
            .flat_bin_uri(flat_bin_uri)
            .root(stringify(get_root(o.destinations)))
            .safe_resume(o.safe_resume)
            .userpriv(false));

    fetch_cmd();

    if (o.fetch_only)
        return;

    if (! o.destinations)
        throw PackageInstallActionError("Can't install '" + stringify(q) + "-"
                + stringify(v) + "' because no destination was provided");

    EbinInstallCommandParams install_params(
            EbinInstallCommandParams::create()
            .b(binaries)
            .root(stringify(get_root(o.destinations)))
            .debug_build(o.debug_build)
            .phase(ebin_ip_prepare)
            .disable_cfgpro(o.no_config_protect)
            .config_protect(_imp->portage_repository->profile_variable("CONFIG_PROTECT"))
            .config_protect_mask(_imp->portage_repository->profile_variable("CONFIG_PROTECT_MASK"))
            .loadsaveenv_dir(_imp->params.buildroot / stringify(q.category) / (
                    stringify(q.package) + "-" + stringify(v)) / "temp")
            .slot(SlotName(metadata->slot)));

    EbinInstallCommand prepare_cmd(command_params, install_params);
    prepare_cmd();

    install_params.phase = ebin_ip_initbinenv;
    EbinInstallCommand initbinenv_cmd(command_params, install_params);
    initbinenv_cmd();

    install_params.phase = ebin_ip_setup;
    EbinInstallCommand setup_cmd(command_params, install_params);
    setup_cmd();

    install_params.phase = ebin_ip_unpackbin;
    EbinInstallCommand unpackbin_cmd(command_params, install_params);
    unpackbin_cmd();

    for (DestinationsCollection::Iterator d(o.destinations->begin()),
            d_end(o.destinations->end()) ; d != d_end ; ++d)
    {
        if (! (*d)->destination_interface)
            throw PackageInstallActionError("Can't install '" + stringify(q) + "-"
                    + stringify(v) + "' to destination '" + stringify((*d)->name())
                    + "' because destination does not provide destination_interface");

        if ((*d)->destination_interface->want_pre_post_phases())
        {
            install_params.phase = ebin_ip_preinstall;
            install_params.root = (*d)->installed_interface ?
                stringify((*d)->installed_interface->root()) : "/";
            EbinInstallCommand preinst_cmd(command_params, install_params);
            preinst_cmd();
        }

        (*d)->destination_interface->merge(
                MergeOptions::create()
                .package(PackageDatabaseEntry(q, v, _imp->portage_repository->name()))
                .image_dir(command_params.buildroot / stringify(q.category) / (stringify(q.package) + "-"
                        + stringify(v)) / "image")
                .environment_file(command_params.buildroot / stringify(q.category) / (stringify(q.package) + "-"
                        + stringify(v)) / "temp" / "loadsaveenv")
                );

        if ((*d)->destination_interface->want_pre_post_phases())
        {
            install_params.phase = ebin_ip_postinstall;
            install_params.root = (*d)->installed_interface ?
                stringify((*d)->installed_interface->root()) : "/";
            EbinInstallCommand postinst_cmd(command_params, install_params);
            postinst_cmd();
        }
    }

    install_params.phase = ebin_ip_tidyup;
    EbinInstallCommand tidyup_cmd(command_params, install_params);
    tidyup_cmd();
}

tr1::shared_ptr<PortageRepositoryEntries>
EbinEntries::make_ebin_entries(
        const Environment * const e, PortageRepository * const r, const PortageRepositoryParams & p)
{
    return tr1::shared_ptr<PortageRepositoryEntries>(new EbinEntries(e, r, p));
}

std::string
EbinEntries::get_environment_variable(const QualifiedPackageName & q,
        const VersionSpec & v, const std::string & var, tr1::shared_ptr<const PortageRepositoryProfile>) const
{
    PackageDatabaseEntry for_package(q, v, _imp->portage_repository->name());

    throw EnvironmentVariableActionError("Couldn't get environment variable '" +
            stringify(var) + "' for package '" + stringify(for_package) + "'");
}

void
EbinEntries::merge(const MergeOptions & m)
{
    Context context("When merging '" + stringify(m.package) + "' at '" + stringify(m.image_dir)
            + "' to ebin repository '" + stringify(_imp->portage_repository->name()) + "':");

    if (! _imp->portage_repository->destination_interface->is_suitable_destination_for(m.package))
        throw PackageInstallActionError("Not a suitable destination for '" + stringify(m.package) + "'");

    FSEntry ebin_dir(_imp->params.location);
    ebin_dir /= stringify(m.package.name.category);
    ebin_dir.mkdir();
    ebin_dir /= stringify(m.package.name.package);
    ebin_dir.mkdir();

    FSEntry ebin_file_name(ebin_dir / (stringify(m.package.name.package) + "-" + stringify(m.package.version) + ".ebin.incomplete"));
    std::ofstream ebin_file(stringify(ebin_file_name).c_str());
    if (! ebin_file)
        throw PackageInstallActionError("Cannot write to '" + stringify(ebin_file_name) + "'");

    tr1::shared_ptr<const VersionMetadata> metadata(
            _imp->params.environment->package_database()->fetch_repository(m.package.repository)->
            version_metadata(m.package.name, m.package.version));

    if (metadata->deps_interface)
    {
        DepSpecPrettyPrinter r(0, false), p(0, false), s(0, false);
        metadata->deps_interface->run_depend()->accept(&r);
        metadata->deps_interface->post_depend()->accept(&p);
        metadata->deps_interface->suggested_depend()->accept(&s);
        ebin_file << "RDEPEND=" << r << std::endl;
        ebin_file << "PDEPEND=" << p << std::endl;
        ebin_file << "SDEPEND=" << s << std::endl;
    }

    if (metadata->license_interface)
    {
        DepSpecPrettyPrinter l(0, false);
        metadata->license_interface->license()->accept(&l);
        ebin_file << "LICENSE=" << l << std::endl;
    }

    ebin_file << "SLOT=" << metadata->slot << std::endl;
    DepSpecPrettyPrinter h(0, false);
    metadata->homepage()->accept(&h);
    ebin_file << "HOMEPAGE=" << h << std::endl;
    ebin_file << "DESCRIPTION=" << metadata->description << std::endl;
    ebin_file << "EAPI=" << metadata->eapi.name << std::endl;

    if (metadata->ebuild_interface)
    {
        DepSpecPrettyPrinter p(0, false), s(0, false), r(0, false);
        metadata->ebuild_interface->provide()->accept(&p);
        metadata->ebuild_interface->src_uri()->accept(&s);
        metadata->ebuild_interface->restrictions()->accept(&r);
        ebin_file << "PROVIDE=" << p << std::endl;
        ebin_file << "SRC_URI=" << s << std::endl;
        ebin_file << "RESTRICT=" << r << std::endl;
        ebin_file << "KEYWORDS=" << join(metadata->ebuild_interface->keywords()->begin(),
                metadata->ebuild_interface->keywords()->end(), " ") << std::endl;
        ebin_file << "IUSE=" << join(metadata->ebuild_interface->iuse()->begin(),
                metadata->ebuild_interface->iuse()->end(), " ") << std::endl;
        ebin_file << "INHERITED=" << join(metadata->ebuild_interface->inherited()->begin(),
                metadata->ebuild_interface->inherited()->end(), " ") << std::endl;
    }

    FSEntry pkg_file_name(_imp->params.pkgdir / (
                stringify(_imp->portage_repository->name()) + "--" +
                stringify(m.package.name.category) + "--" +
                stringify(m.package.name.package) + "-" +
                stringify(m.package.version) + ".tar.bz2"));

    ebin_file << "BIN_URI=" << _imp->params.write_bin_uri_prefix << pkg_file_name.basename() << std::endl;

    if (pkg_file_name.exists())
        pkg_file_name.unlink();

    EbinCommandParams command_params(EbinCommandParams::create()
            .environment(_imp->params.environment)
            .db_entry(&m.package)
            .portdir(_imp->params.master_repository ? _imp->params.master_repository->params().location :
                _imp->params.location)
            .pkgdir(_imp->params.pkgdir)
            .buildroot(_imp->params.buildroot));

    EbinMergeCommand merge_cmd(
            command_params,
            EbinMergeCommandParams::create()
            .pkg_file_name(pkg_file_name)
            .image(m.image_dir)
            .environment_file(m.environment_file));

    merge_cmd();

    FSEntry real_ebin_file_name(ebin_dir / (stringify(m.package.name.package) + "-" + stringify(m.package.version) + ".ebin"));
    if (real_ebin_file_name.exists())
        real_ebin_file_name.unlink();
    ebin_file_name.rename(real_ebin_file_name);
}

bool
EbinEntries::is_package_file(const QualifiedPackageName & n, const FSEntry & e) const
{
    return is_file_with_prefix_extension(e, stringify(n.package) + "-", ".ebin", IsFileWithOptions());
}

VersionSpec
EbinEntries::extract_package_file_version(const QualifiedPackageName & n, const FSEntry & e) const
{
    Context context("When extracting version from '" + stringify(e) + "':");
    return VersionSpec(strip_leading_string(strip_trailing_string(e.basename(), ".ebin"), stringify(n.package) + "-"));
}

bool
EbinEntries::pretend(const QualifiedPackageName &, const VersionSpec &,
        tr1::shared_ptr<const PortageRepositoryProfile>) const
{
    return true;
}

