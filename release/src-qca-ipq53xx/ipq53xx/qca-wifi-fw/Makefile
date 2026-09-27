include ../qsdk.config

# Convert asuswrt build environment variable to qca-hk
PKG_BUILD_DIR	:= $(shell pwd)/source

# Define variables for qca-hk
INSTALLFWDIR	:= $(INSTALLDIR)/lib/firmware
INSTALLVENFWDIR	:= $(INSTALLDIR)/vendor/firmware
QCA5332_MSC	:= $(wildcard $(PKG_BUILD_DIR)/QCA5332/BIN_FILES/Data*.msc)
QCN9224_MSC	:= $(wildcard $(PKG_BUILD_DIR)/QCN9224/BIN_FILES/qcn9224/Data*.msc)
QCA5332_FW	:= $(wildcard $(PKG_BUILD_DIR)/QCA5332/BIN_FILES/*)
QCN9224_FW	:= $(wildcard $(PKG_BUILD_DIR)/QCN9224/BIN_FILES/qcn9224/*)

MODEL_QCA5332_FILES	:= $(patsubst %/REG_CTL,,$(wildcard $(BUILD_NAME)/QCA5332/*))
MODEL_QCN9224_FILES	:= $(patsubst %/REG_CTL,,$(wildcard $(BUILD_NAME)/QCN9224/*))

$(info BUILD_NAME=$(BUILD_NAME))
$(info MODEL_QCA5332_FILES=$(MODEL_QCA5332_FILES))
$(info MODEL_QCN9224_FILES=$(MODEL_QCN9224_FILES))

BIN2TXT:=	bdfUtil/bdfConvert/bin2txt.py
TXT2BIN:=	bdfUtil/bdfConvert/txt2bin.py
STDTXT:=	bdfUtil/bdfConvert/bdwlan.txt
NEWTXT:=	bdfUtil/bdfConvert/bdwlan_new.txt

MODEL_BIN_FILES := $(filter-out %_new.bin,$(wildcard \
	$(BUILD_NAME)/IPQ8074/bdwlan*.bin $(BUILD_NAME)/IPQ8074/REG_CTL/bdwlan*.bin \
	$(BUILD_NAME)/IPQ8074A/bdwlan*.bin $(BUILD_NAME)/IPQ8074A/REG_CTL/bdwlan*.bin))
MODEL_TXT_FILES := $(MODEL_BIN_FILES:.bin=.txt)
MODEL_PATCH_FILES := $(MODEL_BIN_FILES:.bin=.patch)
MODEL_NEW_BIN_FILES := $(MODEL_BIN_FILES:.bin=_new.bin)
MODEL_NEW_TXT_FILES := $(MODEL_NEW_BIN_FILES:.bin=.txt)

# Model-specific definition
BOARD_ID2_LIST	:=
ifneq ($(findstring $(BUILD_NAME),TUF-BE6500 TUF-BE9400),)
BOARD_ID2_LIST	:= b0015 b1019
endif

all:
	@#Nothing to do

install:
	@mkdir -p $(addprefix $(INSTALLVENFWDIR)/,IPQ5332 qcn9224)
	for f in $(notdir $(QCA5332_MSC)) ; do \
		ln -sf /lib/firmware/IPQ5332/$${f} $(INSTALLVENFWDIR)/IPQ5332/$${f} ; \
	done
	for f in $(notdir $(QCN9224_MSC)) ; do \
		ln -sf /lib/firmware/qcn9224/$${f} $(INSTALLVENFWDIR)/qcn9224/$${f} ; \
	done
	@mkdir -p $(addprefix $(INSTALLFWDIR)/,IPQ5332 qcn9224)
	rsync -avcH --chmod=F644 $(QCA5332_FW) $(INSTALLFWDIR)/IPQ5332
	$(if $(MODEL_QCA5332_FILES),rsync -avcH --chmod=F644 $(MODEL_QCA5332_FILES) $(INSTALLFWDIR)/IPQ5332)
	rsync -avcH --chmod=F644 $(QCN9224_FW) $(INSTALLFWDIR)/qcn9224
	$(if $(MODEL_QCN9224_FILES),rsync -avcH --chmod=F644 $(MODEL_QCN9224_FILES) $(INSTALLFWDIR)/qcn9224)
	# Disable QDSS tracing and remove space/tab lines that triggers warning msg of get_parameter_from_ini_file().
	for f in $(INSTALLDIR)/lib/firmware/*/firmware_rdp_feature*.ini ; do \
		sed -i -e "s,^\(.*\)_qdss_tracing=1,\1_qdss_tracing=0," \
		       -e "/^[ 	]\+$$/d" -e "/^$$/d" -e "/^#/d" $$f ; \
	done
ifneq ($(findstring $(BUILD_NAME),TUF-BE6500 TUF-BE9400),)
	ln -sf /tmp/caldata.bin $(INSTALLFWDIR)/IPQ5332/caldata.bin
	ln -sf /tmp/caldata_2.bin $(INSTALLFWDIR)/qcn9224/caldata_2.bin
	[ -z "$(BOARD_ID2_LIST)" ] || ($(foreach b,$(BOARD_ID2_LIST),ln -sf /tmp/caldata_2.bin $(INSTALLFWDIR)/qcn9224/caldata_2.$(b);))
endif

clean:
	@#Nothing to do

patch_bdwlan_txt: patch_bdwlan_txt.c
	$(CC) -Wall -o $@ $<

# model-specific bdwlan*.bin ==> bdwlan*.txt in same directory.
%.txt: %.bin
	python2 source/$(subst $(BUILD_NAME)/,,$(subst REG_CTL/,,$(dir $^)))$(BIN2TXT) $< \
		source/$(subst $(BUILD_NAME)/,,$(subst REG_CTL/,,$(dir $^)))$(STDTXT) && \
		mv -f source/$(subst $(BUILD_NAME)/,,$(subst REG_CTL/,,$(dir $^)))$(NEWTXT) $@

# generate difference between model-specific bdwlan*.txt and latest standard bdwlan.txt
%.patch: %.txt
	diff --unified=1 -I "COMMON_BDF_HEADER.baseBdfHeader.checksum" -I "COMMON_BDF_HEADER.baseBdfHeader.boardDataRev" \
		source/$(subst $(BUILD_NAME)/,,$(subst REG_CTL/,,$(dir $^)))$(STDTXT) $< > $@ || echo

# copy latest standard bdwlan.txt to model-specific bdwlan*_new.txt,
# # parsing .patch, seperate keyword from "+TYPE NAME VAL", search "TYPE NAME " in new txt and then modify it
%_new.txt: %.txt
	cp -f source/$(subst $(BUILD_NAME)/,,$(subst REG_CTL/,,$(dir $^)))$(STDTXT) $@
	@if [ -e "$(<:.txt=.patch)" ] && [ -n "`cat $(<:.txt=.patch)`" ] ; then \
		echo "$(<:.txt=.patch) remove `grep \"^-[^-]\" $(<:.txt=.patch)|wc -l` lines" \
			", add `grep \"^+[^+]\" $(<:.txt=.patch)|wc -l` lines. Applying ......"; \
		./patch_bdwlan_txt -p $(<:.txt=.patch) -b $@ ; \
	fi

# convert model-specific bdwlan*_new.txt to model-specific bdwlan*_new.bin
%_new.bin: %_new.txt
	python2 source/$(subst $(BUILD_NAME)/,,$(subst REG_CTL/,,$(dir $^)))$(TXT2BIN) $< $@

.PHONY: bin2txt txt2patch patch2newtxt newtxt2newbin apply_new_bin
bin2txt: $(MODEL_TXT_FILES)
	#touch $(MODEL_BIN_FILES)

txt2patch: $(MODEL_PATCH_FILES)
	#touch $(MODEL_TXT_FILES)

patch2newtxt: patch_bdwlan_txt $(MODEL_NEW_TXT_FILES)
	#touch $(MODEL_TXT_FILES)

newtxt2newbin: $(MODEL_NEW_BIN_FILES)
	#touch $(MODEL_NEW_TXT_FILES)

apply_new_bin:
	@for f in `find . -name "*_new.bin"` ; do \
		mv -f $${f} `echo $${f}|sed -e "s,_new,,"` ; \
	done
