/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
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

#include "merge_common.hh"

#include <paludis/digests/md5.hh>
#include <paludis/util/dir_iterator.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/log.hh>
#include <paludis/util/pstream.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/strip.hh>
#include <paludis/util/system.hh>
#include <paludis/util/tokeniser.hh>
#include <paludis/selinux/security_context.hh>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"

using namespace paludis;
using namespace merge;

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::istreambuf_iterator;
using std::ostreambuf_iterator;

namespace
{
    int exit_status;

    FSEntry
    make_config_protect_name(const FSEntry & name, const FSEntry & file_to_install)
    {
        int n(0);

        ifstream our_md5_file(stringify(file_to_install).c_str());
        if (! our_md5_file)
            throw Failure("Could not get md5 for '" + stringify(file_to_install) + "'");
        MD5 our_md5(our_md5_file);

        FSEntry result(name);
        while (true)
        {
            if (! result.exists())
                return result;
            else if (result.is_regular_file())
            {
                ifstream other_md5_file(stringify(result).c_str());
                if (! other_md5_file)
                    throw Failure("Could not get md5 for '" + stringify(result) + "'");
                MD5 other_md5(other_md5_file);

                if (our_md5.hexsum() == other_md5.hexsum())
                    return result;
            }

            std::stringstream s;
            s << std::setw(4) << std::setfill('0') << std::right << n++;
            result = FSEntry(stringify(name.dirname() / ("._cfg" + s.str() + "_"
                            + name.basename())));
        }
    }

    /**
     * Output stream buffer class that's opened via an FD, so that we can avoid race
     * conditions that would be present on the non-Unix-specific standard file
     * stream classes.
     *
     * See \ref TCppSL Ch. 13.13 for what we're doing here. The buffer code is
     * based upon the "io/outbuf2.hpp" example in section 13.13.3.
     */
    class FDOutputStreamBuf :
        public std::streambuf
    {
        protected:
            int fd;

            virtual int_type
            overflow(int_type c)
            {
                if (c != EOF)
                {
                    char z = c;
                    if (1 != write(fd, &z, 1))
                        return EOF;
                }
                return c;
            }

            virtual std::streamsize
            xsputn(const char * s, std::streamsize num)
            {
                return write(fd, s, num);
            }

        public:
            FDOutputStreamBuf(const int f) :
                fd(f)
            {
            }
    };

    /**
     * Member from base initialisation for FDOutputStream.
     */
    class FDOutputStreamBase
    {
        protected:
            FDOutputStreamBuf buf;

        public:
            FDOutputStreamBase(const int fd) :
                buf(fd)
            {
            }
    };
    /**
     * Output stream buffer class that's opened via an FD, so that we can avoid race
     * conditions that would be present on the non-Unix-specific standard file
     * stream classes.
     */
    class FDOutputStream :
        protected FDOutputStreamBase,
        public std::ostream
    {
        public:
            FDOutputStream(const int fd) :
                FDOutputStreamBase(fd),
                std::ostream(&buf)
        {
        }
    };

    /**
     * RAII holder for a file descriptor.
     */
    class FDHolder
    {
        private:
            const int _fd;
            const bool _sync;

        public:
            FDHolder(const int fd, bool sync = true) :
                _fd(fd),
                _sync(sync)
            {
            }

            ~FDHolder()
            {
                if (-1 != _fd)
                {
                    if (_sync)
                        ::fsync(_fd);
                    ::close(_fd);
                }
            }

            operator int () const
            {
                return _fd;
            }
    };

    void
    do_dir(const FSEntry & root, const FSEntry & src_dir,
            const FSEntry & dst_dir, ofstream * const contents)
    {
        Context context("Installing directory in root '" + stringify(root) + "' from '"
                    + stringify(src_dir) + "' to '" + stringify(dst_dir) + "':");

        std::string root_str(stringify(root)), dst_dir_str(stringify(dst_dir.dirname()));
        if (root_str == "/")
            root_str.clear();
        if (0 != dst_dir_str.compare(0, root_str.length(), root_str))
            throw Failure("do_dir confused: '" + root_str + "' '" + dst_dir_str + "'");

        cout << ">>> " << std::setw(5) << std::left << "[dir]" << " " <<
            dst_dir_str.substr(root_str.length()) << "/" << dst_dir.basename() << endl;

        if (dst_dir.is_directory())
            /* nothing */;
        else if (dst_dir.is_symbolic_link() && dst_dir.realpath().is_directory())
            Log::get_instance()->message(ll_qa, lc_no_context, "Expected '" + stringify(dst_dir) +
                    "' to be a directory or non-existent, but found a symlink to a directory");
        else if (dst_dir.exists())
            throw Failure("Expected '" + stringify(dst_dir) +
                    "' to be a directory or non-existent, but found a file");
        else
        {
            mode_t mode(src_dir.permissions());

#ifdef HAVE_SELINUX
            CountedPtr<FSCreateCon, count_policy::ExternalCountTag> createcon(0);
            if (MatchPathCon::get_instance()->good())
            {
                FSCreateCon *p = new FSCreateCon(MatchPathCon::get_instance()->match(dst_dir_str.substr(root_str.length()),
                            mode));
                createcon.assign(p);
            }
#endif

            FSEntry(dst_dir).mkdir(mode);
            FSEntry(stringify(dst_dir)).chown(src_dir.owner(), src_dir.group());
            /* the chmod is needed to pick up fancy set*id bits */
            FSEntry(stringify(dst_dir)).chmod(src_dir.permissions());
        }

        *contents << "dir " << dst_dir_str.substr(root_str.length()) << "/" <<
            dst_dir.basename() << endl;
    }

    void
    copy_file_contents(int input_fd, int fd)
    {
        char buf[4096];
        ssize_t count;
        while ((count = read(input_fd, buf, 4096)) > 0)
            write(fd, buf, count);
    }

    void
    do_obj(const FSEntry & root, const FSEntry & src,
            const FSEntry & dst, ofstream * const contents)
    {
        Context context("Installing object in root '" + stringify(root) + "' from '"
                    + stringify(src) + "' to '" + stringify(dst) + "':");

        std::string root_str(stringify(root)), dst_dir_str(stringify(dst.dirname()));
        if (root_str == "/")
            root_str.clear();
        if (0 != dst_dir_str.compare(0, root_str.length(), root_str))
            throw Failure("do_obj confused: '" + root_str + "' '" + dst_dir_str + "'");

        cout << ">>> " << std::setw(5) << std::left << "[obj]" << " " <<
            dst_dir_str.substr(root_str.length()) << "/" << dst.basename();

        if (dst.is_directory())
            throw Failure("Cannot overwrite directory '" + stringify(dst) + "' with a file");
        else
        {
            FSEntry real_dst(dst);

            FDHolder input_fd(::open(stringify(src).c_str(), O_RDONLY), false);
            if (-1 == input_fd)
                throw Failure("Cannot read '" + stringify(src) + "'");

            if (dst.exists())
            {
                if (is_config_protected(root, dst))
                {
                    real_dst = make_config_protect_name(dst, src);
                    if (dst != real_dst)
                        cout << " -> " << real_dst << endl;
                    else
                        cout << endl;
                }
                else
                {
                    FSEntry(dst).unlink();
                    cout << endl;
                }
            }
            else
                cout << endl;

            /* FDHolder must be destroyed before we do the md5 thing, or the
             * disk write may not have synced. */
            {
#ifdef HAVE_SELINUX
                CountedPtr<FSCreateCon, count_policy::ExternalCountTag> createcon(0);
                if (MatchPathCon::get_instance()->good())
                    createcon.assign(new 
                            FSCreateCon(MatchPathCon::get_instance()->match(dst_dir_str.substr(root_str.length()) + "/"
                                    + dst.basename(), src.permissions())));
#endif
                FDHolder fd(::open(stringify(real_dst).c_str(), O_WRONLY | O_CREAT, src.permissions()));
                if (-1 == fd)
                    throw Failure("Cannot open '" + stringify(real_dst) + "' for write");

                if (0 != ::fchown(fd, src.owner(), src.group()))
                    throw Failure("Cannot fchown '" + stringify(real_dst) + "': " +
                            stringify(::strerror(errno)));

                /* the chmod is needed for set*id bits, which are dropped by
                 * umask in the ::open */
                if (0 != ::fchmod(fd, src.permissions()))
                    throw Failure("Cannot fchmod '" + stringify(real_dst) + "': " +
                            stringify(::strerror(errno)));

                copy_file_contents(input_fd, fd);
            }

            ifstream dst_file(stringify(dst).c_str());
            if (! dst_file)
                throw Failure("Could not get md5 for '" + stringify(dst_file) + "'");
            MD5 md5(dst_file);

            *contents << "obj " << dst_dir_str.substr(root_str.length()) << "/" <<
                dst.basename() << " " << md5.hexsum() << " " <<
                FSEntry(stringify(dst)).mtime() << endl;
        }
    }

    void
    do_sym(const FSEntry & root, const FSEntry & src,
            const FSEntry & dst, ofstream * const contents)
    {
        Context context("Installing symlink in root '" + stringify(root) + "' from '"
                    + stringify(src) + "' to '" + stringify(dst) + "':");

        std::string root_str(stringify(root)), dst_dir_str(stringify(dst.dirname()));
        if (root_str == "/")
            root_str.clear();
        if (0 != dst_dir_str.compare(0, root_str.length(), root_str))
            throw Failure("do_sym confused: '" + root_str + "' '" + dst_dir_str + "'");

        cout << ">>> " << std::setw(5) << std::left << "[sym]" << " " <<
            dst_dir_str.substr(root_str.length()) << "/" << dst.basename() << endl;

        if (dst.exists())
        {
            if (dst.is_directory())
                throw Failure("Can't overwrite directory '" + stringify(dst) +
                        "' with symlink to '" + src.readlink() + "'");
            else
                FSEntry(dst).unlink();
        }

#ifdef HAVE_SELINUX
        // permissions() on a symlink does weird things, but matchpathcon only cares about the file type,
        // so just pass S_IFLNK.
        CountedPtr<FSCreateCon, count_policy::ExternalCountTag> createcon(0);
        if (MatchPathCon::get_instance()->good())
            createcon.assign(new 
                    FSCreateCon(MatchPathCon::get_instance()->match(dst_dir_str.substr(root_str.length()) + "/"
                            + dst.basename(), S_IFLNK)));
#endif

        if (0 != ::symlink(src.readlink().c_str(), stringify(dst).c_str()))
        {
            Log::get_instance()->message(ll_warning, lc_no_context, "Couldn't create symlink '"
                    + src.readlink() + "' at '" + stringify(dst) + "'");
            exit_status |= 2;
        }

        *contents << "sym " << dst_dir_str.substr(root_str.length()) << "/" <<
            dst.basename() << " -> " << dst.readlink() << " " <<
            FSEntry(stringify(dst)).mtime() << endl;
    }

    void
    merge_this(const FSEntry & root, const FSEntry & src_dir, const FSEntry & dst_dir,
            ofstream * const contents)
    {
        Context context("Merging under root '" + stringify(root) + "' from '"
                    + stringify(src_dir) + "' to '" + stringify(dst_dir) + "':");

        if (! root.is_directory())
            throw Failure("merge_this called with bad root '" + stringify(root) + "'");
        if (! src_dir.is_directory())
            throw Failure("merge_this called with bad src_dir '" + stringify(src_dir) + "'");
        if (! dst_dir.is_directory())
            throw Failure("merge_this called with bad dst_dir '" + stringify(dst_dir) + "'");

        for (DirIterator d(src_dir, false), d_end ; d != d_end ; ++d)
        {
            if (d->is_regular_file())
                do_obj(root, src_dir / d->basename(), dst_dir / d->basename(), contents);
            else if (d->is_symbolic_link())
                do_sym(root, src_dir / d->basename(), dst_dir / d->basename(), contents);
            else if (d->is_directory())
            {
                do_dir(root, src_dir / d->basename(), dst_dir / d->basename(), contents);
                merge_this(root, (src_dir / d->basename()).realpath(), (dst_dir / d->basename()).realpath(), contents);
            }
            else
                throw Failure("Don't know how to merge '" + stringify(*d) + "'");
        }
    }
}

int
main(int argc, char * argv[])
{
    Context context("In main program:");

#ifdef HAVE_SELINUX
    // If the MatchPathCon initialisation fails, don't attempt to match contexts when merging.
    if (! MatchPathCon::get_instance()->good())
        Log::get_instance()->message(ll_warning, lc_no_context,
                "matchpathcon_init failed; not setting security contexts");
#endif

    exit_status = 0;
    try
    {
        if (argc != 4)
            throw Failure("Usage: " + stringify(argv[0]) + " src dst contents");

        Log::get_instance()->set_program_name(argv[0]);
        std::string log_level(getenv_with_default("PALUDIS_EBUILD_LOG_LEVEL", "qa"));

        if (log_level == "debug")
            Log::get_instance()->set_log_level(ll_debug);
        else if (log_level == "qa")
            Log::get_instance()->set_log_level(ll_qa);
        else if (log_level == "warning")
            Log::get_instance()->set_log_level(ll_warning);
        else if (log_level == "silent")
            Log::get_instance()->set_log_level(ll_silent);
        else
            throw Failure("bad value for log level");

        Log::get_instance()->message(ll_debug, lc_no_context,
                "CONFIG_PROTECT is " + getenv_with_default("CONFIG_PROTECT", "(unset)"));
        Log::get_instance()->message(ll_debug, lc_no_context,
                "CONFIG_PROTECT_MASK is " + getenv_with_default("CONFIG_PROTECT_MASK", "(unset)"));

        FSEntry src(argv[1]), dst(argv[2]), contents(argv[3]);

        if (! ((src = src.realpath())).is_directory())
            throw Failure(stringify(argv[1]) + ": not a directory");
        if (! ((dst = dst.realpath())).is_directory())
            throw Failure(stringify(argv[2]) + ": not a directory");

        ofstream contents_file(stringify(contents).c_str());
        if (! contents_file)
            throw Failure(stringify(contents) + ": not writeable");

        ::umask(0000);
        merge_this(dst, src, dst, &contents_file);
        return exit_status;
    }
    catch (const Failure & f)
    {
        cerr << argv[0] << ": fatal error: " << f.message << endl;
        return EXIT_FAILURE;
    }
    catch (const Exception & e)
    {
        cerr << argv[0] << ": fatal error:" << endl
            << "  * " << e.backtrace("\n  * ") << e.message()
            << " (" << e.what() << ")" << endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception & e)
    {
        cerr << argv[0] << ": fatal error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}

