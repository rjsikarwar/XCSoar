DRIVER_SRC_DIR = $(SRC)/Device/Driver

VOLKSLOGGER_SOURCES = \
	$(SRC)/Device/Volkslogger/CRC16.cpp \
	$(SRC)/Device/Volkslogger/Util.cpp \
	$(SRC)/Device/Volkslogger/dbbconv.cpp \
	$(SRC)/Device/Volkslogger/grecord.cpp \
	$(SRC)/Device/Volkslogger/vlapi2.cpp \
	$(SRC)/Device/Volkslogger/vlapihlp.cpp \
	$(SRC)/Device/Volkslogger/vlapisys_win.cpp \
	$(SRC)/Device/Volkslogger/vlconv.cpp

ifeq ($(HAVE_MSVCRT),n)
VOLKSLOGGER_SOURCES += $(SRC)/Device/Volkslogger/vlutils.cpp
endif

DRIVER_SOURCES = \
	$(VOLKSLOGGER_SOURCES) \
	$(DRIVER_SRC_DIR)/AltairPro.cpp \
	$(DRIVER_SRC_DIR)/BorgeltB50.cpp \
	$(DRIVER_SRC_DIR)/CAI302.cpp \
	$(DRIVER_SRC_DIR)/CaiGpsNav.cpp \
	$(DRIVER_SRC_DIR)/Condor.cpp \
	$(DRIVER_SRC_DIR)/EW.cpp \
	$(DRIVER_SRC_DIR)/EWMicroRecorder.cpp \
	$(DRIVER_SRC_DIR)/FlymasterF1.cpp \
	$(DRIVER_SRC_DIR)/Flytec.cpp \
	$(DRIVER_SRC_DIR)/Generic.cpp \
	$(DRIVER_SRC_DIR)/IMI.cpp \
	$(DRIVER_SRC_DIR)/LX.cpp \
	$(DRIVER_SRC_DIR)/Leonardo.cpp \
	$(DRIVER_SRC_DIR)/NmeaOut.cpp \
	$(DRIVER_SRC_DIR)/PosiGraph.cpp \
	$(DRIVER_SRC_DIR)/Vega.cpp \
	$(DRIVER_SRC_DIR)/PSMove.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger.cpp \
	$(DRIVER_SRC_DIR)/XCOM760.cpp \
	$(DRIVER_SRC_DIR)/ILEC.cpp \
	$(DRIVER_SRC_DIR)/Westerboer.cpp \
	$(DRIVER_SRC_DIR)/Zander.cpp

DRIVER_LIBS = $(TARGET_OUTPUT_DIR)/driver.a

$(DRIVER_LIBS): $(call SRC_TO_OBJ,$(DRIVER_SOURCES))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
