#!/usr/bin/env ruby
# vim: set sw=4 sts=4 et tw=80 :
#
# Copyright (c) 2007 Ciaran McCreesh
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

require 'test/unit'
require 'Paludis'

module Paludis
    class TestCase_SupportsActionTestBase < Test::Unit::TestCase
        def test_no_create
            assert_raise NoMethodError do
                ce = SupportsActionTestBase.new('test')
            end
        end
    end

    class TestCase_SupportsFetchActionTest < Test::Unit::TestCase
        def test_create
            assert_kind_of SupportsFetchActionTest, SupportsFetchActionTest.new
            assert_kind_of SupportsActionTestBase, SupportsFetchActionTest.new
        end
    end

    class TestCase_SupportsInfoActionTest < Test::Unit::TestCase
        def test_create
            assert_kind_of SupportsInfoActionTest, SupportsInfoActionTest.new
            assert_kind_of SupportsActionTestBase, SupportsInfoActionTest.new
        end
    end

    class TestCase_SupportsConfigActionTest < Test::Unit::TestCase
        def test_create
            assert_kind_of SupportsConfigActionTest, SupportsConfigActionTest.new
            assert_kind_of SupportsActionTestBase, SupportsConfigActionTest.new
        end
    end

    class TestCase_Action < Test::Unit::TestCase
        def test_no_create
            assert_raise NoMethodError do
                ce = Action.new('test')
            end
        end
    end

    class TestCase_FetchActionOptions < Test::Unit::TestCase
        def test_create
            assert_kind_of FetchActionOptions, FetchActionOptions.new(false, false)
            assert_kind_of FetchActionOptions, FetchActionOptions.new(
                {:safe_resume => false, :fetch_unneeded => false})
        end
    end

    class TestCase_FetchActionFailure < Test::Unit::TestCase
        def test_create
            assert_kind_of FetchActionFailure, FetchActionFailure.new('target_file', false, false, 'fic')
            assert_kind_of FetchActionFailure, FetchActionFailure.new(
                {
                :requires_manual_fetching => false, :failed_automatic_fetching => false,
                :target_file => 'target_file', :failed_integrity_checks => 'fic'
                }
            )
        end

        def test_methods_hash_args
             failure =  FetchActionFailure.new(
                {
                :requires_manual_fetching => false, :failed_automatic_fetching => false,
                :target_file => 'target_file', :failed_integrity_checks => 'fic'
                }
             )
             assert_equal 'target_file', failure.target_file;
             assert ! failure.requires_manual_fetching?
             assert ! failure.failed_automatic_fetching?
             assert_equal 'fic', failure.failed_integrity_checks;
        end

        def test_methods_4_args
             failure = FetchActionFailure.new('target_file', false, false, 'fic')
             assert_equal 'target_file', failure.target_file;
             assert ! failure.requires_manual_fetching?
             assert ! failure.failed_automatic_fetching?
             assert_equal 'fic', failure.failed_integrity_checks;
        end
    end

    class TestCase_FetchAction < Test::Unit::TestCase
        def test_create
            assert_kind_of FetchAction, FetchAction.new(FetchActionOptions.new(false, false))
            assert_kind_of Action, FetchAction.new(FetchActionOptions.new(false, false))

            assert_kind_of FetchAction, FetchAction.new(FetchActionOptions.new(
                {:safe_resume => false, :fetch_unneeded => false}))
        end

        def test_bad_create
            assert_raise TypeError do
                FetchAction.new("foo")
            end

            assert_raise ArgumentError do
                FetchAction.new(FetchActionOptions.new({:monkey => false}))
            end
        end

        def test_options
            a = FetchAction.new(FetchActionOptions.new(false, true))
            assert_equal a.options.fetch_unneeded, false
            assert_equal a.options.safe_resume, true

            a = FetchAction.new(FetchActionOptions.new({:safe_resume => false, :fetch_unneeded => true}))
            assert_equal a.options.fetch_unneeded, true
            assert_equal a.options.safe_resume, false
        end
    end

    class TestCase_InfoAction < Test::Unit::TestCase
        def test_create
            assert_kind_of InfoAction, InfoAction.new
            assert_kind_of Action, InfoAction.new
        end

        def test_bad_create
            assert_raise ArgumentError do
                InfoAction.new('')
            end
        end
    end

    class TestCase_ConfigAction < Test::Unit::TestCase
        def test_create
            assert_kind_of ConfigAction, ConfigAction.new
            assert_kind_of Action, ConfigAction.new
        end

        def test_bad_create
            assert_raise ArgumentError do
                ConfigAction.new('')
            end
        end
    end
end


