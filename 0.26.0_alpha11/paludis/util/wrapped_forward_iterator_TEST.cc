/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 Ciaran McCreesh
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

#include <paludis/util/wrapped_forward_iterator-impl.hh>
#include <paludis/util/join.hh>
#include <paludis/util/iterator_funcs.hh>
#include <test/test_runner.hh>
#include <test/test_framework.hh>
#include <vector>
#include <list>

using namespace test;
using namespace paludis;

namespace test_cases
{
    struct WrappedForwardIteratorTest : TestCase
    {
        WrappedForwardIteratorTest() : TestCase("wrapped_forward_iterator") { }

        void run()
        {
            std::list<int> l;
            l.push_back(1);
            l.push_back(2);
            l.push_back(3);

            typedef WrappedForwardIterator<void, int> I;
            TEST_CHECK_EQUAL(join(I(l.begin()), I(l.end()), ", "), "1, 2, 3");
            TEST_CHECK(I(l.begin()).underlying_iterator<std::list<int>::iterator>() == l.begin());
        }
    } test_wrapped_forward_iterator;
}

