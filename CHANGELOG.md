# Changelog

### Changelog 2.02
- Reverted PSM refresh due to crashes.

### Changelog 2.01
- Added ability to refresh PSM games. Thanks to KuromeSan.
- Added support for FW 3.57 and 3.73 Thanks to RealYoti.

### Changelog 2.00
- Added support for FW 3.72.

### Changelog 1.99
- Added support for FW 3.71.

### Changelog 1.98
- Improved "Refresh LiveArea". Thanks to TheRadziu.

### Changelog 1.97
- Added support for symlinks which enables ability to set and show bookmarks and recent files. Thanks to abertschi.
- Added a few shortcuts: left+[]: show bookmarks, left+/\: show recent files, left+O: QR scanner.
- Fixed bug in ad-hoc dialog where X is always the enter button.

### Changelog 1.96
- Added ability to skip pages in file browser and SFO viewer using L/R triggers.
  (QR download has been temporarily removed and will be reimplemented later).
- Added ability to move files between partitions by copying&removing.
- Fixed bug where the option to disable warning message was not saved.
- Undone usb changes because people reported an issue with vpk installation with it.

### Changelog 1.95
- Added option to disable warning messages when installing vpks.
- Fixed bug in USB connection, where your Memory Card could be corrupted.
- Fixed line breaks in SFO files and long names will now scroll.
- Fixed compatibility with `udcd_uvc.skprx ` thanks to xerpi.

### Changelog 1.94
- Added ability to umount all partitions that you can mount.
- Fixed crash when using StorageMgr.

### Changelog 1.93
- Added xmc0: to partition list and added ability to mount xmc0:.
- Fixed ux0: mounting bug where it wasn't detected that the gamesd has already been mounted.

### Changelog 1.92
- Added ability to mount gamesd ux0:
- Fixed bug where the VitaShell kernel module was loaded but not started, thus it'd run out of memory after some launches.

### Changelog 1.91
- Added ability to create new files, thanks to abertschi.
- Added ability to skip last dir by holding L on startup.
- Added support for firmware 3.68.

### Changelog 1.9
- Added ability to open decrypted trophys, savedatas and appmetas.
- Moved `Open Decrypted` entry to main context menu for faster access.

### Changelog 1.83
- Fixed automatic network update by using rawgit. This was needed because github removed support for TLS 1.0/1.1.
- Removed changelog support, because the files at ux0:patch must now be pfs encrypted.

### Changelog 1.82
- Added support for firmware 3.65/3.67.

### Changelog 1.81
- Added adhoc file transfer feature.
- Fixed bug where it didn't count files and folders in archives correctly.
- Improved controls code.

### Changelog 1.8
- Aligned memory to 64bit for optimal I/O.
- Fixed crash when using FTP.

### Changelog 1.79
- Added ability to open .psarc files.
- Added support for multi volume rar archives.
- Added support for password encrypted zip archives.
- Added support for file property in archives, now it can show size and contains.
- Added natural sorting for filenames.
- Fixed bug where size of files in archives over 2GB where shown as 16EB.
- Fixed refresh license DB when ran from ur0:.

### Changelog 1.78
- Fixed bug where archives > 2GB couldn't be opened.

### Changelog 1.77
- Added archive support for .7z, .tar.gz, .tar.bz2 and more by using libarchive.
- Improved archive file system (opening archives should now be much faster).
- Fixed keyboard freeze after using it multiple times.
- Fixed file browser cursor correction.
- Fixed Electron theme visual glitch and added new settings look (thanks to Acemad).

### Changelog 1.76
- Added DLC refresh + restore license from Sqlite DB by VitaSmith.
- Added license database refresh by VitaSmith.
- Fixed C2-12828-1 error.

### Changelog 1.75
- Fixed bug where Memory Card was umounted after closing USB (fixed `The file is corrupt` error).
- Fixed QR download of URLs that link to files.

### Changelog 1.74
- Fixed QR download and C2-13696-5 error.

### Changelog 1.73
- Fixed crash when you pressed triangle in the home menu.

### Changelog 1.72
- Fixed pfs mounting bug where you could not export files from specific folders.
- Fixed bug where files from https could not be downloaded using QR code.
- Fixed bug with pvsd usb device mounting where microSD could not be found.

### Changelog 1.71
- Fixed pvsd usb device mounting.
- Fixed bug where uma0: options were grayed out on non PS TV models.
- Fixed bug where the QR code scanner showed the result twice.

### Changelog 1.7
- Added QR Code scanner and file downloader by cxziaho (press L+R to open the camera).
- Added option in `More->` to open applications in decrypted mode (bypass PFS). Thanks to frangarcj.
- Added ability to mount sd2vita and psvsd as usb device.
- Fixed bug where `imc0:` could not be mounted if the Memory Card was not available.
- Fixed bug where accessing the settings menu crashed if there was no Memory Card available.
- Fixed bug where automatic usb connection was not blocked, after leaving and resuming VitaShell.
- PS TV will now always open FTP when pressing SELECT.

### Changelog 1.63
- Added ability to export video files.
- Added support for PSVSD uma0: mounting.
- Fixed bug where the right analog stick was simulated when the headphone was plugged in.

### Changelog 1.62
- Fixed bug where the cursor was moving too fast when the headphone was plugged in.

### Changelog 1.61
- Fixed database corruption crash by inheriting `calendar`, `mms` and `mtp` when mounting USB ux0:.
- Fixed bug where insufficent memory dialog appeared on uma0:.
- Fixed bug where pressing triangle crashed if unsafe mode was disabled.

### Changelog 1.60
- Added ability to mount uma0: and ability to use uma0: as ux0:.
- Added ability to mount game card as usb device.
- Added possibility to refresh the livearea.
- Added scrolling text for long filenames.
- Added `Sort by` option to context menu (R trigger combo removed).
- Added `Electron` theme by Acemad, the winner of the VitaShell theme contest.
- Added theme selector.
- Improved property dialog animation.
- Fixed text editor bugs.
- Fixed bug where USB cable wasn't recognized.
- Fixed bug where VitaShell left to livearea after deleting the updater.
- Removed battery icon in status bar for PSTV.
- File statistics are now inherited when copying.
- I/O operations speed will now be showed in KB/s.
- Made control smoother.

### Changelog 1.51
- Fixed bug where `Please wait...` was shown instead of a specifc USB message.
- Fixed VPK installation crash.
- Fixed bug where `theme.txt` was always resetted.
- Updated to newest libftpvita.

### Changelog 1.50
- Added USB mass storage transfer support.
- Added RAR archive support (by Mayoshiroi).
- Added coredump viewer.

### Changelog 1.43
- Added taiHEN config.txt reload option available in molecularShell.
- Changed default HENkaku spoofed version to 3.63.
- Fixed bug where you couldn't exit the application.

### Changelog 1.42
- Added a brand new settings menu which is accessible by pressing START (design by WesleyPolicena).
  This adds the ability to disable auto-update and power options have been added where you can
  reboot, enter standby or turn off your device.
- Fixed local date being incorrect.
- The name `molecularShell` will now be displayed if it's runned as molecularShell, otherwise `VitaShell`.
  This will help differentiating the two applications, as molecularShell is from now on a safe homebrew
  and doesn't have the full range of features like VitaShell. Additionally in molecularShell you can see
  the HENkaku settings section which are hidden in VitaShell (note that both still share the same eboot.bin,
  only the flag is different).

### Changelog 1.41
- Added ability to sort files and folders by size and date. Press R to switch sort method.
- Added the long desired `Properties` feature. You can also check whether an eboot.bin is safe or not.

### Changelog 1.4
- Added group RW permissions on files and folders when moving.
  Safe homebrews like RetroArch will now recognize files and folders
  that you have moved from `ux0:video`.
- Added scanning for dangerous functions in packages.
- Added possibility to choose compression level.
- Fixed time information in zip archives.

### Changelog 1.31
- Touching the screen on dialogs would abort the process, fixed.

### Changelog 1.3
- Added ability to compress files and folders into a zip archive.
- Added scanning for unsafe fself's and imports.
- Added lrc support for audio player by Mayoshiroi.
- Added partition used/total size information, thanks to littlebalup.
- Added `Install folder` by soarqin.

### Changelog 1.2
- Added ability to export photo and music files (`More` -> `Export media`).
- Added battery percentage to system information (thanks to littlebalup).
- Added toolbox where more features will have place, by theorywrong (press START).
- Improved stability for file browser delete operation.
- Pressing L in the music player will now restart the song if the song is not at the beginning.
- VitaShell will now remove the unused updater bubble after successful updating.

### Changelog 1.1
- Added cover.jpg and folder.jpg as alternative cover image.
- Fixed ID3 tag parsing.
- Play and pause icons where swapped, fixed.
- FTP does now show `Please wait...` until it has been connected to Wi-Fi.

### Changelog 1.0
- Added audio player with .MP3 and .OGG support (Press /\ to turn off the display while listening to music. Graphics by Valle).
- Added changelog to the VitaShell bubble (press /\ or hold the bubble of VitaShell in the livearea).
- Added ability to resume to the directory you've last visted before exiting VitaShell).
- Added memory card free space check. A message will pop up if you don't have enough free space for copying or installing.
- Leaving to livearea or suspending the device does no longer corrupt an IO process anymore (while copying things you can now leave VitaShell and resume).
- Improved text editor and added search feature to context menu (press /\).
- The temporary folder `ptmp/pkg` is now cleared after cancelling or unsuccesful installation.
- Fixed bug where copying to different partitions didn't work.
- Fixed network update bug.

### Changelog 0.95
- Added ability to install update files as .vpk (for Vitamin).
- Added patch to make .vpk installation appearing as full version instead of test version.
- Added text editor by BigBoot (WIP).
- Added `More` entry to the context menu.
- Added `Install all` feature to install all packages available in the folder (by ribbid987).
- Added `Calculate SHA1` feature by xerpia64.
- Added support for ftp promoting for https://github.com/soywiz/vitaorganizer.
- Fixed `Move` operation. Now it does integrate into folders and replace files.
- Dropped GENERAL_COLOR, now all colors are adjustable.

### Changelog 0.91
- Added automatic network update. VitaShell will now notify you when there's a new update.
  You'll then be able to download it within the VitaShell application and it will update both
  molecularShell and VitaShell to the newest verison.
- Added text and audio file icon by littlebalup.
- Updated to latest libftpvita which fixed file size string > 2GB and added APPE command.

### Changelog 0.9
- Added possibility to use specific background for file browser, hex editor, text editor, photo viewer.
- Added files and folder icons by littlebalup.
- Added charging battery icon by ribbid987.
- Added sfo reader by theorywrong.
- Added translation support for turkish (english_gb uses the same id as turkish, fix it Sony!).
- Fixed bug where copied files and folders of archives didn't stay on clipboard.
- Allow auto screen-off.
- System information trigger combo changed to START instead of L+R+START.
  System information can now also be translated, thanks to littlebalup.

### Changelog 0.86
- Added dialog box animation and aligned dialog box y to make it look better.
- Fixed wrong time string for files and folders. Thanks to persona5.
- Fixed INSTALL_WARNING text crash.
- Added default files creating.

### Changelog 0.85
- Added customization possibility for ftp icon, battery, dialog and context menu.
- Added random wallpaper feature.
- Changed location of themes to `ux0:VitaShell/theme/YOUR_THEME_NAME`.
- Fixed russian and korean language support.

### Changelog 0.8
- Added support for >2GB zip archives (dropped support for 7zip and rar though).
- Added cache system for zipfs (faster file reading when browsing in zip archives).
- Added possibility to customize the application's UI.
- Added possibility to translate the application.
- Fixed 12h time conversion.

### Changelog 0.7
- Ported to HENkaku (support for renaming/creating folders and for analog stick for fast movement).
- Added custom dialogs.
- Added graphics by Freakler.
- Added possibility to use FTP in background.
- I/O operations can now be canceled.
- Removed misc stuff, shader compiler, homebrew loading, PBOOT.PBP signing, network host.
- Fixed various bugs.

### Changelog 0.6
- Fixed size string of files, again.
- Optimized I/O operations regarding speed.

### Changelog 0.5
- Increased homebrew force-exit compatbility and stability.
- Added network host mountpoint.
- Added ability to compile shader programs (use the _v.cg suffix for vertexes and _f.cg for fragments).
- Finished photo viewer. Use the right analog stick to zoom in/out. Left analog stick to move.
  L/R to rotate and X/O to change display mode.
- Updated to newest vita2dlib which fixed many bugs with images.
- Improved `New folder` by extending to `New folder (X)`, where `X` is an increasing number.
- Improved message dialog texts.
- Limited filenames so it doesn't overlap with the size/folder text. 
- Fixed infinite loop when copying the src to its subfolder by an error result.
- Fixed FTP client crashes and added support for Turboclient Android.
- Fixed alphabetical sorting, finally.

### Changelog 0.4
- Added experimental feature: Holding START to force exit homebrews.
- Added battery symbol by Ruben_Wolf.
- Switched to official PGF font.
- Changed triangle-menu animation to ease-out.
- Improved mark all/unmark all feature.
- Fixed percentage precision in progress bar.
- Fixed small bug in move operation.

### Changelog 0.3
- Added translation support. See translation_readme.txt for more details.
- Added move ability (only possible within same partition).
- Added tabulator support in text viewer.
- Removed `Paste`, `Delete`, `Rename` and `New folder` in read-only partitions.
- Fixed size string of files over 1GB.
- Fixed alphabetical sorting.
- Fixed battery percent bug being -1% on PSM Dev Assistant.

### Changelog 0.2
- Added ability to sign PSP homebrews.
- Added sleep prevention when using FTP, deleting and copying files.
- Added a scrollbar.
- Added date and time to info bar.
- Added correct enter and cancel buttons assignment.
- Added some cosmetic changes.
- Fixed crash when deleting marked entries.
- Copied entries now still rest in clipboard after pasting them.
- The application now cleans itself before launching homebrews.
