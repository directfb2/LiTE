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

fontdata_names = [
  'Vera',
  'VeraBd',
  'VeraIt',
  'VeraBI',
  'VeraMo',
  'VeraMoBd',
  'VeraMoIt',
  'VeraMoBI',
  'VeraSe',
  'VeraSeBd',
  'whitrabt'
]

lite_images = []

imagedata_names = [
  'bottom',
  'bottomleft',
  'bottomright',
  'button_disabled',
  'button_disabled_on',
  'button_hilite',
  'button_hilite_on',
  'button_normal',
  'button_normal_on',
  'button_pressed',
  'checkbox',
  'left',
  'progressbar_bg',
  'progressbar_fg',
  'right',
  'scrollbarbox',
  'textbuttonbox',
  'top',
  'topleft',
  'topright',
  'wincursor'
]

rawdata_hdrs = []

if font_headers == 'disabled'
  if get_option('fontdir') == ''
    foreach name : fontdata_names
      lite_fonts += name + '.dgiff'
      lite_fonts += name + '.ttf'
    endforeach
  endif
else
  foreach fontdata_name : fontdata_names
    rawdata_hdrs += custom_target(fontdata_name,
                                  input: fontdata_name + '.' + font_headers,
                                  output: fontdata_name + '.h',
                                  capture: true,
                                  command: [directfb_csource, '--raw', '--name=@0@'.format(fontdata_name), '@INPUT@'])
  endforeach
endif

if image_headers == 'disabled'
  foreach name : imagedata_names
    lite_images += name + '.dfiff'
    lite_images += name + '.png'
  endforeach
else
  foreach imagedata_name : imagedata_names
    rawdata_hdrs += custom_target(imagedata_name,
                                  input: imagedata_name + '.' + image_headers,
                                  output: imagedata_name + '.h',
                                  capture: true,
                                  command: [directfb_csource, '--raw', '--name=@0@'.format(imagedata_name), '@INPUT@'])
  endforeach
endif

install_data(lite_fonts, lite_images, install_dir: litedatadir)
