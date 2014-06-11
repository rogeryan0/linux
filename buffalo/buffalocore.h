#if !defined(_BUFFALOCORE_H_)
#define _BUFFALOCORE_H_

#ifdef CONFIG_X86
#else // CONFIG_X86
#if defined(CONFIG_ARCH_FEROCEON_MV78XX0)
#include "boardEnv/buffalo/buffalo78100BoardEnv.h"
#include "boardEnv/mvBoardEnvSpec.h"
#endif

#include "boardEnv/mvBoardEnvLib.h"

#endif // CONFIG_X86
#if defined(CONFIG_ARCH_FEROCEON_KW) || defined(CONFIG_ARCH_ARMADA370) || defined(CONFIG_ARCH_ARMADA_XP)
struct bf_proc_entry_tab {
  const char *name;
  int isDirectory;
  read_proc_t *read_proc;
  write_proc_t *write_proc;
  void *data;
  MV_BOARD_GPP_CLASS class;
};
#endif

#endif
