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
        version: '0.9.1')

lite_version = meson.project_version()
lite_major_version = lite_version.split('.')[0].to_int()
lite_minor_version = lite_version.split('.')[1].to_int()
lite_micro_version = lite_version.split('.')[2].to_int()

litedatadir = join_paths(get_option('prefix'), get_option('datadir'), 'lite')

config_h = configuration_data()

if get_option('fontdir') == ''
  config_h.set_quoted('LITEFONTDIR', litedatadir,           description: 'LiTE font directory.')
else
  config_h.set_quoted('LITEFONTDIR', get_option('fontdir'), description: 'LiTE font directory.')
endif

configure_file(configuration: config_h, output: 'config.h')

config_inc = include_directories('.')

pkgconfig = import('pkgconfig')

directfb_dep = dependency('directfb')

subdir('data')
subdir('lite')
