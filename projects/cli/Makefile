#
# PINKIE Project Makefile
#
# Defines the required components to compile for this project.
# PINKIE configuration is defined in pinkie_cfg.h
#
PROJECT = $(shell pwd)
PINKIE = $(PROJECT)/../..
SRC += $(PROJECT)/main.c

# required components
PINKIE_MOD_ACYCLIC = y

export


all:
	@make --no-print-directory -C $(PINKIE) -f Makefile.main all


test: all
	./tests/acyclic_testsuite
	@echo "\n\nTests successful\n"


.DEFAULT:
	@make --no-print-directory -C $(PINKIE) -f Makefile.main $@
