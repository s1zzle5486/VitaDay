# VitaDay 0.10.31

Local MP3 music and richer weather for homebrew-enabled PS Vita.

Install **VitaDay-0.10.31.vpk** with VitaShell over the previous version. Settings, calendar events and playlists stay in `ux0:data/VitaDay/`; title ID remains `VDAY00001`.

## Changes since 0.10.20

- Added a local MP3 player with playlists on the left, tracks on the right and playback controls below, using your appearance colors. L/R includes Music in the main tab cycle.
- Added Favorites and user playlists: create, rename, delete, add/remove tracks and reorder. SELECT manages playlists; Square or three dots opens track actions. Deleting an MP3 from the card requires confirmation; removing from a playlist keeps the file.
- Added ID3 title/artist tags and embedded PNG/JPEG album artwork, with a fallback for missing or unsupported images. Covers decode in a separate thread. The mini-player shows the artist and cover too.
- Added Home playback controls and seek progress. Both players show elapsed time left and remaining time right; the full player's bar is centered. Playlist repeat is on by default, with shuffle, repeat-one and repeat-off options.
- Fixed navigation between playlists, tracks and transport controls, animated playback indication, macOS `._` files and invalid-MP3 filtering. Removed the in-app volume slider; use the Vita's physical volume buttons.
- Added sunlight, ozone and PM2.5 forecasts. Sunlight shows the instantaneous forecast with the preceding-hour average alongside it. Reworked weather text spacing, chart labels and daily summaries; corrected Simplified Chinese precipitation/pressure translations.
- Added value-based colors for numeric weather values and updated chart palettes for wind, humidity, ozone and PM2.5. Both unit systems share the same physical scale, with readable colors in light/dark themes. Pollutant colors follow EEA hourly concentration boundaries, not a combined AQI.
- Personal dates now use holiday-style calendar markers in their selected color.

## Copy your music

Copy MP3s into **`ux0:data/VitaDay/music/`** or **`ux0:music/`** with VitaShell, then open Music and choose **SELECT → Refresh**. Supports up to 256 MP3s, four nested folder levels and 64 playlists including Favorites. Music is local only; no account, API key or online music service is used. Closing the app or fully powering off stops playback. The Power/display-blank behavior still needs checking on physical hardware.

## Custom backgrounds

Copy PNG/JPG/JPEG files to `ux0:data/VitaDay/backgrounds/`, then choose their thumbnail in **Settings → Appearance → Background**. Recommended: **960 × 544**, maximum: **2048 × 2048**. Square rescans the gallery. Automatic theme mode offers separate day/night backgrounds. [Full instructions](https://github.com/s1zzle5486/VitaDay#add-your-own-background).

## English previews

Desktop renders with sample data, not console captures:

![Home](https://raw.githubusercontent.com/s1zzle5486/VitaDay/v0.10.31/docs/images/home.png)

![MP3 player](https://raw.githubusercontent.com/s1zzle5486/VitaDay/v0.10.31/docs/images/music.png)

![Weather](https://raw.githubusercontent.com/s1zzle5486/VitaDay/v0.10.31/docs/images/weather.png)

## Validation

Native VitaSDK build, ELF relocation checks, LiveArea checks and VPK integrity passed. The complete offline desktop suite passed with ASan/UBSan, including color-boundary and contrast checks. Weather layout was checked in all 20 languages. Native player/artwork/queue checks passed in Vita3K during earlier player builds; the full app has an existing emulator networking crash. This exact release has not been tested on physical Vita hardware.

VPK SHA-256: `7c98ab783bcb0809d6a4d054ba39411ce323df5cee4aea57d441cbce231c4d58`.
