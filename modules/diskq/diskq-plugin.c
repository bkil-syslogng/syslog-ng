/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
 */
#include "cfg-parser.h"
#include "plugin.h"
#include "plugin-types.h"

extern CfgParser diskq_parser;

static Plugin diskq_plugins[] =
{
  {
    .type = LL_CONTEXT_INNER_DEST,
    .name = "disk_buffer",
    .parser = &diskq_parser,
  },
};

#ifndef STATIC
const ModuleInfo module_info =
{
  .canonical_name = "disk_buffer",
  .version = SYSLOG_NG_VERSION,
  .description = "This module provides disk buffer based queuing mechanism",
  .core_revision = SYSLOG_NG_SOURCE_REVISION,
  .plugins = diskq_plugins,
  .plugins_len = G_N_ELEMENTS(diskq_plugins),
};
#endif

gboolean
disk_buffer_module_init(GlobalConfig *cfg, CfgArgs *args)
{
  plugin_register(cfg, diskq_plugins, G_N_ELEMENTS(diskq_plugins));
  return TRUE;
}
