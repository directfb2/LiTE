#  This file is part of LiTE.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

lite_sources = [
  'animation.c',
  'box.c',
  'button.c',
  'check.c',
  'cursor.c',
  'font.c',
  'image.c',
  'label.c',
  'list.c',
  'lite.c',
  'progressbar.c',
  'scrollbar.c',
  'slider.c',
  'textbutton.c',
  'textline.c',
  'theme.c',
  'window.c',
]

lite_headers = [
  'animation.h',
  'box.h',
  'button.h',
  'check.h',
  'cursor.h',
  'font.h',
  'image.h',
  'label.h',
  'list.h',
  'lite.h',
  'progressbar.h',
  'scrollbar.h',
  'slider.h',
  'textbutton.h',
  'textline.h',
  'theme.h',
  'window.h',
]

liblite = library('lite-@0@.@1@'.format(lite_major_version, lite_minor_version),
                  lite_sources,
                  include_directories: config_inc,
                  c_args: '-DDATADIR="@0@"'.format(litedatadir),
                  dependencies: directfb_dep,
                  version: '@0@.0.0'.format(lite_micro_version),
                  install: true)

meson.add_install_script(join_paths(meson.source_root(), 'meson_symlink.sh'),
                         'liblite-@0@.@1@.so'.format(lite_major_version, lite_minor_version),
                         join_paths(get_option('prefix'), get_option('libdir'), 'liblite.so'))

install_headers(lite_headers, subdir: 'lite')

pkgconfig.generate(filebase: 'lite',
                   name: 'LiTE',
                   description: 'Lightweight Toolkit Enabler library',
                   requires: 'directfb',
                   libraries: '-L${libdir} -llite')
