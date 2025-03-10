#define LOG_TAG "getprop"
#include <stdio.h>
#include <stdlib.h>

#include "log_new.h"

int main() {
  getprop();
  return 0;
}

int getprop(int argc, char *argv[], char *envp[]) {
  int i;
  for (i = 0; envp[i] != NULL; i++) LOGD("[%s]", envp[i]);
  getchar();
  exit(0);
  return 0;
}
