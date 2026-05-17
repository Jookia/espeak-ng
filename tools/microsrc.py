#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright 2026 Jookia <contact@jookia.org>

# This script takes the espeak-ng source code and generates a smaller set of
# source code aimed to embed in to an existing project. This can be more
# convenient than building a library just for a single application.

import sys
import pathlib
import shutil
import os

def main(args):
    if len(args) != 4:
        print('Usage: bundle.py /path/to/espeak-ng microsrc VERSION')
        return 1

    espeak_dir = args[1] # The espeak-ng directory
    microsrc_dir = args[2] # The microsrc directory
    version = args[3] # The espeak-ng version

    espeak_path = pathlib.Path(espeak_dir)
    dst_path = pathlib.Path(microsrc_dir)
    src_path = espeak_path / 'src'

    # Create a clean destination directory
    if dst_path.exists():
	    shutil.rmtree(dst_path)
    os.mkdir(dst_path)

    # Copy the following projects to the microsrc directory:
    # - espeak-ng
    # - libespeak-ng
    # - speechPlayer
    # - ucd-tools
    # While we do this we will merge their include directories.
    copy_trees = [
        ('include', 'include'),
        ('compat', 'compat'),
        ('libespeak-ng', 'libespeak-ng'),
        ('speechPlayer/src', 'speechPlayer'),
        ('speechPlayer/include', 'include'),
        ('ucd-tools/src', 'ucd-tools'),
        ('ucd-tools/src/include', 'include')]
    for (src, dst) in copy_trees:
        shutil.copytree(src_path / src, dst_path / dst, dirs_exist_ok=True)
    shutil.rmtree(dst_path / 'ucd-tools' / 'include')
    shutil.copy(src_path / 'espeak-ng.c', dst_path / 'espeak-ng.c')

    # Remove CMakeLists.txt to avoid conflicts with the build environment.
    (dst_path / 'libespeak-ng/CMakeLists.txt').unlink()

    # Add a simple configuration, but put it in the library directory to avoid
    # claiming <config.h> which may already be used by the build environment.
    config_h = open(dst_path / 'include/config.h', 'w')
    config_h.write(
        f'#define USE_KLATT 1\n'
        f'#define USE_SPEECHPLAYER 1\n'
        f'#define PACKAGE_VERSION "{version}"\n'
        f'#define LIBESPEAK_NG_EXPORT 1\n')
    config_h.close()

    # Add a very simple build script example
    build_sh = open(dst_path / 'build.sh', 'w')
    build_sh.write(
        f'#!/usr/bin/env bash\n'
        f'exec g++ -Os -g \\\n'
        f'  -fpermissive -fPIC -fno-exceptions -fwrapv \\\n'
        f'  -Iinclude -Iinclude/compat \\\n'
        f'  compat/*.c \\\n'
        f'  libespeak-ng/*.c \\\n'
        f'  ucd-tools/*.c \\\n'
        f'  speechPlayer/*.cpp \\\n'
        f'  espeak-ng.c \\\n'
        f'  -o espeak-ng\n')
    build_sh.close()
    os.chmod(dst_path / 'build.sh', 0o755)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
