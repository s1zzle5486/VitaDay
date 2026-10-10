ifndef VITASDK
$(error Set VITASDK to your VitaSDK directory)
endif
CXX := $(VITASDK)/bin/arm-vita-eabi-g++
TOOLS := $(VITASDK)/bin
LIBS := -lvita2d -lfreetype -lpng -ljpeg -lcurl -lssl -lcrypto -lz -lbz2 -lzstd -lm -lSceAudio_stub -lSceAppMgr_stub -lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub -lSceCtrl_stub -lSceTouch_stub -lScePgf_stub -lScePvf_stub -lSceCommonDialog_stub -lSceIme_stub -lSceAppUtil_stub -lScePower_stub -lSceRtc_stub -lSceNet_stub -lSceNetCtl_stub -lSceHttp_stub -lSceSsl_stub -lSceIofilemgr_stub
.PHONY: all clean
all: VitaDay.vpk
build/VitaDay.elf: src/main.cpp src/music.hpp src/music_tags.hpp src/music_art.hpp src/music_library.hpp src/music_vita.hpp src/music_ui.hpp src/mp3_decoder.cpp src/dr_mp3.h src/localization.hpp src/offline_weather.hpp src/weather_chart.hpp src/weather_cursor.hpp src/weather_status.hpp src/relative_time.hpp src/text_fit.hpp src/text_runs.hpp src/cjk_bitmap.hpp src/core.hpp src/network.hpp src/air_quality.hpp src/address.hpp src/holiday_names.hpp src/json.hpp src/ui.hpp src/input.hpp src/graph_stick.hpp src/rounded_panel.hpp src/control_guide.hpp src/scrollbar.hpp src/color_picker.hpp src/png_pixels.hpp src/jpeg_pixels.hpp src/lodepng.cpp src/lodepng.h tools/vita_gap.ld Makefile tools/check_relocations.py
	mkdir -p build
	$(CXX) -I$(VITASDK)/arm-vita-eabi/include/freetype2 -std=gnu++17 -O2 -g -Wall -Wextra -Wno-psabi -Wno-misleading-indentation -Wl,-q -Wl,-z,nocopyreloc -Wl,-T,tools/vita_gap.ld -DLODEPNG_NO_COMPILE_ENCODER src/main.cpp src/mp3_decoder.cpp src/lodepng.cpp -o $@ $(LIBS)
	python3 tools/check_relocations.py $@ $(TOOLS)/arm-vita-eabi-readelf
build/eboot.bin: build/VitaDay.elf
	$(TOOLS)/vita-elf-create $< build/VitaDay.velf
	$(TOOLS)/vita-make-fself -s build/VitaDay.velf $@
# ATTRIBUTE: NO_SUSPEND_DEACTIVATE (0x8) + DISABLE_SYSTEM_SUSPEND (0x80000).
# Power blanks the display while this app is open; the process remains awake.
build/param.sfo: Makefile VERSION
	mkdir -p build
	$(TOOLS)/vita-mksfoex -s TITLE_ID=VDAY00001 -s APP_VER=01.31 -d ATTRIBUTE=524296 -d ATTRIBUTE2=12 "VitaDay" $@
VitaDay.vpk: build/eboot.bin build/param.sfo $(shell find assets -type f) $(shell find sce_sys -type f) tools/check_livearea.py tools/update_livearea.py VERSION
	python3 tools/update_livearea.py
	python3 tools/check_livearea.py .
	$(TOOLS)/vita-pack-vpk -s build/param.sfo -b build/eboot.bin -a sce_sys=sce_sys -a assets=assets $@
clean:
	rm -rf build VitaDay.vpk
