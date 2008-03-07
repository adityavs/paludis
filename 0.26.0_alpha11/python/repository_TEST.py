#!/usr/bin/env python
# vim: set fileencoding=utf-8 sw=4 sts=4 et :

#
# Copyright (c) 2007 Piotr Jaroszyński
#
# This file is part of the Paludis package manager. Paludis is free software;
# you can redistribute it and/or modify it under the terms of the GNU General
# Public License version 2, as published by the Free Software Foundation.
#
# Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA
#

import os

repo_path = os.path.join(os.getcwd(), "repository_TEST_dir/testrepo")
os.environ["PALUDIS_HOME"] = os.path.join(os.getcwd(), "repository_TEST_dir/home")

from paludis import *

# To check for QA easily
import paludis

import unittest

Log.instance.log_level = LogLevel.WARNING

class TestCase_01_Repository(unittest.TestCase):
    def setUp(self):
        global e, nce, db, repo, irepo
        e = EnvironmentMaker.instance.make_from_spec("")
        nce = NoConfigEnvironment(repo_path)
        db = e.package_database
        repo = db.fetch_repository("testrepo")
        irepo = db.fetch_repository("installed")

    def test_01_fetch(self):
        self.assert_(isinstance(repo, Repository))
        self.assert_(isinstance(irepo, Repository))

    def test_02_create_error(self):
        self.assertRaises(Exception, Repository)

    def test_03_name(self):
        self.assertEquals(str(repo.name), "testrepo")
        self.assertEquals(str(irepo.name), "installed")

    def test_05_has_category_named(self):
        self.assert_(repo.has_category_named("foo"))
        self.assert_(not repo.has_category_named("bar"))

    def test_06_has_package_named(self):
        self.assert_(repo.has_package_named("foo/bar"))
        self.assert_(not repo.has_package_named("foo/foo"))
        self.assert_(not repo.has_package_named("bar/foo"))

    def test_07_package_ids(self):
        self.assertEquals([x.version for x in repo.package_ids("foo/bar")],
                [VersionSpec("1.0"), VersionSpec("2.0")]
                )
        self.assertEquals(len(list(repo.package_ids("bar/baz"))), 0)

    def test_08_category_names(self):
        self.assertEquals([str(x) for x in repo.category_names], ["foo", "foo1", "foo2", "foo3", "foo4"])

    def test_09_category_names_containing_package(self):
        self.assertEquals([str(x) for x in repo.category_names_containing_package("bar")],
                ["foo", "foo1", "foo2", "foo3", "foo4"])

    def test_10_package_names(self):
        for (i, qpn) in enumerate(repo.package_names("bar")):
            self.assertEquals(i, 0)
            self.assertEquals(str(qpn), "foo/bar")

    def test_11_some_ids_might_support_action(self):
        self.assert_(repo.some_ids_might_support_action(SupportsFetchActionTest()))
        self.assert_(not irepo.some_ids_might_support_action(SupportsFetchActionTest()))
        self.assert_(repo.some_ids_might_support_action(SupportsInstallActionTest()))
        self.assert_(not irepo.some_ids_might_support_action(SupportsInstallActionTest()))
        self.assert_(not repo.some_ids_might_support_action(SupportsUninstallActionTest()))
        self.assert_(irepo.some_ids_might_support_action(SupportsUninstallActionTest()))
        self.assert_(not repo.some_ids_might_support_action(SupportsInstalledActionTest()))
        self.assert_(irepo.some_ids_might_support_action(SupportsInstalledActionTest()))
        self.assert_(repo.some_ids_might_support_action(SupportsPretendActionTest()))
        self.assert_(not irepo.some_ids_might_support_action(SupportsPretendActionTest()))
        self.assert_(not repo.some_ids_might_support_action(SupportsConfigActionTest()))
        self.assert_(irepo.some_ids_might_support_action(SupportsConfigActionTest()))

class TestCase_02_RepositoryInterfaces(unittest.TestCase):
    def setUp(self):
        global e, nce, db, repo, irepo
        e = EnvironmentMaker.instance.make_from_spec("")
        nce = NoConfigEnvironment(repo_path)
        db = e.package_database
        repo = db.fetch_repository("testrepo")
        irepo = db.fetch_repository("installed")

    def test_02_sets_interface(self):
        si = repo.sets_interface
        self.assert_(isinstance(si, RepositorySetsInterface))

    def test_03_syncable_interface(self):
        si = repo.syncable_interface
        self.assert_(isinstance(si, RepositorySyncableInterface))

    def test_04_use_interface(self):
        ui = repo.use_interface
        self.assert_(isinstance(ui, RepositoryUseInterface))

        pid = list(repo.package_ids("foo/bar"))[1]

        self.assertEquals(ui.query_use("test1", pid), UseFlagState.ENABLED)
        self.assertEquals(ui.query_use("test2", pid), UseFlagState.DISABLED)
        self.assertEquals(ui.query_use("test3", pid), UseFlagState.ENABLED)
        self.assertEquals(ui.query_use("test4", pid), UseFlagState.UNSPECIFIED)
        self.assertEquals(ui.query_use("test5", pid), UseFlagState.DISABLED)
        self.assertEquals(ui.query_use("test6", pid), UseFlagState.ENABLED)
        self.assertEquals(ui.query_use("test7", pid), UseFlagState.ENABLED)

        self.assert_(not ui.query_use_mask("test1", pid))
        self.assert_(not ui.query_use_mask("test2", pid))
        self.assert_(not ui.query_use_mask("test3", pid))
        self.assert_(not ui.query_use_mask("test4", pid))
        self.assert_(ui.query_use_mask("test5", pid))
        self.assert_(not ui.query_use_mask("test6", pid))
        self.assert_(not ui.query_use_mask("test7", pid))

        self.assert_(not ui.query_use_force("test1", pid))
        self.assert_(not ui.query_use_force("test2", pid))
        self.assert_(not ui.query_use_force("test3", pid))
        self.assert_(not ui.query_use_force("test4", pid))
        self.assert_(not ui.query_use_force("test5", pid))
        self.assert_(ui.query_use_force("test6", pid))
        self.assert_(ui.query_use_force("test7", pid))

        self.assert_(ui.describe_use_flag("test1", pid), "A test use flag")

    def test_05_world_interface(self):
        wi = irepo.world_interface
        self.assert_(isinstance(wi, RepositoryWorldInterface))

    def test_06_environment_variable_interface(self):
        evi = repo.environment_variable_interface
        self.assert_(isinstance(evi, RepositoryEnvironmentVariableInterface))

    def test_07_mirrors_interface(self):
        mi = repo.mirrors_interface
        self.assert_(isinstance(mi, RepositoryMirrorsInterface))

    def test_08_provides_interface(self):
        pi = irepo.provides_interface
        self.assert_(isinstance(pi, RepositoryProvidesInterface))

    def test_09_virtuals_interface(self):
        vi = repo.virtuals_interface
        self.assert_(isinstance(vi, RepositoryVirtualsInterface))

    def test_10_destination_interface(self):
        di = irepo.destination_interface
        self.assert_(isinstance(di, RepositoryDestinationInterface))

    def test_12_e_interface(self):
        ei = nce.main_repository.e_interface

        self.assert_(isinstance(ei, RepositoryEInterface))

        path = os.path.join(os.getcwd(), "repository_TEST_dir/testrepo/profiles/testprofile")
        profile = ei.find_profile(path)

        self.assert_(isinstance(profile, RepositoryEInterfaceProfilesDescLine))
        self.assertEquals(profile.path, path)
        self.assertEquals(profile.arch, "x86")
        self.assertEquals(profile.status, "stable")


        self.assertEquals(ei.find_profile("broken"), None)

        profile = iter(ei.profiles).next()
        ei.profile = profile

        self.assertEquals(ei.profile_variable("ARCH"), "test")

    def test_12_qa_interface(self):
        if hasattr(paludis, "QAReporter"):
            class PyQAR(QAReporter):
                def __init__(self):
                    QAReporter.__init__(self)
                    self.messages = 0

                def message(self, msg):
                    self.messages += 1

                def status(self, msg):
                    return

            qi = repo.qa_interface
            self.assert_(isinstance(qi, RepositoryQAInterface))

            qr = PyQAR()

            qi.check_qa(qr, QACheckProperties(), QACheckProperties(),
                    QAMessageLevel.DEBUG, "repository_TEST_dir/testrepo")
            self.assertEquals(qr.messages > 0, True)

class TestCase_03_FakeRepository(unittest.TestCase):
    def setUp(self):
        global e, f
        e = EnvironmentMaker.instance.make_from_spec("")
        f = FakeRepository(e, "fake")

    def test_01_init(self):
        pass
        self.assertEquals(str(f.name), "fake")

    def test_02_init_bad(self):
        self.assertRaises(Exception, FakeRepository, e)
        self.assertRaises(Exception, FakeRepository, "foo", "foo")

    def test_03_add_category(self):
        self.assertEquals(list(f.category_names), [])

        f.add_category("cat-foo")
        self.assertEquals([str(x) for x in f.category_names], ["cat-foo"])
        f.add_category("cat-foo")
        self.assertEquals([str(x) for x in f.category_names], ["cat-foo"])
        f.add_category("cat-bar")
        self.assertEquals([str(x) for x in f.category_names], ["cat-bar", "cat-foo"])

    def test_04_add_category_bad(self):
        self.assertRaises(Exception, f.add_category, 1)
        self.assertRaises(Exception, f.add_category, "a", "b")

    def test_05_add_package(self):
        foo_1 = QualifiedPackageName("cat-foo/pkg1")
        foo_2 = QualifiedPackageName("cat-foo/pkg2")
        bar_1 = QualifiedPackageName("cat-bar/pkg1")

        f.add_category("cat-foo")
        self.assertEquals(list(f.package_names("cat-foo")), [])

        f.add_package(foo_1)
        self.assertEquals(list(f.package_names("cat-foo")), [foo_1])
        f.add_package(foo_1)
        self.assertEquals(list(f.package_names("cat-foo")), [foo_1])

        f.add_package(foo_2)
        self.assertEquals(list(f.package_names("cat-foo")), [foo_1, foo_2])

        f.add_package(bar_1)
        self.assertEquals(list(f.package_names("cat-bar")), [bar_1])

    def test_06_add_package_bad(self):
        self.assertRaises(Exception, f.add_category, 1)
        self.assertRaises(Exception, f.add_category, "a", "b")

    def test_07_add_version(self):
        f.add_package("cat-foo/pkg")
        self.assertEquals(list(f.package_ids("cat-foo/pkg")), [])

        pkg = f.add_version("cat-foo/pkg", "1")
        self.assertEquals(list(f.package_ids("cat-foo/pkg")), [pkg])
        self.assertEquals(pkg.version, VersionSpec("1"))

        pkg2 = f.add_version("cat-foo/pkg", "2")
        self.assertEquals(list(f.package_ids("cat-foo/pkg")), [pkg, pkg2])
        self.assertEquals(pkg2.version, VersionSpec("2"))

        pkg3 = f.add_version("cat-bar/pkg", "0")
        self.assertEquals(list(f.package_ids("cat-bar/pkg")), [pkg3])

        self.assertEquals([str(x) for x in f.category_names], ["cat-bar", "cat-foo"])

    def test_08_add_version_bad(self):
        self.assertRaises(Exception, f.add_version, 1)
        self.assertRaises(Exception, f.add_version, "a", "b", "c")


if __name__ == "__main__":
    unittest.main()

