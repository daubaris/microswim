APPLICATION = microswim_example
BOARD ?= native

RIOTBASE ?= $(CURDIR)/RIOT
CFLAGS += -DLWIP_IPV4
CFLAGS += -DMICROSWIM_JSON=1
CFLAGS += -DETHARP_SUPPORT_STATIC_ENTRIES=1
CFLAGS += -DRIOT_OS

USEMODULE += microswim
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../../

USEMODULE += event_timeout
USEMODULE += event_thread
USEMODULE += event_thread_medium
USEMODULE += netdev_default
USEMODULE += netutils
USEMODULE += uuid
USEMODULE += ipv4_addr
USEMODULE += sock_ip
USEMODULE += sock_udp
USEMODULE += sock_async
USEMODULE += sock_async_event
USEMODULE += sock_util
USEMODULE += od
USEMODULE += lwip
USEMODULE += lwip_sock
USEMODULE += lwip_sock_udp
USEMODULE += lwip_sock_ip
USEMODULE += lwip_ipv4
USEMODULE += lwip_netif
USEMODULE += lwip_arp
USEMODULE += lwip_dhcp_auto
USEMODULE += posix_sockets
USEMODULE += posix_inet
USEMODULE += posix_select
USEMODULE += ztimer_sec
USEMODULE += ztimer_msec
USEMODULE += esp_wifi

USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += ps

DEVELHELP ?= 1

QUIET ?= 1

SRC += main.c
SRC += utils.c 

include $(RIOTBASE)/Makefile.include
