include $(NITROS9DIR)/rules.mak

DSK	= bf.dsk

SOURCEFILES := $(wildcard *.bf)
TEXTFILES	= README.md $(SOURCEFILES)
OBJS		= bf
ALLOBJS		= $(OBJS)

.PHONY: all banner clean showobjs showfiles

all:	banner $(ALLOBJS) $(DSK)

banner:
	@$(ECHO) "**************************************************"
	@$(ECHO) "*                                                *"
	@$(ECHO) "*                    Brainf*ck                   *"
	@$(ECHO) "*                                                *"
	@$(ECHO) "**************************************************"

bf: bf.asm
	$(AS) $(ASOUT)$@ $< $(AFLAGS) -DH6309=1

clean:
	$(RM) $(ALLOBJS) $(DSK) *.list *.map

$(DSK): Makefile $(ALLOBJS) $(TEXTFILES)
	$(RM) $@
	$(OS9FORMAT_SS35) $@ -n"bf"
	$(MAKDIR) $@,CMDS
	$(OS9COPY) $(ALLOBJS) $@,CMDS
	$(OS9ATTR_EXEC) $(foreach file,$(ALLOBJS),$@,CMDS/$(file))
	$(CPL) $(TEXTFILES) $@,.
	$(OS9ATTR_TEXT) $(foreach file,$(TEXTFILES),$@,$(file))

# Only $(CMDS) are shown here
showobjs:
	@$(ECHO) bf

# Only $(CMDS) are shown here
showfiles:
	@$(ECHO) $(SOURCEFILES)
