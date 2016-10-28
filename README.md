# SigContrastFastAviUtl
Sigmodial contrast Aviutl plugin. IM is not used and only works on Y-channel

# Build Tips
Should be easy to build as there are no external dependency (except VS2015 redist). There is a property sheet in the project that set a macro for AviUtl's path. By default, DEBUG auf will be in the same folder as aviutl.exe, while RELEASE build will be put inside the \plugins folder.

# Warning: Dirty Code
A lot of useless code that doesn't being compiled are left behind; Commented out block left behind; so beware.

The Class "Histogram" is not being used at this moment.

# Known issues
* checkbox is not implemented
* parameter change may not reflect properly if you change the parameters while the plugin is disabled.
* UI language depends on Windows codepage, but Win10 mess up Chinese and Japanese fonts... not my problem.
* No multithreading: patch welcome
* It sucks: patch welcome
