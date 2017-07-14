PROJECT:=timelog

O:=build

GCC:=gcc

all: $(O) $(O)/$(PROJECT)

$(O):
	mkdir -p $(O)

$(O)/%: %.c
	$(GCC) $(CFLAGS) -o $@ $<
