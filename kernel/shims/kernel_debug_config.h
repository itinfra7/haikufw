#ifndef HAIKUFW_KERNEL_DEBUG_CONFIG_H
#define HAIKUFW_KERNEL_DEBUG_CONFIG_H

/*
 * Standalone kernel add-on builds on an installed Haiku system do not ship the
 * generated kernel_debug_config.h header that the full Haiku source-tree build
 * produces. The private kernel headers we need pull this file indirectly via
 * <debug.h>. For haikufw we only need the small subset of knobs referenced by
 * those headers.
 */

#define DEBUG_SPINLOCKS 0
#define B_DEBUG_SPINLOCK_CONTENTION 0

#endif
