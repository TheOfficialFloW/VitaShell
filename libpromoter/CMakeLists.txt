SET (TARGETLIB ${CMAKE_CURRENT_BINARY_DIR}/libScePromoterUtil_stub.a)
add_custom_command(OUTPUT ${TARGETLIB}
		COMMAND ${VITASDK}/bin/vita-libs-gen ${CMAKE_CURRENT_SOURCE_DIR}/promoterutil.json build_libs
		COMMAND ${CMAKE_MAKE_PROGRAM} -C build_libs
		COMMAND ${CMAKE_COMMAND} -E copy build_libs/libScePromoterUtil_stub.a ${TARGETLIB}
		COMMAND ${CMAKE_COMMAND} -E remove_directory build_libs
		)
add_custom_target(libpromoter_target DEPENDS ${TARGETLIB})
add_library(libpromoter STATIC IMPORTED GLOBAL)
add_dependencies(libpromoter libpromoter_target)
set_target_properties(libpromoter PROPERTIES IMPORTED_LOCATION ${TARGETLIB} INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})
