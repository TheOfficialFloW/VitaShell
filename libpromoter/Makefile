ARCH ?= arm-vita-eabi
AS = $(ARCH)-as
AR = $(ARCH)-ar
RANLIB = $(ARCH)-ranlib

TARGETS = libScePromoterUtil_stub.a

ScePromoterUtil_OBJS = ScePromoterUtil_ScePromoterUtil_scePromoterUtilityInit.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityExit.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityDeletePkg.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityUpdateLiveArea.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityPromotePkg.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityPromotePkgWithRif.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityGetState.o ScePromoterUtil_ScePromoterUtil_scePromoterUtilityGetResult.o
SceKernel_OBJS =
ALL_OBJS=

all: $(TARGETS)

define LIBRARY_template
 $(1): $$($(1:lib%_stub.a=%)_OBJS)
 ALL_OBJS += $$($(1:lib%_stub.a=%)_OBJS)
endef

$(foreach library,$(TARGETS),$(eval $(call LIBRARY_template,$(library))))

all: $(TARGETS)

install: $(TARGETS)
	cp $(TARGETS) $(VITASDK)/arm-vita-eabi/lib

clean:

	rm -f $(TARGETS) $(ALL_OBJS)

$(TARGETS):
	$(AR) cru $@ $?
	$(RANLIB) $@

%.o: %.S
	$(AS) $< -o $@
