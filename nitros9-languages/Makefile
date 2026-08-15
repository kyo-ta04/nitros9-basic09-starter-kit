ifndef NITROS9DIR
NITROS9DIR := $(abspath ../nitros9)
endif

NITROS9_LANGUAGES_DIR := $(CURDIR)

export NITROS9DIR
export NITROS9_LANGUAGES_DIR

include $(NITROS9DIR)/rules.mak

dirs = basic09 bf ccompiler
clean_dirs = $(addprefix clean-,$(dirs))

all: $(dirs)

$(dirs):
	$(MAKE) -C $@

clean: $(clean_dirs)

$(clean_dirs):
	$(MAKE) -C $(patsubst clean-%,%,$@) clean

.PHONY: all clean $(dirs) $(clean_dirs)
