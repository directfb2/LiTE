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

project('LiTE', 'c',
        version: '0.9.1',
        meson_version: '>= 0.61')

lite_version = meson.project_version()
lite_major_version = lite_version.split('.')[0].to_int()
lite_minor_version = lite_version.split('.')[1].to_int()
lite_micro_version = lite_version.split('.')[2].to_int()

if get_option('default_library') == 'both'
  error('''Choose library type between 'shared' or 'static'.''')
elif get_option('default_library') == 'shared'
  libsuffix = '.so'
else
  libsuffix = '.a'
endif

litedatadir = join_paths(get_option('prefix'), get_option('datadir'), 'lite')

font_headers  = get_option('font-headers')
image_headers = get_option('image-headers')

if font_headers == 'disabled'
  if get_option('fontdir') == ''
    add_global_arguments('-DLITEFONTDIR="@0@"'.format(litedatadir), language: 'c')
  else
    add_global_arguments('-DLITEFONTDIR="@0@"'.format(get_option('fontdir')), language: 'c')
  endif
endif

if image_headers == 'disabled'
  add_global_arguments('-DLITEIMAGEDIR="@0@"'.format(litedatadir), language: 'c')
endif

if font_headers != 'disabled' or image_headers != 'disabled'
  directfb_csource = find_program('directfb-csource')
endif

pkgconfig = import('pkgconfig')

directfb_dep = dependency('directfb')

subdir('data')
subdir('lite')
