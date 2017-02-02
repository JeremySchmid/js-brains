@echo off

set CommonCompilerFlages=-MTd -Zi -nologo -Od -Oi -Gm- -GR- -EHa- -Wall -WX -wd4505 -wd4189 -wd4820 -wd4201 -wd4100 -wd4191 -wd4710 -wd4514 -FC -DDEBUG_ON=1 -DSLOW_CODE=1 -DINTERNAL_BUILD=1
set CommonLinkerFlags=/link user32.lib gdi32.lib winmm.lib -subsystem:windows -opt:ref /MACHINE:x64

echo:
pushd ..\..\..\build\brains
del *.pdb >del-output.txt 2>del-error.txt
cl %CommonCompilerFlages% -LD -Fmbase.map -Fd ..\..\Dropbox\c\brains\base.cpp /link -incremental:no -PDB:handmade_%random%.pdb /EXPORT:GameGetSoundSamples /EXPORT:GameUpdate /EXPORT:GameRender
cl %CommonCompilerFlages% -Fmwin32_base.map ..\..\Dropbox\c\brains\win32_base.cpp %CommonLinkerFlags%
popd
echo:

:: -MT  ---  packages our necessary dll's into our exe
::-nologo -Oi  --- turns off most of the compiling spit, turns on compiler intrinsics (so if the compiler knows assembly for things like sinf it uses it),
:: -Gm- -GR- -EHa-  ---   turns off minimal rebuild stuff (rebuild everything now), turns off runtime var-type info (C++), turns off try-catch (C++)
:: -Fm  ---  lets you specify a map file (tells you where all the functions are in the executable)

::wd4189 - local var initialized but never referenced (optimized out anyway)
::wd4820 - padding warnings in structs (annoying and so far useless)
::wd4201 - nonstandard extension - a nameless struct or union (inside a struct)
::wd4100 - unreferenced formal parameter
::wd4191 - type cast - x_input_set_state getProcAddress stuff
::wd4710 - warning that an intended inline function was not inlined
::wd4514 - an inline function was removed bc it was never used
::wd4505 - a local function was removed bc it was never used

::subsystem:windows = make it a version that runs in a window (other option: console, which lets you use main as the beginning function)
:: -opt:ref  ---  tells linker to attempt to remove unnecessary functions from final executable