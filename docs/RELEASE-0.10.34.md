# VitaDay 0.10.34 — fixes and faster startup

A small maintenance update for homebrew-enabled PS Vita.

## Changes

- Applied community corrections to the Simplified Chinese interface and holiday translations.
- Added handling for the system quit request, cancellation of active network requests, and orderly release of the music worker, audio port and system services when closing the app.
- Reduced startup work: load the selected language first, defer built-in photo thumbnails until Appearance is opened, and prepare fonts and circle masks when needed.
- Added startup and shutdown diagnostics in `ux0:data/VitaDay/startup-status.txt` and `shutdown-status.txt`.

Thanks to [Whale-Circus](https://github.com/Whale-Circus) for the Simplified Chinese corrections and the report about other apps failing to launch after exiting VitaDay.

## Install

Install **VitaDay-0.10.34.vpk** with VitaShell over your existing installation. Settings, events, cached data and playlists stay in `ux0:data/VitaDay/`; title ID remains `VDAY00001`.

## Validation

Native VitaSDK build, ELF relocation checks, LiveArea checks, VPK integrity and the complete offline ASan/UBSan test suite passed. On a physical Vita, application entry to first frame improved from **8.48 to 3.22 seconds**; the owner measured approximately **5 seconds** from tapping Start. System loading before application entry is outside the logged interval.

The system-wide app-launch issue after closing VitaDay still needs confirmation from affected hardware; the shutdown changes address that report, but this release does not claim that the original scenario has been verified as resolved.

SHA-256: `3e5c122b059b8163fcc92abbb95b63f51174db205295b98626d52560ed9e7217`.
