/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <common.h>

extern uint64_t g_nr_guest_inst;

char *insert_string(const char *original, const char *insert, size_t position);
char *find_last_substr_ptr(const char *str, const char *substr);
int find_last_substr_idx(const char *str, const char *substr);
FILE * get_log_file(const char* log_fpath, char * fname_suffix);


#ifndef CONFIG_TARGET_AM
FILE *log_fp = NULL;
#ifdef CONFIG_ITRACE_IRINGBUF
FILE *log_ifp = NULL;
#endif
#ifdef CONFIG_MTRACE
FILE *log_mfp = NULL;
#endif
#ifdef CONFIG_FTRACE
FILE *log_ffp = NULL;
FILE *elf_fp = NULL;
#include <elf.h>
#endif

void init_log(const char *log_fpath)
{
  log_fp = stdout;
  if (log_fpath != NULL)
  {
    FILE *fp = fopen(log_fpath, "w");
    Assert(fp, "Can not open '%s'", log_fpath);
    log_fp = fp;
  }
  Log("Log is written to %s", log_fpath ? log_fpath : "stdout");

  #ifdef CONFIG_ITRACE_IRINGBUF
  if (log_fpath != NULL)
  {
    char *itrace_fname_suffix = "-itrace";
    log_ifp = get_log_file(log_fpath,itrace_fname_suffix);
  }
  #endif

  #ifdef CONFIG_MTRACE
  if (log_fpath != NULL)
  { 
    char *mtrace_fname_suffix = "-mtrace";
    log_mfp = get_log_file(log_fpath,mtrace_fname_suffix);
  }
  #endif

  #ifdef CONFIG_FTRACE
  if (log_fpath!=NULL)
  {
    char *ftrace_fname_suffix = "-ftrace";
    log_ffp = get_log_file(log_fpath,ftrace_fname_suffix);
  }
  #endif
}

bool log_enable()
{
  return MUXDEF(CONFIG_TRACE, (g_nr_guest_inst >= CONFIG_TRACE_START) && (g_nr_guest_inst <= CONFIG_TRACE_END), false);
}
#endif

/*
add fname_suffix to the log_fpath, e.g. log_file specified in command line, generating a new file name log_filefname_suffix
openning this file in "w" mode and return the FILE* fp;
*/
FILE * get_log_file(const char* log_fpath, char * fname_suffix){
    int insert_pos = find_last_substr_idx(log_fpath, ".txt");
    insert_pos = insert_pos ? insert_pos : strlen(log_fpath);
    char *trace_fname = insert_string(log_fpath, fname_suffix, insert_pos);
    FILE *fp = fopen(trace_fname, "w");
    free(trace_fname);
    Assert(fp, "Can not open '%s'", trace_fname);
    return fp;
}

/*
REMENBER TO FREE the pointer returned by insert_string(), otherwise memory leakage is caused.
*/
char *insert_string(const char *original, const char *insert, size_t position)
{
  // 获取原字符串和插入字符串的长度
  size_t len_original = strlen(original);
  size_t len_insert = strlen(insert);

  // 检查插入位置是否有效
  if (position > len_original)
  {
    position = len_original; // 插入位置超出原字符串长度时，插入到字符串末尾
  }

  // 计算新字符串的总长度并分配内存
  size_t new_length = len_original + len_insert + 1; // +1 用于终止符
  char *result = (char *)malloc(new_length * sizeof(char));
  if (result == NULL)
  {
    printf("Memory allocation failed\n");
    exit(1);
  }

  // 拷贝原字符串的前部分到新字符串
  strncpy(result, original, position);
  result[position] = '\0'; // 添加终止符

  // 拼接插入字符串
  strcat(result, insert);

  // 拼接原字符串的后部分
  strcat(result, original + position);

  return result;
}

char *find_last_substr_ptr(const char *str, const char *substr)
{
  char *last_position = NULL;
  char *current_position = strstr(str, substr);

  while (current_position != NULL)
  {
    last_position = current_position;
    current_position = strstr(current_position + 1, substr);
  }

  return last_position;
}

int find_last_substr_idx(const char *str, const char *substr){
  char * last_position = find_last_substr_ptr(str,substr);
  int last_idx = (last_position-str)/sizeof(char);
  return last_idx;
}