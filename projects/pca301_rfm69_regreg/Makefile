#
# PINKIE Project Makefile
#
# Defines the required components to compile for this project.
# PINKIE configuration is defined in pinkie_cfg.h
#
#
PROJECT = $(shell pwd)
PINKIE = $(PROJECT)/../..
SRC += \
    $(PROJECT)/main.c \
    $(PROJECT)/pca301.c \
    $(PROJECT)/pca301_rfm69.c

# required components
PINKIE_MOD_ACYCLIC = y
PINKIE_MOD_REGREG = y
PINKIE_MOD_REGREG_ACYCLIC = y
PINKIE_RADIO_RFM69 = y

export


all:
	@make --no-print-directory -C $(PINKIE) -f Makefile.main all


.DEFAULT:
	@make --no-print-directory -C $(PINKIE) -f Makefile.main $@
