#------------------------------------------------------------------------
#  any question contact: tony.huang@linkeasy.com
#-------------------------------------------------------------------------
target :=libuart.a


all:$(target)
.PHONY : clean all

SRCDIR := ./src
INCDIR := ./inc
OBJDIR := ./obj

CC := gcc
RM := rm -f -r

CCFLAGS := -m64 -g -O1
CCFLAGS += -lpthread -I../../lib/include -I../include -I ../../pub
CCFLAGS += -L../../lib_x86_64 -lad7606

#CCFLAGS += -lpthread -I./include -I./libusb/include
#CCFLAGS += -L./libusb/lib -lusb-1.0


libsrc :=  $(wildcard ./*.c)
libobj :=  $(addprefix $(OBJDIR)/, $(notdir $(libsrc:.c=.o)))


$(target):$(libobj)
	ar -crv $@ $<

$(libobj) : $(OBJDIR)/%.o : ./%.c
	$(CC) -Wall -c $(CCFLAGS) $< -o $@

clean :
	$(RM) $(target)
	$(RM) $(OBJDIR)/*.o
