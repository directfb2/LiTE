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

include $(APPDIR)/Make.defs

CFLAGS += -Idata
CFLAGS += -DDFB_CORE_SYSTEM=nuttxfb
CFLAGS += -DDFB_INPUT_DRIVER=nuttx_input
CFLAGS += -DDFB_FONT_PROVIDER=$(CONFIG_LITE_FONT_PROVIDER)
CFLAGS += -DDFB_IMAGE_PROVIDER=$(CONFIG_LITE_IMAGE_PROVIDER)
CFLAGS += -DDFB_WINDOW_MANAGER=default

CSRCS  = lite/animation.c
CSRCS += lite/box.c
CSRCS += lite/button.c
CSRCS += lite/check.c
CSRCS += lite/cursor.c
CSRCS += lite/font.c
CSRCS += lite/image.c
CSRCS += lite/label.c
CSRCS += lite/list.c
CSRCS += lite/lite.c
CSRCS += lite/progressbar.c
CSRCS += lite/scrollbar.c
CSRCS += lite/slider.c
CSRCS += lite/textbutton.c
CSRCS += lite/textline.c
CSRCS += lite/theme.c
CSRCS += lite/window.c

RAWDATA_HDRS  = data/bottom.h
RAWDATA_HDRS += data/bottomleft.h
RAWDATA_HDRS += data/bottomright.h
RAWDATA_HDRS += data/button_disabled.h
RAWDATA_HDRS += data/button_disabled_on.h
RAWDATA_HDRS += data/button_hilite.h
RAWDATA_HDRS += data/button_hilite_on.h
RAWDATA_HDRS += data/button_normal.h
RAWDATA_HDRS += data/button_normal_on.h
RAWDATA_HDRS += data/button_pressed.h
RAWDATA_HDRS += data/checkbox.h
RAWDATA_HDRS += data/left.h
RAWDATA_HDRS += data/progressbar_bg.h
RAWDATA_HDRS += data/progressbar_fg.h
RAWDATA_HDRS += data/right.h
RAWDATA_HDRS += data/scrollbarbox.h
RAWDATA_HDRS += data/textbuttonbox.h
RAWDATA_HDRS += data/top.h
RAWDATA_HDRS += data/topleft.h
RAWDATA_HDRS += data/topright.h
RAWDATA_HDRS += data/Vera.h
RAWDATA_HDRS += data/VeraBd.h
RAWDATA_HDRS += data/VeraIt.h
RAWDATA_HDRS += data/VeraBI.h
RAWDATA_HDRS += data/VeraMo.h
RAWDATA_HDRS += data/VeraMoBd.h
RAWDATA_HDRS += data/VeraMoIt.h
RAWDATA_HDRS += data/VeraMoBI.h
RAWDATA_HDRS += data/VeraSe.h
RAWDATA_HDRS += data/VeraSeBd.h
RAWDATA_HDRS += data/whitrabt.h
RAWDATA_HDRS += data/wincursor.h

DIRECTFB_CSOURCE ?= directfb-csource

data/%.h: data/%.$(shell echo $(CONFIG_LITE_IMAGE_EXTENSION))
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

data/%.h: data/%.$(shell echo $(CONFIG_LITE_FONT_EXTENSION))
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

lite/font.c: data/Vera.h data/VeraBd.h data/VeraIt.h data/VeraBI.h data/VeraMo.h data/VeraMoBd.h data/VeraMoIt.h data/VeraMoBI.h data/VeraSe.h data/VeraSeBd.h data/whitrabt.h

lite/lite.c: data/bottom.h data/bottomleft.h data/bottomright.h data/button_normal.h data/button_pressed.h data/button_hilite.h data/button_disabled.h data/button_hilite_on.h data/button_disabled_on.h data/button_normal_on.h data/checkbox.h data/left.h data/progressbar_bg.h data/progressbar_fg.h data/right.h data/scrollbarbox.h data/textbuttonbox.h data/top.h data/topleft.h data/topright.h data/wincursor.h

context:: $(RAWDATA_HDRS)

distclean::
	$(call DELFILE, $(RAWDATA_HDRS))

include $(APPDIR)/Application.mk
