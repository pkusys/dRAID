LIBS = -lpthread -lnuma -ldl -libverbs

SPDK_LIB_DIR := ~/draid-spdk/build/lib
COMMON_CFLAGS = -O0
PKG_CONFIG_PATH = $(SPDK_LIB_DIR)/pkgconfig
SPDK_DPDK_LIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --cflags spdk_event spdk_event_bdev spdk_env_dpdk)
SYS_LIB := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --static spdk_syslibs)

error:
	@echo "Please choose one of the following targets: xxx"

fio_plugin.o:
	cp ~/draid-spdk/examples/bdev/fio_plugin/fio_plugin.c . && \
	cc -o fio_plugin.o -MMD -MP -MF fio_plugin.d.tmp -g  -Wall -Wextra -Wno-unused-parameter \
		-Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing \
		-I ~/draid-spdk/include -march=native -Wformat -Wformat-security -D_GNU_SOURCE \
		-fPIC -fstack-protector -fno-common -I ~/draid-spdk/isa-l/.. -DNDEBUG -O2 \
		-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -DSPDK_GIT_COMMIT=7a8a39289 -pthread -Wno-pointer-sign \
		-Wstrict-prototypes -Wold-style-definition -std=gnu99 -I ~/fio \
		-c fio_plugin.c && mv -f fio_plugin.d.tmp fio_plugin.d && touch -c fio_plugin.o

spdk_bdev: fio_plugin.o ../../dRAID/host/bdev_raid_rpc.a
	g++ $(COMMON_CFLAGS) -o spdk_bdev -Wl,-z,relro,-z,now -Wl,-z,noexecstack -fuse-ld=bfd -shared -rdynamic -Wl,-z,nodelete fio_plugin.o \
		-L ~/draid-spdk/build/lib -Wl,--whole-archive -Wl,--no-as-needed -lspdk_bdev_malloc -lspdk_bdev_null -lspdk_bdev_nvme -lspdk_bdev_passthru \
		-lspdk_bdev_lvol -lspdk_bdev_error -lspdk_bdev_gpt -lspdk_bdev_split -lspdk_bdev_delay -lspdk_bdev_zone_block -lspdk_blobfs_bdev \
		-lspdk_blobfs -lspdk_blob_bdev -lspdk_lvol -lspdk_blob -lspdk_nvme -lspdk_dma -lspdk_rdma -lspdk_bdev_ftl -lspdk_ftl -lspdk_bdev_aio \
		-lspdk_bdev_virtio -lspdk_virtio -lspdk_accel_ioat -lspdk_ioat -lspdk_scheduler_dynamic -lspdk_env_dpdk -lspdk_scheduler_dpdk_governor \
		-lspdk_scheduler_gscheduler -lspdk_sock_posix -lspdk_event -lspdk_event_bdev -lspdk_bdev -lspdk_notify -lspdk_event_accel -lspdk_accel \
		-lspdk_event_vmd -lspdk_vmd -lspdk_event_sock -lspdk_init -lspdk_thread -lspdk_trace -lspdk_sock -lspdk_rpc -lspdk_jsonrpc -lspdk_json \
		-lspdk_util -lspdk_log -Wl,--no-whole-archive ~/draid-spdk/build/lib/libspdk_env_dpdk.a -Wl,-rpath-link ~/draid-spdk/dpdk/build/lib \
		-Wl,--whole-archive ../../dRAID/host/bdev_raid_rpc.a ~/draid-spdk/dpdk/build/lib/librte_bus_pci.a ~/draid-spdk/dpdk/build/lib/librte_cryptodev.a \
		~/draid-spdk/dpdk/build/lib/librte_eal.a ~/draid-spdk/dpdk/build/lib/librte_ethdev.a ~/draid-spdk/dpdk/build/lib/librte_hash.a \
		~/draid-spdk/dpdk/build/lib/librte_kvargs.a ~/draid-spdk/dpdk/build/lib/librte_mbuf.a ~/draid-spdk/dpdk/build/lib/librte_mempool.a \
		~/draid-spdk/dpdk/build/lib/librte_mempool_ring.a ~/draid-spdk/dpdk/build/lib/librte_net.a ~/draid-spdk/dpdk/build/lib/librte_pci.a \
		~/draid-spdk/dpdk/build/lib/librte_power.a ~/draid-spdk/dpdk/build/lib/librte_rcu.a ~/draid-spdk/dpdk/build/lib/librte_ring.a \
		~/draid-spdk/dpdk/build/lib/librte_telemetry.a ~/draid-spdk/dpdk/build/lib/librte_vhost.a -Wl,--no-whole-archive,-Bdynamic  -lnuma -ldl  \
		-libverbs -lrdmacm -L ~/draid-spdk/isa-l/.libs -lisal -pthread -lrt -luuid -lcrypto -lm -libverbs -lrdmacm -laio

clean:
	rm -f *.o *.d spdk_bdev