# VitaShell #

VitaShell is an alternative replacement of the PS Vita's LiveArea. It offers you a file browser, homebrew launcher, built-in FTP, network host feature and much more.
This homebrew was an entry of the Revitalize PS Vita homebrew competition and has won the 1. Price.
http://wololo.net/2015/12/23/ps-vita-revitalize-competition-and-the-winners-are/

### In order to run / compile VitaShell you need ###
* Rejuvenate: http://yifan.lu/p/rejuvenate/
* vitasdk: https://github.com/vitasdk / https://bintray.com/package/files/vitasdk/vitasdk/toolchain?order=desc&sort=fileLastModified&basePath=&tab=files
* vita2dlib: https://github.com/xerpi/vita2dlib
* ftpvitalib https://github.com/xerpi/ftpvitalib
* debugnet: https://github.com/psxdev/debugnet
* psp2link: https://github.com/psxdev/psp2link
* psp2client: https://github.com/psxdev/psp2client

### Credits ###
* yifanlu for Rejuvenate
* xerpi for ftpvitalib and vita2dlib
* psxdev (bigboss) for debugnet, psp2link and psp2client
* frangarcj and xyz for vita shader (https://github.com/frangarcj/vitahelloshader / https://github.com/xyzz/vita-shaders)
* wololo for the Revitalize contest
* Everybody who contributed on vitasdk


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