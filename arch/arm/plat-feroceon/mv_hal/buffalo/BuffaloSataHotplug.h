#if !defined(_BUFFALO_SATA_HOTPLUG_H_)
#define _BUFFALO_SATA_HOTPLUG_H_

#include <linux/timer.h>
#include "mvTypes.h"

typedef enum _sata_plug_state {
	SATA_STAT_UNKNOWN,
	SATA_STAT_PLUGGED,
	SATA_STAT_UNPLUGGED,
}SATA_PLUG_STATE;

struct sata_hotplug_data {
	struct timer_list timer;
	MV_U32 pin;
	MV_BOOL prevpinstat;
	SATA_PLUG_STATE prevplugstat;
	unsigned int loops;
	void *dev_id;
};

#endif
