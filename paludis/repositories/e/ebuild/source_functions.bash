#!/usr/bin/env bash
# vim: set sw=4 sts=4 et :

# Copyright (c) 2007, 2008 David Leverton
#
# Based in part upon ebuild.sh from Portage, which is Copyright 1995-2005
# Gentoo Foundation and distributed under the terms of the GNU General
# Public License v2.
#
# This file is part of the Paludis package manager. Paludis is free software;
# you can redistribute it and/or modify it under the terms of the GNU General
# Public License, version 2, as published by the Free Software Foundation.
#
# Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA

ebuild_safe_source()
{
    set -- "${@}" '[^a-zA-Z_]*' '*[^a-zA-Z0-9_]*' \
        EUID PPID UID FUNCNAME GROUPS SHELLOPTS \
        'BASH_@(ARGC|ARGV|LINENO|SOURCE|VERSINFO|REMATCH)' \
        'BASH_COMPLETION?(_DIR)' 'bash+([0-9])?([a-z])' \
        EBUILD_KILL_PID PALUDIS_LOADSAVEENV_DIR PALUDIS_DO_NOTHING_SANDBOXY SANDBOX_ACTIVE \
        PALUDIS_IGNORE_PIVOT_ENV_FUNCTIONS PALUDIS_IGNORE_PIVOT_ENV_VARIABLES

    trap DEBUG
    set -T
    shopt -s extdebug
    trap "[[ \${BASH_COMMAND%%=*} == ?(*[[:space:]])!($(IFS='|'; shift; echo "${*}")) ||
              \${BASH_COMMAND%%[[:space:]]*} != @(*=*|export|declare) ]]" DEBUG

    source "${1}"
    eval "trap DEBUG; shopt -u extdebug; set +T; return ${?}"
}

