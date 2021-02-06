#!make

SHELL := /bin/bash
APPNAME=SkeletonCavern

.PHONY: help all linux_build macos_build info_plist iconset run

help: ## Shows this help.
	@IFS=$$'\n' ; \
	help_lines=(`fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//'`); \
	for help_line in $${help_lines[@]}; do \
		IFS=$$'#' ; \
		help_split=($$help_line) ; \
		help_command=`echo $${help_split[0]} | sed -e 's/^ *//' -e 's/ *$$//'` ; \
		help_info=`echo $${help_split[2]} | sed -e 's/^ *//' -e 's/ *$$//'` ; \
		printf "%-30s %s\n" $$help_command $$help_info ; \
	done

all: linux_build macos_build ## build linux and macos

linux_build: skeleton_cavern.c ## build x86_64 binary
	mkdir -p _build/x86_64
	cc skeleton_cavern.c -I/usr/local/include -L/usr/local/lib -lSDL2 -o _build/x86_64/skeleton_cavern.x86_64

macos_build: skeleton_cavern.c info_plist iconset ## build SkeletonCavern.app bundle
	mkdir -p _build/macos/$(APPNAME).app/Contents/MacOS
	cc skeleton_cavern.c -I/usr/local/include -L/usr/local/lib -lSDL2 -o _build/macos/$(APPNAME).app/Contents/MacOS/$(APPNAME)

info_plist: ## copy the Info.plist file to where it goes
	mkdir -p _build/macos/$(APPNAME).app/Contents/MacOS
	cp Info.plist _build/macos/$(APPNAME).app/Contents/Info.plist

iconset: ## build the SkeletonCavern.icns file and put it where it goes
	mkdir -p _build/macos/$(APPNAME).app/Contents/Resources
	mkdir -p _build/macos/$(APPNAME).iconset
	sips -z 16 16     $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_16x16.png
	sips -z 32 32     $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_16x16@2x.png
	sips -z 32 32     $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_32x32.png
	sips -z 64 64     $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_32x32@2x.png
	sips -z 128 128   $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_128x128.png
	sips -z 256 256   $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_128x128@2x.png
	sips -z 256 256   $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_256x256.png
	sips -z 512 512   $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_256x256@2x.png
	sips -z 512 512   $(APPNAME)Icon.png --out _build/macos/$(APPNAME).iconset/icon_512x512.png
	cp $(APPNAME)Icon.png _build/macos/$(APPNAME).iconset/icon_512x512@2x.png
	iconutil -c icns -o _build/macos/$(APPNAME).app/Contents/Resources/$(APPNAME).icns _build/macos/$(APPNAME).iconset
	rm -r _build/macos/$(APPNAME).iconset

run: linux_build ## run the linux.x86_64 binary
	./_build/x86_64/skeleton_cavern.x86_64
