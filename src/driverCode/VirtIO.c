/*
 * Minimal virtio-mmio block driver for BaseOS: reads ONE 512-byte sector.
 * Handles both transport versions (1 = legacy, 2 = modern).
 * Submits the request and returns immediately — no polling.
 *
 * Queue memory (given by the core in bpt->queue_base / queue_size):
 *   legacy: base must be 4096-aligned, at least LEG_NEED bytes
 *   modern: base must be 16-aligned,   at least MOD_NEED bytes
 *   (reserving 8192 bytes aligned to 4096 satisfies both)
 */

#include <stdint.h>
#include "../../include/driverHeaders/generic_struct_ssd.h"
#include "../../include/driverHeaders/uart.h"         /* must declare:  uart_puts(const char *)  */

#define VIRTIO_DEBUG 1       /* set to 0 to silence the register/progress prints */

/* ---------- logging (virtio fails silently, so print what happens) ---------- */
#if VIRTIO_DEBUG
static void log_hex(const char *label, uint64_t v)
{
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    for (unsigned i = 0; i < 16; i++)
        buf[2 + i] = "0123456789abcdef"[(v >> (60 - 4 * i)) & 0xf];
    buf[18] = 0;
    uart_puts(label);
    uart_puts(buf);
    uart_puts("\r\n");
}
#define LOG(s)          uart_puts(s "\r\n")
#define LOGHEX(l, v)    log_hex(l, (uint64_t)(v))
#else
#define LOG(s)          ((void)0)
#define LOGHEX(l, v)    ((void)0)
#endif

/* ---------- memory barrier ---------- */
#if defined(__aarch64__)
#define BARRIER() __asm__ volatile("dmb sy" ::: "memory")
#elif defined(__riscv)
#define BARRIER() __asm__ volatile("fence rw,rw" ::: "memory")
#else
#define BARRIER() __sync_synchronize()
#endif

/* ---------- registers (all 32-bit) ---------- */
#define R_MAGIC               0x000
#define R_VERSION             0x004
#define R_DEVICE_ID           0x008
#define R_DEV_FEATURES        0x010
#define R_DEV_FEATURES_SEL    0x014
#define R_DRV_FEATURES        0x020
#define R_DRV_FEATURES_SEL    0x024
#define R_GUEST_PAGE_SIZE     0x028   /* legacy only */
#define R_QUEUE_SEL           0x030
#define R_QUEUE_NUM_MAX       0x034
#define R_QUEUE_NUM           0x038
#define R_QUEUE_ALIGN         0x03c   /* legacy only */
#define R_QUEUE_PFN           0x040   /* legacy only */
#define R_QUEUE_READY         0x044   /* modern only */
#define R_QUEUE_NOTIFY        0x050
#define R_INT_STATUS          0x060
#define R_INT_ACK             0x064
#define R_STATUS              0x070
#define R_QUEUE_DESC_LO       0x080   /* modern only */
#define R_QUEUE_DESC_HI       0x084
#define R_QUEUE_AVAIL_LO      0x090
#define R_QUEUE_AVAIL_HI      0x094
#define R_QUEUE_USED_LO       0x0a0
#define R_QUEUE_USED_HI       0x0a4
#define R_CAPACITY_LO         0x100   /* block config: size in sectors */
#define R_CAPACITY_HI         0x104

/* ---------- Status register bits ---------- */
#define ST_ACK          1u
#define ST_DRIVER       2u
#define ST_DRIVER_OK    4u
#define ST_FEATURES_OK  8u
#define ST_FAILED       128u

/* ---------- queue structures (layout fixed by the virtio spec) ---------- */
#define QN 4u                       /* queue size: 4 slots, one request uses 3 */

#define DESC_F_NEXT   1u
#define DESC_F_WRITE  2u            /* device may WRITE into this buffer */

struct virtq_desc {                 /* 16 bytes */
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};
struct virtq_avail {                /* written by us */
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[QN];
    uint16_t used_event;
};
struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
};
struct virtq_used {                 /* written by the device */
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[QN];
    uint16_t avail_event;
};
struct blk_req_hdr {                /* 16 bytes, first buffer of every request */
    uint32_t type;                  /* 0 = read */
    uint32_t reserved;
    uint64_t sector;
};

/* ---------- memory layout inside the queue block ---------- */
#define PAGE            4096u
#define LEG_USED_OFF    4096u                    /* used ring starts on QueueAlign */
#define LEG_HDR_OFF     (LEG_USED_OFF + 64u)
#define LEG_STATUS_OFF  (LEG_HDR_OFF + 16u)
#define LEG_NEED        (LEG_STATUS_OFF + 16u)
#define MOD_AVAIL_OFF   64u
#define MOD_USED_OFF    128u
#define MOD_HDR_OFF     192u
#define MOD_STATUS_OFF  208u
#define MOD_NEED        224u

/* ---------- driver state ---------- */
/*
 * g_ready has a NON-zero start value on purpose: it lands in .data, which is
 * part of the flat binary, so we never depend on .bss being zeroed.
 */
#define NOT_READY 0xA5A5A5A5u
#define READY     0x600DF00Du
static volatile uint32_t g_ready = NOT_READY;

static uintptr_t                    g_mmio;
static volatile struct virtq_desc  *g_desc;
static volatile struct virtq_avail *g_avail;
static volatile struct virtq_used  *g_used;
static volatile struct blk_req_hdr *g_hdr;
static volatile uint8_t            *g_status;
static uintptr_t                    g_hdr_addr;
static uintptr_t                    g_status_addr;
static uint16_t                     g_last_used;

static inline uint32_t rd(uintptr_t base, uint32_t off)
{
    return *(volatile uint32_t *)(base + off);
}
static inline void wr(uintptr_t base, uint32_t off, uint32_t val)
{
    *(volatile uint32_t *)(base + off) = val;
}

static int init_fail(uintptr_t m, int code)
{
    wr(m, R_STATUS, ST_FAILED);
    return code;
}

/* ---------- one-time setup ---------- */
static int virtio_init(const struct ssd_request_bpt *b)
{
    uintptr_t m = b->mmio_base;
    uintptr_t q = b->queue_base;

    uint32_t version = rd(m, R_VERSION);
    LOGHEX("virtio magic     : ", rd(m, R_MAGIC));      /* expect 0x74726976 */
    LOGHEX("virtio version   : ", version);             /* 1 legacy, 2 modern */
    LOGHEX("virtio device id : ", rd(m, R_DEVICE_ID));  /* expect 2 = block   */
    if (version != 1 && version != 2)
        return SSD_ERR_INIT;

    /* queue memory checks (the core gave us this block) */
    uint32_t need      = (version == 1) ? LEG_NEED : MOD_NEED;
    uint32_t align_req = (version == 1) ? PAGE : 16u;
    if ((q & (align_req - 1u)) != 0 || b->queue_size < need) {
        LOG("virtio: queue memory misaligned or too small");
        return SSD_ERR_QUEUE;
    }
    for (uint32_t i = 0; i < need; i += 4)              /* zero it ourselves */
        *(volatile uint32_t *)(q + i) = 0;

    /* handshake: reset, acknowledge, driver */
    wr(m, R_STATUS, 0);
    wr(m, R_STATUS, ST_ACK);
    wr(m, R_STATUS, ST_ACK | ST_DRIVER);

    uint32_t status = ST_ACK | ST_DRIVER;
    if (version == 2) {
        /* modern devices require the VERSION_1 feature (bit 32 = word 1, bit 0) */
        wr(m, R_DEV_FEATURES_SEL, 1);
        if ((rd(m, R_DEV_FEATURES) & 1u) == 0) {
            LOG("virtio: device lacks VERSION_1");
            return init_fail(m, SSD_ERR_INIT);
        }
        wr(m, R_DRV_FEATURES_SEL, 0);  wr(m, R_DRV_FEATURES, 0);
        wr(m, R_DRV_FEATURES_SEL, 1);  wr(m, R_DRV_FEATURES, 1);
        status |= ST_FEATURES_OK;
        wr(m, R_STATUS, status);
        if ((rd(m, R_STATUS) & ST_FEATURES_OK) == 0) {
            LOG("virtio: device rejected our features");
            return init_fail(m, SSD_ERR_INIT);
        }
    } else {
        wr(m, R_DRV_FEATURES, 0);                       /* accept no extras */
    }

    /* queue 0 */
    wr(m, R_QUEUE_SEL, 0);
    uint32_t max = rd(m, R_QUEUE_NUM_MAX);
    LOGHEX("virtio QueueNumMax: ", max);
    if (max < QN) {
        LOG("virtio: queue 0 missing or too small");
        return init_fail(m, SSD_ERR_QUEUE);
    }
    wr(m, R_QUEUE_NUM, QN);

    g_desc = (volatile struct virtq_desc *)q;
    if (version == 1) {
        g_avail = (volatile struct virtq_avail *)(q + 16u * QN);
        g_used  = (volatile struct virtq_used  *)(q + LEG_USED_OFF);
        g_hdr_addr    = q + LEG_HDR_OFF;
        g_status_addr = q + LEG_STATUS_OFF;
        wr(m, R_GUEST_PAGE_SIZE, PAGE);
        wr(m, R_QUEUE_ALIGN, PAGE);
        wr(m, R_QUEUE_PFN, (uint32_t)(q / PAGE));
    } else {
        g_avail = (volatile struct virtq_avail *)(q + MOD_AVAIL_OFF);
        g_used  = (volatile struct virtq_used  *)(q + MOD_USED_OFF);
        g_hdr_addr    = q + MOD_HDR_OFF;
        g_status_addr = q + MOD_STATUS_OFF;
        uint64_t d = (uint64_t)q;
        uint64_t a = (uint64_t)(q + MOD_AVAIL_OFF);
        uint64_t u = (uint64_t)(q + MOD_USED_OFF);
        wr(m, R_QUEUE_DESC_LO,  (uint32_t)d);  wr(m, R_QUEUE_DESC_HI,  (uint32_t)(d >> 32));
        wr(m, R_QUEUE_AVAIL_LO, (uint32_t)a);  wr(m, R_QUEUE_AVAIL_HI, (uint32_t)(a >> 32));
        wr(m, R_QUEUE_USED_LO,  (uint32_t)u);  wr(m, R_QUEUE_USED_HI,  (uint32_t)(u >> 32));
        wr(m, R_QUEUE_READY, 1);
    }
    g_hdr    = (volatile struct blk_req_hdr *)g_hdr_addr;
    g_status = (volatile uint8_t *)g_status_addr;
    g_mmio   = m;
    g_last_used = 0;

    status |= ST_DRIVER_OK;
    wr(m, R_STATUS, status);
    LOGHEX("virtio status    : ", rd(m, R_STATUS));
    LOGHEX("virtio capacity  : ", ((uint64_t)rd(m, R_CAPACITY_HI) << 32) | rd(m, R_CAPACITY_LO));
    return SSD_OK;
}

/* ---------- submit one read request (does not wait) ---------- */
static int virtio_read_sector(const struct ssd_request_bpt *b)
{
    /* 1. request header + status byte */
    g_hdr->type     = 0;                        /* read */
    g_hdr->reserved = 0;
    g_hdr->sector   = (uint64_t)b->sector_address;
    *g_status       = 0xFF;                     /* device overwrites: 0 ok, 1 io error, 2 unsupported */

    /* 2. three-descriptor chain: header -> data -> status */
    g_desc[0].addr  = (uint64_t)g_hdr_addr;
    g_desc[0].len   = 16;
    g_desc[0].flags = DESC_F_NEXT;
    g_desc[0].next  = 1;

    g_desc[1].addr  = (uint64_t)b->ram_buffer_address;
    g_desc[1].len   = 512;
    g_desc[1].flags = DESC_F_NEXT | DESC_F_WRITE;
    g_desc[1].next  = 2;

    g_desc[2].addr  = (uint64_t)g_status_addr;
    g_desc[2].len   = 1;
    g_desc[2].flags = DESC_F_WRITE;
    g_desc[2].next  = 0;

    /* 3. hand the chain (starting at descriptor 0) to the device */
    uint16_t idx = g_avail->idx;
    g_avail->ring[idx & (QN - 1u)] = 0;
    BARRIER();                                  /* device must see the chain before the new idx */
    g_avail->idx = (uint16_t)(idx + 1u);
    BARRIER();

    /* 4. ring the doorbell */
    wr(g_mmio, R_QUEUE_NOTIFY, 0);

    return SSD_OK;
}

/* ---------- entry point called through chosen_interface ---------- */
int virtio_ssd_driver(struct ssd_request_bpt *bpt)
{
    if (g_ready != READY) {
        int r = virtio_init(bpt);
        if (r != SSD_OK)
            return r;
        g_ready = READY;
    }
    return virtio_read_sector(bpt);
}
