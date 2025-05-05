/*
请不要修改此文件
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int has_noinit_flag(int *argc_ptr, char *argv[]) {
  int argc = *argc_ptr;
  int init_flag = 0;
  for (int i = 1; i < argc; i++) {
    if (!init_flag) {
      if (strcmp(argv[i], "--noinit") == 0) {
        init_flag = 1;
      }
    }
    if (init_flag && i < argc - 1) {
      argv[i] = argv[i + 1];
    }
  }
  *argc_ptr = argc - init_flag;
  return init_flag;
}