#########
#
# The top level targets link in the two .o files for now.
#
TARGETS += ledspi-server

LEDSPI_OBJS = util.o spio.o lib/cesanta/frozen.o lib/cesanta/mongoose.o

all: $(TARGETS) ledspi.service ledspi-service

ifeq ($(shell uname -m),armv7l)
# We are on the BeagleBone Black itself;
# do not cross compile.
export CROSS_COMPILE:=
else
# We are not on the BeagleBone and might be cross compiling.
# If the environment does not set CROSS_COMPILE, set our
# own.  Install a cross compiler with something like:
#
# sudo apt-get install gcc-arm-linux-gnueabi
#
export CROSS_COMPILE?=arm-linux-gnueabi-
endif

CFLAGS += \
	-std=c99 \
	-W \
	-Wall \
	-D_BSD_SOURCE \
	-Wp,-MMD,$(dir $@).$(notdir $@).d \
	-Wp,-MT,$@ \
	-I. \
	-O2 \
	-lm \
	-mtune=cortex-a8 \
	-march=armv7-a \
	-Wunused-parameter \
	-DNS_ENABLE_IPV6 \
	-Wunknown-pragmas \
	-Wsign-compare

LDFLAGS += \

LDLIBS += \
	-lm \
	-lpthread \

COMPILE.o = $(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $<
COMPILE.a = $(CROSS_COMPILE)ar crv $@ $^
COMPILE.link = $(CROSS_COMPILE)gcc $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(foreach O,$(TARGETS),$(eval $O: $O.o $(LEDSPI_OBJS)))

$(TARGETS):
	$(COMPILE.link)

ledspi.service: ledspi.service.in
	sed 's%LEDSPI_PATH%'`pwd`'%' ledspi.service.in > ledspi.service

ledspi-service: ledspi-service.in
	sed 's%LEDSPI_PATH%'`pwd`'%' ledspi-service.in > ledspi-service.sh
	chmod +x ledspi-service.sh

.PHONY: clean

clean:
	rm -rf \
		*.o \
		*.i \
		.*.o.d \
		*~ \
		$(TARGETS) \
		*.bin \
		ledspi.service \
		ledspi-service.sh

# Include all of the generated dependency files
-include .*.o.d
