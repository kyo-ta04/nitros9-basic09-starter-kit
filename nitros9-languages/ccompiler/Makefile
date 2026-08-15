include $(NITROS9DIR)/rules.mak

DSK		= CCompiler.dsk
CMDS		= cc1 c.prep c.pass1 c.pass2 c.opt c.asm c_asm c.link make
DIS_MODULES	= cc1 c.pass1 c.pass2 c.opt c.link make c.comp
HEADERS		= $(wildcard defs/*.h)
LIBRARIES	= $(wildcard lib/*.l lib/*.r)
C_SOURCES	= $(wildcard sources/*.c)
ASM_SOURCES	= $(wildcard sources/*.a)
SYS_SOURCES	= $(wildcard sources/*.sys)

.PHONY: all banner clean

# Make all components
all:	banner $(CMDS) $(DSK)

banner:
	@$(ECHO) "**************************************************"
	@$(ECHO) "*                                                *"
	@$(ECHO) "*                    C Compiler                  *"
	@$(ECHO) "*                                                *"
	@$(ECHO) "**************************************************"

# Clean all components
clean:
	$(RM) $(CMDS) $(DSK) *.list *.map

# Build the recovered, maintainable disassemblies instead of the raw byte dumps.
$(DIS_MODULES): %: %_dis.asm
	$(AS) $(AFLAGS) $< $(ASOUT)$@

c.asm: c_asm
	$(CP) $< $@

$(DSK): Makefile $(CMDS) $(HEADERS) $(LIBRARIES) $(C_SOURCES) \
	$(ASM_SOURCES) $(SYS_SOURCES)
	$(RM) $@
	$(OS9FORMAT_DS40) -q $@ -n"C Compiler"
	$(MAKDIR) $@,CMDS
	$(MAKDIR) $@,LIB
	$(MAKDIR) $@,DEFS
	$(MAKDIR) $@,SOURCES
	$(MAKDIR) $@,SOURCES/SYS
	$(OS9COPY) $(CMDS) $@,CMDS
	$(OS9ATTR_EXEC) $(foreach file,$(CMDS),$@,CMDS/$(file))
	$(CPL) $(HEADERS) $@,DEFS
	$(OS9COPY) $(LIBRARIES) $@,LIB
	$(CPL) $(C_SOURCES) $@,SOURCES
	$(CPL) $(ASM_SOURCES) $(SYS_SOURCES) $@,SOURCES/SYS
