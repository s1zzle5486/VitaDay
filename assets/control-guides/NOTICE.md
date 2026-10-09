# VitaDay control guides

Twenty raster guides generated with the built-in image_gen tool for VitaDay. They use the generated PS Vita controller poster and the VitaDay 0.10.11 desktop-rendered home screen as references. The centre image is a generated localized rendition of that screen. Prompt set: prompts.json. German wording correction: correction-de.txt.

Files follow the locale codes in src/localization.hpp. The app loads only the current language, on demand. Native images are area-resampled to 960 x 540 and shown at fixed size; there is no zoom.

Native .vgi files contain a 20-byte VDAYGI01 header followed by little-endian RGB565 pixels. tools/build_control_guides.cpp converts each original PNG from the separately delivered VitaDay-controls-20-languages.zip, by area-resampling to 960 x 540 without cropping. 8-bit colour channels are quantized to 5/6/5 bits. Full-resolution PNG originals remain in that separate archive, outside the installed app.
