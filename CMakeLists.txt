cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)

##### begin project setup ######

SET(PROJECT_TITLE VitaShell)
SET(PROJECT_TITLEID VITASHELL)

SET(FTP_HOST 192.168.178.22)
SET(FTP_PORT 1337)

SET(PROJECT_SOURCES
	main.c
	init.c
	io_process.c
	package_installer.c
	network_update.c
	context_menu.c
	archive.c
	photo.c
	audioplayer.c
	file.c
	text.c
	hex.c
	sfo.c
	uncommon_dialog.c
	message_dialog.c
	ime_dialog.c
	config.c
	theme.c
	language.c
	utils.c
	sha1.c
	list_dialog.c
	minizip/unzip.c
	minizip/ioapi.c
	bm.c
	audio/vita_audio.c
	audio/player.c
	audio/id3.c
	audio/oggplayer.c
	audio/mp3player.c
	audio/mp3xing.c
	libmad/bit.c
	libmad/decoder.c
	libmad/fixed.c
	libmad/frame.c
	libmad/huffman.c
	libmad/layer12.c
	libmad/layer3.c
	libmad/stream.c
	libmad/synth.c
	libmad/timer.c
)

SET(PROJECT_RESOURCES
	archive_icon.png
	audio_icon.png
	base_head_bin.h
	battery_bar_charge.png
	battery_bar_green.png
	battery_bar_red.png
	battery.png
	changeinfo.txt
	colors.txt
	cover.png
	english_us.txt
	fastforward.png
	fastrewind.png
	file_icon.png
	folder_icon.png
	ftp.png
	image_icon.png
	pause.png
	play.png
	sfo_icon.png
	stop.png
	text_icon.png
	theme.txt
	updater_eboot.bin
	updater_param.bin
)


SET(PROJECT_LIBS
	vorbisfile 
	ogg 
	vorbis 
	ftpvita 
	vita2d 
	png 
	jpeg 
	z 
	m 
	c 
	SceAppMgr_stub 
	SceAppUtil_stub 
	SceCommonDialog_stub 
	SceCtrl_stub 
	SceDisplay_stub 
	SceGxm_stub 
	SceIme_stub 
	SceHttp_stub 
	SceKernel_stub 
	SceMusicExport_stub 
	SceNet_stub 
	SceNetCtl_stub 
	SceSsl_stub 
	SceSysmodule_stub 
	ScePhotoExport_stub 
	ScePower_stub 
	ScePgf_stub
	SceAudio_stub 
	SceAudiodec_stub 
	SceTouch_stub

	${CMAKE_CURRENT_SOURCE_DIR}/libpromoter/libScePromoterUtil_stub.a 
)

SET(PROJECT_EXTRA_JSON
	libpromoter/promoterutil.json
)


#####  end project setup  ######



project (${PROJECT_TITLE})

###### begin vitasdk setup ######

SET(VITASDK $ENV{VITASDK})

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

SET(CMAKE_VITA_PREFIX ${VITASDK}/bin/arm-vita-eabi)
set(CMAKE_C_COMPILER ${CMAKE_VITA_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${CMAKE_VITA_PREFIX}-g++)
set(CMAKE_STRIP ${CMAKE_VITA_PREFIX}-strip)
set(CMAKE_LD ${CMAKE_VITA_PREFIX}-ld)
set(CMAKE_AR ${CMAKE_VITA_PREFIX}-ar)
SET(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable" )

###### end vitasdk setup ######

add_executable(${PROJECT_TITLE}.elf ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_TITLE}.elf ${PROJECT_LIBS})

foreach(res ${PROJECT_RESOURCES})
	string(REPLACE "." "_" res_obj ${res})
	add_custom_command(
			OUTPUT ${res_obj}.o
			COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ${CMAKE_LD} -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/${res_obj}.o resources/${res}
	)
	target_sources(${PROJECT_TITLE}.elf PUBLIC ${res_obj}.o)
endforeach(res)


set(__SEND_ASSETS_COMMAND__ "echo;sending;resources")
set(__PACK_ASSETS_COMMAND__ "")
file(GLOB_RECURSE __RESOURCES__ RELATIVE ${CMAKE_SOURCE_DIR}/assets "assets/*")
foreach(asset ${__RESOURCES__})
	get_filename_component(path ${asset} DIRECTORY)
	set(abs_file "${CMAKE_SOURCE_DIR}/assets/${asset}")
	set(__PACK_ASSETS_COMMAND__ "${__PACK_ASSETS_COMMAND__};--a;${abs_file}=${asset}")
	if(NOT ${asset} MATCHES sce_sys)
		set(__SEND_ASSETS_COMMAND__ "${__SEND_ASSETS_COMMAND__};&&;echo;${asset};\;curl;--progress-bar;-T;${abs_file};ftp://${FTP_HOST}:${FTP_PORT}/ux0:/app/${PROJECT_TITLEID}/${path}")
	endif()
endforeach(asset)

set(__PROJECT_EXTRA_JSON__ "")
foreach(json ${PROJECT_EXTRA_JSON})
	list(APPEND __PROJECT_EXTRA_JSON__ ${CMAKE_CURRENT_SOURCE_DIR}/${json})
endforeach(json)

add_custom_command(
		OUTPUT ${PROJECT_TITLE}.velf
		COMMAND vita-elf-create ${PROJECT_TITLE}.elf ${PROJECT_TITLE}.velf ${__PROJECT_EXTRA_JSON__}
		DEPENDS ${PROJECT_TITLE}.elf
)
add_custom_target(
		velf
		DEPENDS ${PROJECT_TITLE}.velf
)

add_custom_command(
		OUTPUT eboot.bin
		COMMAND vita-make-fself ${PROJECT_TITLE}.velf eboot.bin
		DEPENDS velf
)
add_custom_target(
		eboot
		DEPENDS eboot.bin
)

add_custom_command(
		OUTPUT param.sfo
		COMMAND vita-mksfoex -s TITLE_ID="${PROJECT_TITLEID}" "${PROJECT_TITLE}" param.sfo
)
add_custom_target(
		sfo
		DEPENDS param.sfo
)

add_custom_command(
		OUTPUT ${PROJECT_TITLE}.vpk
		COMMAND vita-pack-vpk -s param.sfo -b eboot.bin ${__PACK_ASSETS_COMMAND__} ${PROJECT_TITLE}.vpk
		DEPENDS eboot
		DEPENDS sfo
)
add_custom_target(
		vpk
		DEPENDS ${PROJECT_TITLE}.vpk
)

add_custom_target(
		sendvpk
		COMMAND echo sending ${PROJECT_TITLE}.vpk to ${FTP_HOST}:${FTP_PORT}/ux0:/vpks/${PROJECT_TITLE}.vpk
		COMMAND curl --progress-bar -T ${PROJECT_TITLE}.vpk ftp://${FTP_HOST}:${FTP_PORT}/ux0:/data/vpks/
		DEPENDS ${PROJECT_TITLE}.vpk
)

add_custom_target(
		send
		COMMAND echo sending eboot.bin to ${FTP_HOST}:${FTP_PORT}/ux0:/app/${PROJECT_TITLEID}/eboot.bin
		COMMAND curl --progress-bar -T eboot.bin ftp://${FTP_HOST}:${FTP_PORT}/ux0:/app/${PROJECT_TITLEID}/
		DEPENDS eboot.bin
)

add_custom_target(
		sendassets
		COMMAND ${__SEND_ASSETS_COMMAND__}
		DEPENDS eboot.bin
)

add_custom_target(
		sendall
		DEPENDS send sendassets
)