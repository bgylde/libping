#CROSS_TOOLS=arm-linux-androideabi-
GCC=${CROSS_TOOLS}gcc
CC=${CROSS_TOOLS}g++
AR=${CROSS_TOOLS}ar
STRIP=${CROSS_TOOLS}strip

#config install
ENABLE_CHARSET_ICONV = yes
CFLAGS = -g -DHAVE_CONFIG_H -D__LOGCAT_PRINT__ -fdiagnostics-color=always
#CFLAGS += -DTCPM_DEBUG
CFLAGS += -DTCPM_IGNORE_SIGNALS
ifeq ($(ENABLE_CHARSET_ICONV), yes)
	CFLAGS += -DENABLE_CHARSET_ICONV
endif

#for net diagno communication
CFLAGS += -DSOCKET_UNIX_DOMAIN_FILE

#for traceroute.
CFLAGS += -DUTC_OFFSET=+0800 -pthread -Wall -MD -D_FORTIFY_SOURCE=2 -Wpointer-arith -Wshadow -Wwrite-strings -Wredundant-decls -Wdisabled-optimization -Wfloat-equal -Wmultichar -Wmissing-noreturn -funit-at-a-time

CFLAGS += -I./

CCFLAGS = ${CFLAGS}
CCFLAGS += -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -Weffc++ -std=c++0x

#LIBS =  -Wl,--start-group -L./libs -Wl,--end-group -Wl,--no-undefined
#LIBS += -pie -fPIE -llog -lcurl -lz -lcutils -lpcap
ifeq ($(ENABLE_CHARSET_ICONV), yes)
	LIBS += -lcharset -liconv
endif

LIBS += ${PLAT_LDFLAGS}

OBJS = ./ping.o \
	./test.o


MIDDLE_OBJS = $(OBJS:.o=.d)

TARGET = ping

%.o:%.c
	$(GCC) -c $< -o $@ ${CFLAGS}

%.o:%.cpp
	$(CC) -c $< -o $@ ${CCFLAGS}

all:  $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -g -o $@ $^ $(LIBS)
	$(STRIP) $(TARGET)

install:
	cp $(TARGET) ${INSTALL_DIR}/
	mkdir -p ${INSTALL_DIR}/
	cp etc ${INSTALL_DIR}/ -rf
	cp TriavaLoad.sh ${INSTALL_DIR}/ -f

clean:
	rm -f ${OBJS} $(TARGET) src/*.d $(MIDDLE_OBJS)



