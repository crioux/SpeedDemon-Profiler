#!/bin/sh


#### Build release directory structure

mkdir builds/$1
mkdir builds/$1/docs
mkdir builds/$1/lib
mkdir builds/$1/lib/WCE/
mkdir builds/$1/lib/WCE/PXA255
mkdir builds/$1/lib/WCE/PXA270
mkdir builds/$1/lib/Win32/
mkdir builds/$1/lib/Win32/X86
mkdir builds/$1/lib/Win64/
mkdir builds/$1/lib/Win64/x64


#### Copy files

# Documentation
cp docs/faq.pdf builds/$1/docs/faq.pdf
cp docs/manual.pdf builds/$1/docs/manual.pdf
cp docs/faq.doc builds/$1/docs/faq.doc
cp docs/manual.doc builds/$1/docs/manual.doc
cp docs/faq.odt builds/$1/docs/faq.odt
cp docs/manual.odt builds/$1/docs/manual.odt

# WCE/PXA255
cp speeddemonlib/SpeedDemonLib/PXA255/SpeedDemonLib.lib builds/$1/lib/WCE/PXA255/speeddemonlib.lib

# WCE/PXA270
cp speeddemonlib/SpeedDemonLib/PXA270/SpeedDemonLib.lib builds/$1/lib/WCE/PXA270/speeddemonlib.lib

# WIN32/X86
cp speeddemonlib/SpeedDemonLib/X86/SpeedDemonLib.lib builds/$1/lib/Win32/X86/speeddemonlib.lib

# WIN64/x64
cp speeddemonlib/SpeedDemonLib/x64/SpeedDemonLib.lib builds/$1/lib/Win64/x64/speeddemonlib.lib

# SPDReader
cp spdreader/Release/spdreader.exe builds/$1/spdreader.exe
cp spdreader/msdia80.dll builds/$1/msdia80.dll
cp c:/windows/system32/msvcp80.dll builds/$1/msvcp80.dll
cp c:/windows/system32/msvcr80.dll builds/$1/msvcr80.dll

