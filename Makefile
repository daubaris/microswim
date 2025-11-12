SRC += src/microswim.c
SRC += src/member.c
SRC += src/message.c
SRC += src/ping.c
SRC += src/ping_req.c
SRC += src/update.c
SRC += src/event.c

CFLAGS += -DMICROSWIM_JSON=1
CFLAGS += -DRIOT_OS=1

SRC += src/encode_json.c
SRC += src/decode_json.c

include $(RIOTBASE)/Makefile.base
