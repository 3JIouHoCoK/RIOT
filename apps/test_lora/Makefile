RIOTBASE ?= $(CURDIR)/../..
APPLICATION ?= test_lora
BOARD ?= lora-l432kc
BOARD_WHITELIST :=  \
					adafruit-clue \
					adafruit-itsybitsy-nrf52 \
					arduino-nano-33-ble \
					cc2538dk \
					feather-nrf52840 \
					native \
					nrf52840dk \
					nrf52840dongle \
					nrf52840-mdk \
					omote \
					openmote-cc2538 \
					reel \
					remote-pa \
					remote-reva \
					remote-revb \
					pba-d-01-kw2x \
					rak3172 \
          lora2w \
          lora-l432kc \
          weact-f411ce \
					#

DISABLE_MODULE += auto_init_at86rf2xx auto_init_nrf802154 auto_init_sx126x auto_init_sx127x


EXTERNAL_MODULE_DIRS += lora_sx127x_cmd
USEMODULE += lora_sx127x

USEMODULE += od
USEMODULE += luid
USEMODULE += l2util
USEMODULE += ieee802154
USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += ps
USEMODULE += event_thread
USEMODULE += event_callback
USEMODULE += xtimer
USEMODULE += netdev_default
USEMODULE += od_string
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=1024

include $(RIOTBASE)/Makefile.include

ifneq (,$(filter bhp,$(USEMODULE)))
  USEMODULE += bhp_event
endif
