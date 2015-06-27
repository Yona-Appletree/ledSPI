#########
#
# The top level targets link in the two .o files for now.
#
TARGETS += ledspi-server

LEDSPI_OBJS = util.o spio.o lib/cesanta/frozen.o lib/cesanta/mongoose.o

all: $(TARGETS) ledspi.service ledspi-service

export CROSS_COMPILE:=

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
