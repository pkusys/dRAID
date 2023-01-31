LIBS = -lpthread -lnuma -ldl -libverbs

SPDK_LIB_DIR := ~/draid-spdk/build/lib
COMMON_CFLAGS = -O0
PKG_CONFIG_PATH = $(SPDK_LIB_DIR)/pkgconfig
SPDK_DPDK_LIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --cflags spdk_event spdk_event_bdev spdk_env_dpdk)
SYS_LIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --static spdk_syslibs)

rpc_raid_main.o: rpc_raid_main.cc
	g++ -g $(COMMON_CFLAGS) -c rpc_raid_main.cc -o rpc_raid_main.o \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

rpc_raid_main: rpc_raid_main.o bdev_raid_rpc.a ../shared/common.h
	g++ -g $(COMMON_CFLAGS) -o rpc_raid_main rpc_raid_main.o \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		./bdev_raid_rpc.a \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB) $(LIBS)

bdev_raid.o: bdev_raid.cc ../shared/common.h bdev_raid.h
	g++ -g $(COMMON_CFLAGS) -c -fPIC bdev_raid.cc -o bdev_raid.o \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

raid5.o: raid5.cc ../shared/common.h bdev_raid.h
	g++ -g $(COMMON_CFLAGS) -c -fPIC raid5.cc -o raid5.o \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

raid6.o: raid6.cc ../shared/common.h bdev_raid.h
	g++ -g $(COMMON_CFLAGS) -c -fPIC raid6.cc -o raid6.o \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

bdev_raid_rpc.o: bdev_raid_rpc.cc bdev_raid.h
	g++ -g $(COMMON_CFLAGS) -c -fPIC bdev_raid_rpc.cc -o bdev_raid_rpc.o \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

bdev_raid_rpc.a: bdev_raid_rpc.o bdev_raid.o raid5.o raid6.o
	ar rcs bdev_raid_rpc.a bdev_raid_rpc.o bdev_raid.o raid5.o raid6.o

clean:
	rm -f *.o *.so *.a rpc_raid_main
