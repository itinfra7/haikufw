#ifndef HAIKUFW_TRACING_CONFIG_H
#define HAIKUFW_TRACING_CONFIG_H

/*
 * Minimal tracing feature toggles for standalone kernel add-on builds against
 * installed Haiku headers. The full Haiku source-tree build generates this
 * header, but the installed SDK does not ship it.
 */

#define SCHEDULING_ANALYSIS_TRACING 0
#define PAGE_ALLOCATION_TRACING 0
#define PAGE_ALLOCATION_TRACING_STACK_TRACE 0

#endif
