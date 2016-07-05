@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"

set CommonCompilerFlages=-MT -Zi -nologo -Od -Oi -GR- -EHa- -Wall -WX -wd4189 -wd4820 -wd4201 -wd4100 -wd4191 -wd4710 -wd4514 -FC -DDEBUG_ON=1 -DSLOW_CODE=1 -DINTERNAL_BUILD=1
set CommonLinkerFlags=/link user32.lib gdi32.lib winmm.lib -subsystem:windows,5.1 -opt:ref


echo:
pushd ..\..\build
cl %CommonCompilerFlages% -Fmbase.map ..\Dropbox\source\base.cpp /link /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlages% -Fmwin32_base.map ..\Dropbox\source\win32_base.cpp %CommonLinkerFlags%
popd
echo:

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

:: -MT  ---  packages our necessary dll's into our exe
:: -nologo -Oi -GR- -EHa-  ---  turns off most of the compiling spit, turns on compiler intrinsics (so if the compiler knows assembly for things like sinf it uses it), turns off runtime var-type info (C++), turns off try-catch (C++)
:: -Fm  ---  lets you specify a map file (tells you where all the functions are in the executable)

::wd4189 - local var initialized but never referenced (optimized out anyway)
::wd4820 - padding warnings in structs (annoying and so far useless)
::wd4201 - nonstandard extension - a nameless struct or union (inside a struct)
::wd4100 - unreferenced formal parameter
::wd4191 - type cast - x_input_set_state getProcAddress stuff
::wd4710 - warning that an intended inline function was not inlined
::wd4514 - an inline function was removed bc it was never used

::subsystem:windows,5.1 = make it a 32-bit version that actually runs on xp (in a window) - for xp 64-bit use 5.2 instead, and add /MACHINE:x64
:: -opt:ref  ---  tells linker to attempt to remove unnecessary functions from final executable