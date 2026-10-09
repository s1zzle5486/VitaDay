# VitaDay 0.10.20

Clock, weather and calendar dashboard for homebrew-enabled PS Vita.

Install **VitaDay-0.10.20.vpk** with VitaShell over your existing installation. Settings and personal dates are preserved in `ux0:data/VitaDay/` (title ID: `VDAY00001`).

## Changes since the GitHub version, 0.9.6

- Added all 20 Vita interface languages, automatic system-language selection, a full language picker, and a custom bitmap renderer for Japanese, Chinese and Korean characters.
- Improved weather charts with gradient fills and dashed past segments, continuous minute-position browsing, smooth stick acceleration, and fixes for gaps and dotted descending curves. Minute values interpolate hourly forecasts.
- Added offline warnings and cached weather fallback for additional cities, relative update times, and the initial chart cursor at the selected city's current time.
- Added localized control diagrams at the Vita screen's native resolution, clearer rounded panels, smoother background browsing, and scrollbars for lists.
- Expanded the selection of international observances and grouped matching multi-country holidays. International entries use the INT badge; individual source countries are available in the details.
- Fixed persistent holiday caching across countries, years and regions. A matching cached holiday list is reused without a daily redownload.
- Improved calendar event spacing, long-title handling, scrolling and the full-day reader. The Home banner shows the next event and a count of additional entries; selecting it opens that date in the calendar.
- Simplified Locations settings and moved personal date colors next to holiday options.
- Selecting an additional city on Home now opens its weather. The Remove city action and its explanation share a standard two-line card.
- Added the version label to the LiveArea launch card and documented custom backgrounds.

## Custom backgrounds

Copy PNG/JPG/JPEG files to `ux0:data/VitaDay/backgrounds/` using VitaShell, then select a thumbnail under **Settings → Appearance → Background**. Recommended size: **960 × 544**; maximum: **2048 × 2048**. Press Square in the gallery to rescan newly copied files. Automatic theme mode provides separate Day background and Night background selections. [Full instructions](https://github.com/s1zzle5486/VitaDay#add-your-own-background).

## Validation

Native VitaSDK build, executable relocation checks, VPK integrity and LiveArea checks passed. Offline desktop tests and UI previews passed. Screenshots are English desktop renders with sample data. This exact build has not yet been tested on a physical Vita.

VPK SHA-256:
`c27e8e8213a0283479508351484339b0dd14d8d359f8e7090b1a540b52ae8e34`

VPK size: 35,295,303 bytes.
