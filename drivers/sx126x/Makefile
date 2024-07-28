SRC := sx126x.c
ifneq (,$(filter sx126x_radio_hal,$(USEMODULE)))
  SRC += sx126x_radio_hal.c
else
  SRC += sx126x_netdev.c
endif

include $(RIOTBASE)/Makefile.base
