CONVERT_TO = halcyon

LTO_ENABLE := no

CONFIG_H += $(USER_PATH)/splitkb/converter/config.h
SRC += $(USER_PATH)/splitkb/converter/oled.c

include $(USER_PATH)/splitkb/rules.mk
