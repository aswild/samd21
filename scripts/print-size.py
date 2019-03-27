#!/usr/bin/env python
# print-size.py: pretty-print the output of GNU Binutils size(1)
#
# Copyright (C) 2019 Allen Wild <allenwild93@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import print_function, division

import sys, os, re
from argparse import ArgumentParser
from subprocess import check_output, CalledProcessError

class ELFSize(object):
    def __init__(self, filename, size_prog=None):
        if size_prog is None:
            size_prog = os.environ.get('SIZE', 'size')

        cmd = [size_prog, '--format=sysv', filename]
        sizedata = check_output(cmd, universal_newlines=True)

        # Example output, we care about the size column
        # obj/ptctest.elf  :
        # section              size        addr
        # .text               10148        8192
        # .data                 208   536870912
        # .bss                 2068   536871120
        # .ARM.attributes        40           0
        # .comment               29           0
        # .debug_info        539227           0
        # <more debug sections>
        # Total             1402836
        sections = {}
        for line in sizedata.splitlines():
            m = re.match('(\.\w+)\s+(\d+)\s+\d+', line)
            if m:
                sections[m.group(1)] = int(m.group(2))

        self._text = sections.get('.text')
        self._data = sections.get('.data', sections.get('.relocate'))
        self._bss = sections.get('.bss')
        for sec in ('text', 'data', 'bss'):
            if not getattr(self, '_'+sec):
                raise RuntimeError('no size for section .%s'%sec)

    @property
    def text(self):
        return self._text

    @property
    def data(self):
        return self._data

    @property
    def bss(self):
        return self._bss

def print_size(name, size):
        print('  %-12s%6d B %5.1f kB'%(name, size, size/1024.0))

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-q', '--quiet', action='store_true', help="don't print filename header")
    parser.add_argument('-s', '--size-prog', nargs=1, help='size program to use. Default $SIZE from environment or `size`')
    parser.add_argument('file', metavar='FILE', help='ELF binary to analyze')
    args = parser.parse_args()

    try:
        s = ELFSize(args.file, args.size_prog and args.size_prog[0] or None)
    except Exception as e:
        sys.exit('Error calculating binary size: %s'%e)

    if not args.quiet:
        print('Program Size of %s:'%args.file)
    print_size('Total Flash', s.text + s.data)
    print_size('Static RAM', s.data + s.bss)
    print_size('.text', s.text)
    print_size('.data', s.data)
    print_size('.bss', s.bss)
