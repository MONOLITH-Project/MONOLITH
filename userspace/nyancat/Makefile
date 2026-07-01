TARGET := nyancat

APP_OPTFLAGS := -O2
APP_LIB_DEPS := libc
LIBS := $(addprefix $(BUILD_DIR)/libs/,$(addsuffix .a,$(APP_LIB_DEPS)))

include ../common-app.mk
