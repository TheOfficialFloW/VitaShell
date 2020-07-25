cmake_minimum_required(VERSION 2.8)

# VitaSDK defines
if( NOT DEFINED CMAKE_TOOLCHAIN_FILE )
  if( DEFINED ENV{VITASDK} )
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
  message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

# Project start
project(VitaShell)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "VitaShell")
set(VITA_TITLEID  "VITASHELL")
set(VITA_VERSION  "02.02")

# Flags and includes
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format-truncation -fno-lto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c+ + 11 -fno-rtti -fno-exceptions")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")
set(VITA_MAKE_FSELF_FLAGS "${VITA_MAKE_FSELF_FLAGS} -a 0x2808000000000000")

add_subdirectory(modules/kernel)
add_subdirectory(modules/user)
add_subdirectory(modules/patch)
add_subdirectory(modules/usbdevice)

include_directories(
  modules/kernel
  modules/user
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

# Builds
FUNCTION(ADD_RESOURCES out_var)
  SET(result)
  FOREACH(ref_f ${ARGN})
  if (IS_ABSOLUTE "${ref_f}")
    SET(out_f "${ref_f}.o")
    STRING(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" in_f "${ref_f}")
    SET(work_dir "${CMAKE_CURRENT_BINARY_DIR}")
  else()
    SET(out_f "${CMAKE_CURRENT_BINARY_DIR}/${ref_f}.o")
    SET(in_f "${ref_f}")
    SET(work_dir "${CMAKE_SOURCE_DIR}")
  endif()
  GET_FILENAME_COMPONENT(out_dir ${out_f} DIRECTORY)
  ADD_CUSTOM_COMMAND(OUTPUT ${out_f}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
    COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
    DEPENDS ${ref_f}
    WORKING_DIRECTORY ${work_dir}
    COMMENT "Building resource ${out_f}"
    VERBATIM
    )
  LIST(APPEND result ${out_f})
  ENDFOREACH()
  SET(${out_var} "${result}" PARENT_SCOPE)
ENDFUNCTION()

file(GLOB res_files RELATIVE
  ${CMAKE_SOURCE_DIR}
  resources/umass.skprx
  resources/default/*.png
  resources/default/*.txt
  resources/electron/*.png
  resources/electron/*.txt
  resources/*.png
  resources/*.txt
  resources/*.bin
)

# ugly hack
add_resources(vitashell_res
  ${res_files}
  ${CMAKE_CURRENT_BINARY_DIR}/modules/kernel/kernel.skprx
  ${CMAKE_CURRENT_BINARY_DIR}/modules/user/user.suprx
  ${CMAKE_CURRENT_BINARY_DIR}/modules/patch/patch.skprx
  ${CMAKE_CURRENT_BINARY_DIR}/modules/usbdevice/usbdevice.skprx
)

add_executable(VitaShell
  ${vitashell_res}
  main.c
  pfs.c
  main_context.c
  browser.c
  init.c
  usb.c
  qr.c
  io_process.c
  makezip.c
  package_installer.c
  refresh.c
  network_update.c
  network_download.c
  context_menu.c
  archive.c
  psarc.c
  photo.c
  audioplayer.c
  file.c
  text.c
  hex.c
  sfo.c
  rif.c
  sqlite3.c
  coredump.c
  settings.c
  adhoc_dialog.c
  property_dialog.c
  uncommon_dialog.c
  message_dialog.c
  netcheck_dialog.c
  ime_dialog.c
  config.c
  theme.c
  language.c
  utils.c
  elf.c
  sha1.c
  minizip/zip.c
  minizip/ioapi.c
  bm.c
  strnatcmp.c
  audio/vita_audio.c
  audio/player.c
  audio/id3.c
  audio/oggplayer.c
  audio/mp3player.c
  audio/mp3xing.c
  audio/lrcparse.c
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

add_dependencies(VitaShell vitashell_user_stubs)
add_dependencies(VitaShell kernel.skprx)
add_dependencies(VitaShell user.suprx)
add_dependencies(VitaShell patch.skprx)
add_dependencies(VitaShell usbdevice.skprx)

target_link_libraries(VitaShell
  ${CMAKE_CURRENT_BINARY_DIR}/modules/user/libVitaShellUser_stub_weak.a
  ftpvita
  vita2d
  vorbisfile
  vorbis
  ogg
  png
  jpeg
  z
  m
  c
  quirc
  onigmo
  archive
  bz2
  lzma
  crypto
  expat
  taihen_stub
  SceAppMgr_stub
  SceAppUtil_stub
  SceAudio_stub
  SceAudiodec_stub
  SceCamera_stub
  SceCommonDialog_stub
  SceCtrl_stub
  SceDisplay_stub
  SceFios2_stub
  SceGxm_stub
  SceIme_stub
  SceHttp_stub
  SceLibKernel_stub
  SceMtpIfDriver_stub
  SceMusicExport_stub
  SceNet_stub
  SceNetCtl_stub
  SceNpDrm_stub
  SceRegistryMgr_stub
  SceShellSvc_stub
  SceSqlite_stub
  SceSsl_stub
  SceSysmodule_stub
  ScePgf_stub
  ScePhotoExport_stub
  ScePower_stub
  ScePspnetAdhoc_stub
  ScePromoterUtil_stub
  SceTouch_stub
  SceUdcd_stub
  SceUsbstorVStorDriver_stub
  SceVideoExport_stub
  SceVshBridge_stub
)

# Create Vita artifacts
vita_create_self(eboot.bin VitaShell UNSAFE)
vita_create_vpk(VitaShell.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE pkg/sce_sys/icon0.png sce_sys/icon0.png
       pkg/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
       pkg/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
       pkg/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

# for network update
add_custom_target(release
  COMMAND cp eboot.bin ../release/eboot.bin
  COMMAND cp VitaShell.vpk_param.sfo ../release/param.sfo
  COMMAND cp VitaShell.vpk ../release/VitaShell.vpk
  COMMAND cp ../pkg/sce_sys/livearea/contents/template.xml ../release/template.xml
  DEPENDS eboot.bin
  DEPENDS VitaShell.vpk
  DEPENDS VitaShell.vpk_param.sfo
)

add_custom_target(send
  COMMAND curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/${VITA_TITLEID}/
  DEPENDS eboot.bin
)

add_custom_target(copy
  COMMAND cp eboot.bin G:/app/${VITA_TITLEID}/eboot.bin
  DEPENDS eboot.bin
)
