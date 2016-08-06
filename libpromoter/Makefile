TARGET_LIB=libScePromoterUtil_stub.a

$(TARGET_LIB): promoterutil.json
	-rm -rf build_lib
	mkdir build_lib
	vita-libs-gen $< build_lib
	cd build_lib && make
	mv build_lib/$(TARGET_LIB) $(TARGET_LIB)
	rm -rf build_lib

clean:
	rm $(TARGET_LIB)
