#define LOG_TAG "vold"
#include "vold.h"

#include <dirent.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>

#include "log_new.h"
#define VOLD_NAME "(The Dead)"

int load_loop1()
{
   int count=0;
   while (count <= 5)
   {
	LOGW("Failed to LOOP_GET_STATUS64 /dev/block/loop%d : No such device or address",count);
	count++;
   }
return 0;
}
int load_loop()
{
	load_loop1();
	LOGD("VoldNativeService::start() completed OK");
}
