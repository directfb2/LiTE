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

lite_fonts = []
if get_option('fontdir') == ''
  lite_fonts = [
    'Vera.dgiff',
    'Vera.ttf',
    'VeraBd.dgiff',
    'VeraBd.ttf',
    'VeraIt.dgiff',
    'VeraIt.ttf',
    'VeraBI.dgiff',
    'VeraBI.ttf',
    'VeraMo.dgiff',
    'VeraMo.ttf',
    'VeraMoBd.dgiff',
    'VeraMoBd.ttf',
    'VeraMoIt.dgiff',
    'VeraMoIt.ttf',
    'VeraMoBI.dgiff',
    'VeraMoBI.ttf',
    'VeraSe.dgiff',
    'VeraSe.ttf',
    'VeraSeBd.dgiff',
    'VeraSeBd.ttf',
    'whitrabt.dgiff',
    'whitrabt.ttf'
  ]
endif

lite_images = [
  'bottom.dfiff',
  'bottom.png',
  'bottomleft.dfiff',
  'bottomleft.png',
  'bottomright.dfiff',
  'bottomright.png',
  'button_disabled.dfiff',
  'button_disabled.png',
  'button_disabled_on.dfiff',
  'button_disabled_on.png',
  'button_hilite.dfiff',
  'button_hilite.png',
  'button_hilite_on.dfiff',
  'button_hilite_on.png',
  'button_normal.dfiff',
  'button_normal.png',
  'button_normal_on.dfiff',
  'button_normal_on.png',
  'button_pressed.dfiff',
  'button_pressed.png',
  'check.dfiff',
  'check.png',
  'cursor.dfiff',
  'cursor.png',
  'left.dfiff',
  'left.png',
  'progressbar_bg.dfiff',
  'progressbar_bg.png',
  'progressbar_fg.dfiff',
  'progressbar_fg.png',
  'right.dfiff',
  'right.png',
  'scrollbar.dfiff',
  'scrollbar.png',
  'textbutton.dfiff',
  'textbutton.png',
  'top.dfiff',
  'top.png',
  'topleft.dfiff',
  'topleft.png',
  'topright.dfiff',
  'topright.png'
]

install_data(lite_fonts, lite_images, install_dir: litedatadir)
