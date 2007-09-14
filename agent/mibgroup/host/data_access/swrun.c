/*
 *  Swrun MIB architecture support
 *
 * $Id: swrun.c 15768 2007-01-22 16:18:29Z rstory $
 */
/*
 * Copyright (C) 2007 Apple, Inc. All rights reserved.
 * Use is subject to license terms specified in the COPYING file
 * distributed with the Net-SNMP package.
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/data_access/swrun.h>


/**---------------------------------------------------------------------*/
/*
 * local static vars
 */
static int _swrun_init = 0;

/*
 * local static prototypes
 */
static void _swrun_entry_release(netsnmp_swrun_entry * entry,
                                            void *unused);

/**---------------------------------------------------------------------*/
/*
 * external per-architecture functions prototypes
 *
 * These shouldn't be called by the general public, so they aren't in
 * the header file.
 */
extern void netsnmp_arch_swrun_init(void);
extern int netsnmp_arch_swrun_container_load(netsnmp_container* container,
                                             u_int load_flags);

/**
 * initialization
 */
void
init_swrun(void)
{
    DEBUGMSGTL(("swrun:access", "init\n"));

    netsnmp_assert(0 == _swrun_init); /* who is calling twice? */

    if (1 == _swrun_init)
        return;

    _swrun_init = 1;

    netsnmp_arch_swrun_init();
}

void
shutdown_swrun(void)
{
    DEBUGMSGTL(("swrun:access", "shutdown\n"));

}

/**---------------------------------------------------------------------*/
/*
 * container functions
 */
/**
 * create swrun container
 */
netsnmp_container *
netsnmp_swrun_container_create(u_int flags)
{
    netsnmp_container *container;

    DEBUGMSGTL(("swrun:container", "init\n"));

    /*
     * create the container.
     */
    container = netsnmp_container_find("swrun:table_container");
    if (NULL == container)
        return NULL;

    container->container_name = strdup("swrun container");

    return container;
}

/**
 * load swrun information in specified container
 *
 * @param container empty container to be filled.
 *                  pass NULL to have the function create one.
 * @param load_flags flags to modify behaviour. Examples:
 *                   NETSNMP_SWRUN_ALL_OR_NONE
 *
 * @retval NULL  error
 * @retval !NULL pointer to container
 */
netsnmp_container*
netsnmp_swrun_container_load(netsnmp_container* user_container, u_int load_flags)
{
    netsnmp_container* container = user_container;
    int rc;

    DEBUGMSGTL(("swrun:container:load", "load\n"));
    netsnmp_assert(1 == _swrun_init);

    if (NULL == container)
        container = netsnmp_swrun_container_create(load_flags);
    if (NULL == container) {
        snmp_log(LOG_ERR, "no container specified/found for swrun\n");
        return NULL;
    }

    rc =  netsnmp_arch_swrun_container_load(container, load_flags);
    if (0 != rc) {
        if (NULL == user_container) {
            netsnmp_swrun_container_free(container, NETSNMP_SWRUN_NOFLAGS);
            container = NULL;
        }
        else if (load_flags & NETSNMP_SWRUN_ALL_OR_NONE) {
            DEBUGMSGTL(("swrun:container:load",
                        " discarding partial results\n"));
            netsnmp_swrun_container_free_items(container);
        }
    }

    return container;
}

void
netsnmp_swrun_container_free(netsnmp_container *container, u_int free_flags)
{
    DEBUGMSGTL(("swrun:container", "free\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_swrun_container_free\n");
        return;
    }

    if(! (free_flags & NETSNMP_SWRUN_DONT_FREE_ITEMS))
        netsnmp_swrun_container_free_items(container);

    CONTAINER_FREE(container);
}

void
netsnmp_swrun_container_free_items(netsnmp_container *container)
{
    DEBUGMSGTL(("swrun:container", "free_items\n"));

    if (NULL == container) {
        snmp_log(LOG_ERR, "invalid container for netsnmp_swrun_container_free_items\n");
        return;
    }

    /*
     * free all items.
     */
    CONTAINER_CLEAR(container,
                    (netsnmp_container_obj_func*)_swrun_entry_release,
                    NULL);
}

/**---------------------------------------------------------------------*/
/*
 * swrun_entry functions
 */
/**
 */
netsnmp_swrun_entry *
netsnmp_swrun_entry_get_by_index(netsnmp_container *container, oid index)
{
    netsnmp_index   tmp;

    DEBUGMSGTL(("swrun:entry", "by_index\n"));
    netsnmp_assert(1 == _swrun_init);

    if (NULL == container) {
        snmp_log(LOG_ERR,
                 "invalid container for netsnmp_swrun_entry_get_by_index\n");
        return NULL;
    }

    tmp.len = 1;
    tmp.oids = &index;

    return (netsnmp_swrun_entry *) CONTAINER_FIND(container, &tmp);
}

/**
 */
netsnmp_swrun_entry *
netsnmp_swrun_entry_create(int32_t index)
{
    netsnmp_swrun_entry *entry =
        SNMP_MALLOC_TYPEDEF(netsnmp_swrun_entry);

    if(NULL == entry)
        return NULL;

    entry->hrSWRunIndex = index;
    entry->hrSWRunType = 1; /* unknown */
    entry->hrSWRunStatus = 2; /* runnable */

    entry->oid_index.len = 1;
    entry->oid_index.oids = (oid *) & entry->hrSWRunIndex;

    return entry;
}

/**
 */
NETSNMP_INLINE void
netsnmp_swrun_entry_free(netsnmp_swrun_entry * entry)
{
    if (NULL == entry)
        return;

    /*
     * SNMP_FREE not needed, for any of these, 
     * since the whole entry is about to be freed
     */
    free(entry);
}

/**---------------------------------------------------------------------*/
/*
 * Utility routines
 */

/**
 */
static void
_swrun_entry_release(netsnmp_swrun_entry * entry, void *context)
{
    netsnmp_swrun_entry_free(entry);
}


#ifdef TEST
int main(int argc, char *argv[])
{
    const char *tokens = getenv("SNMP_DEBUG");

    netsnmp_container_init_list();

    /** swrun,verbose:swrun,9:swrun,8:swrun,5:swrun */
    if (tokens)
        debug_register_tokens(tokens);
    else
        debug_register_tokens("swrun");
    snmp_set_do_debugging(1);

    init_swrun();
    netsnmp_swrun_container_load(NULL, 0);
    shutdown_swrun();

    return 0;
}
#endif