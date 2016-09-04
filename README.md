# VitaShell #

VitaShell is an alternative replacement of the PS Vita's LiveArea. It offers you a file manager, package installer, built-in FTP and much more.
This homebrew was an entry of the Revitalize PS Vita homebrew competition and won the first prize. HENkaku's molecularShell is also based on VitaShell.


### Customization ###
You can customize those files:
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/colors.txt'**: All colors adjustable
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/bg_browser.png'**: Background for file browser
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/bg_hexeditor.png'**: Background for hex editor
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/bg_texteditor.png'**: Background for text editor
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/bg_photoviewer.png'**: Background for photo viewer
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/wallpaper.png'**: Wallpaper #1
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/wallpaperX.png'**: Wallpaper #X (X is a value from 2-10)
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/dialog.png'**: Dialog menu image (Can be any size. This image file will be stretched by VitaShell to fit the dialog box. Suggestion: Don't use motives, as it will not look good with wrong proportion).
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/context.png'**: Context menu image (Can be any size. Suggestion: It will look great if you add alpha channel to your image).
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/battery.png'**: Battery border icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/battery_bar_green.png'**: Green battery bar
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/battery_bar_red.png'**: Red battery bar
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/battery_bar_charge.png'**: Charging battery bar
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/ftp.png'**: Ftp icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/audio_icon.png'**: Audio icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/archive_icon.png'**: Archive icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/file_icon.png'**: File icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/folder_icon.png'**: Folder icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/image_icon.png'**: Image icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/sfo_icon.png'**: SFO icon
- **'ux0:VitaShell/theme/YOUR_THEME_NAME/text_icon.png'**: Text icon


**Theme setting:** VitaShell will load the theme that is set in **'ux0:VitaShell/theme/theme.txt'** (THEME_NAME = "YOUR_THEME_NAME")

**General info:** You don't need to have all these files in your custom theme, if one of them is missing, the default image file will be loaded instead.

**Wallpapers info:** Wallpapers overlay background images. You can have **ten wallpapers** which VitaShell will display at random interval. If no wallpaper is available, **BACKGROUND_COLOR** from **'colors.txt'** will be used.

**Dialog and context image:** If these files are not available, the colors **DIALOG_BG_COLOR** and **CONTEXT_MENU_COLOR** from **'colors.txt'** will be used instead.

The standard VitaShell theme is provided in **'VitaShellCustomization.rar'** and available in the **'release'** section.

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
- turkish

VitaShell does automatically load the language that matches to the current system language.
If your system language is for example french, it will load from 'ux0:VitaShell/language/french.txt'.

The english language file is provided in **'VitaShellCustomization.rar'** and available in the **'release'** section.

### VitaShell themes and translations collection ###
This is an unofficial VitaShell themes and translations collection:

https://github.com/xy2iii/vitashell-themes

Be sure you pull request your customized design or language file there.

### Changelog 0.91 ###
- Added automatic network update. VitaShell will now notify you when there's a new update.
  You'll then be able to download it within the VitaShell application and it will update both
  molecularShell and VitaShell to the newest verison.
- Added text and audio file icon by littlebalup.
- Updated to latest libftpvita which fixed file size string > 2GB and added APPE command.

### Changelog 0.9 ###
- Added possibility to use specific background for file browser, hex editor, text editor, photo viewer.
- Added files and folder icons by littlebalup.
- Added charging battery icon by ribbid987.
- Added sfo reader by theorywrong.
- Added translation support for turkish (english_gb uses the same id as turkish, fix it Sony!).
- ~~Updated to latest libftpvita which fixed file size string > 2GB and added APPE command.~~
- Fixed bug where copied files and folders of archives didn't stay on clipboard.
- Allow auto screen-off.
- System information trigger combo changed to START instead of L+R+START.
  System information can now also be translated, thanks to littlebalup.

### Changelog 0.86 ###
- Added dialog box animation and aligned dialog box y to make it look better.
- Fixed wrong time string for files and folders. Thanks to persona5.
- Fixed INSTALL_WARNING text crash.
- Added default files creating.

### Changelog 0.85 ###
- Added customization possibility for ftp icon, battery, dialog and context menu.
- Added random wallpaper feature.
- Changed location of themes to 'ux0:VitaShell/theme/YOUR_THEME_NAME'.
- Fixed russian and korean language support.

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
