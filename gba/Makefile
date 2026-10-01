#---------------------------------------------------------------------------------------------------------------------
# Party Royale for Game Boy Advance (Butano).
#
# `make` first runs tools/build_assets.py, which loads the browser game's own js/*.js and sprites/ and writes
# GBA-ready data to generated/ (maps, tiles, palettes, sprites, species). Then Butano builds party-royale.gba.
# See README.md for the toolchain.
#---------------------------------------------------------------------------------------------------------------------
TARGET      	:=  party-royale
BUILD       	:=  build
LIBBUTANO   	:=  butano/butano
PYTHON      	:=  python3
SOURCES     	:=  src butano/common/src
INCLUDES    	:=  include generated/include butano/common/include
DATA        	:=
GRAPHICS    	:=  generated/graphics butano/common/graphics
AUDIO       	:=
AUDIOBACKEND	:=  null
AUDIOTOOL		:=
DMGAUDIO    	:=
DMGAUDIOBACKEND	:=  null
ROMTITLE    	:=  PARTY ROYALE
ROMCODE     	:=  BPRY
USERFLAGS   	:=
USERCXXFLAGS	:=
USERASFLAGS 	:=
USERLDFLAGS 	:=
USERLIBDIRS 	:=
USERLIBS    	:=
DEFAULTLIBS 	:=
STACKTRACE		:=
USERBUILD   	:=  generated
EXTTOOL     	:=  @$(PYTHON) -B tools/build_assets.py --out=$(USERBUILD)

ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak
