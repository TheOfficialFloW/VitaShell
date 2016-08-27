# VitaShell #

VitaShell is an alternative replacement of the PS Vita's LiveArea. It offers you a file manager, package installer, built-in FTP and much more.
This homebrew was an entry of the Revitalize PS Vita homebrew competition and won the first prize. HENkaku's molecularShell is also based on VitaShell.

### Donation ###
Any amount of donation is a big support for VitaShell development and therefore highly appreciated:
https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Y7CVA9VSJA2VW

### Customization ###
Put your colors file at **'ux0:VitaShell/theme/colors.txt'** and if wanted a PNG wallpaper file at **'ux0:VitaShell/theme/wallpaper.png'**.
If no wallpaper is available, **BACKGROUND_COLOR** from **'colors.txt'** will be used.
The standard VitaShell colors file is provided in the **'release'** section

### Multi-language ###
Put your language file at **'ux0:VitaShell/language/x.txt'**, **where the file must be UTF-8 encoded and 'x' is one of the language listed below:**

- japanese
- english_us
- french
- spanish
- german
- italian
- dutch
- portuguese
- russian
- korean
- chinese_t
- chinese_s
- finnish
- swedish
- danish
- norwegian
- polish
- portuguese_br
- english_gb

VitaShell does automatically load the language that matches to the current system language.
If your system language is for example french, it will load from 'ux0:VitaShell/language/french.txt'.
The english language file is provided in the **'release'** section

### VitaShell themes and translations collection ###
This is an unofficial VitaShell themes and translations host:

https://github.com/xy2iii/vitashell-themes

Be sure you send your customized design or language file to there.

### Changelog 0.8 ###
- Added support for >2GB zip archives (dropped support for 7zip and rar though).
- Added cache system for zipfs (faster file reading when browsing in zip archives).
- Added possibility to customize the application's UI.
- Added possibility to translate the application.
- Fixed 12h time conversion.

### Changelog 0.7 ###
- Ported to HENkaku (support for renaming/creating folders and for analog stick for fast movement).
- Added custom dialogs.
- Added graphics by Freakler.
- Added possibility to use FTP in background.
- I/O operations can now be cancelled.
- Removed misc stuff, shader compiler, homebrew loading, PBOOT.PBP signing, network host.
- Fixed various bugs.

### Changelog 0.6 ###
- Fixed size string of files, again.
- Optimized I/O operations regarding speed.

### Changelog 0.5 ###
- Increased homebrew force-exit compatbility and stability.
- Added network host mountpoint.
- Added ability to compile shader programs (use the _v.cg suffix for vertexes and _f.cg for fragments).
- Finished photo viewer. Use the right analog stick to zoom in/out. Left analog stick to move.
  L/R to rotate and X/O to change display mode.
- Updated to newest vita2dlib which fixed many bugs with images.
- Improved 'New folder' by extending to 'New folder (X)', where 'X' is an increasing number.
- Improved message dialog texts.
- Limited filenames so it doesn't overlap with the size/folder text. 
- Fixed infinite loop when copying the src to its subfolder by an error result.
- Fixed FTP client crashes and added support for Turboclient Android.
- Fixed alphabetical sorting, finally.

### Changelog 0.4 ###
- Added experimental feature: Holding START to force exit homebrews.
- Added battery symbol by Ruben_Wolf.
- Switched to official PGF font.
- Changed triangle-menu animation to ease-out.
- Improved mark all/unmark all feature.
- Fixed percentage precision in progress bar.
- Fixed small bug in move operation.

### Changelog 0.3 ###
- Added translation support. See translation_readme.txt for more details.
- Added move ability (only possible within same partition).
- Added tabulator support in text viewer.
- Removed 'Paste', 'Delete', 'Rename' and 'New folder' in read-only partitions.
- Fixed size string of files over 1GB.
- Fixed alphabetical sorting.
- Fixed battery percent bug being -1% on PSM Dev Assistant.

### Changelog 0.2 ###
- Added ability to sign PSP homebrews.
- Added sleep prevention when using FTP, deleting and copying files.
- Added a scrollbar.
- Added date and time to info bar.
- Added correct enter and cancel buttons assignment.
- Added some cosmetic changes.
- Fixed crash when deleting marked entries.
- Copied entries now still rest in clipboard after pasting them.
- The application now cleans itself before launching homebrews.

### In order to compile VitaShell you'll need ###
* vitasdk: https://github.com/vitasdk
* vita2dlib: https://github.com/xerpi/vita2dlib
* ftpvitalib https://github.com/xerpi/ftpvitalib

### Credits ###
* Team Molecule for HENkaku
* xerpi for ftpvitalib and vita2dlib
* wololo for the Revitalize contest
* Everybody who contributed on vitasdk
