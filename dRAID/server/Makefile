LIBS = -lpthread -lnuma -ldl -libverbs

SPDK_LIB_DIR := ~/draid-spdk/build/lib
COMMON_CFLAGS = -O0
PKG_CONFIG_PATH = $(SPDK_LIB_DIR)/pkgconfig
SPDK_DPDK_LIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --cflags spdk_event spdk_event_bdev spdk_env_dpdk)
SYS_LIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --static spdk_syslibs)

raid5: raid5.cc ../shared/common.h
	g++ -g $(COMMON_CFLAGS) -o raid5 raid5.cc \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

raid6: raid6.cc ../shared/common.h
	g++ -g $(COMMON_CFLAGS) -o raid6 raid6.cc \
		$(LIBS) \
		-pthread -Wl,--whole-archive,-Bstatic -I ~/draid-spdk/dpdk/build/include $(SPDK_DPDK_LIB) \
		-Wl,--no-whole-archive,-Bdynamic -I ~/draid-spdk/isa-l/include $(SYS_LIB)

clean:
	rm -f *.o raid5 raid6
