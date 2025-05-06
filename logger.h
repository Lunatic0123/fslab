/*
请不要修改此文件
*/

#define LOG_DEBUG 0
#define LOG_INFO 10
#define LOG_IMPORTANT 15
#define LOG_WARNING 20
#define LOG_ERROR 30
#define LOG_OFF 100

#ifndef LOG_LEVEL
// 在这里设置日志输出级别，大于等于这个级别的日志会被输出
// 如果你想测试极致的性能，你可能需要把日志关掉，这是我们提供这个日志模块的原因
#define LOG_LEVEL LOG_INFO
#endif

// 用法和 printf 一致，比如 `fs_debug("1+1=%d\n", 1 + 1);`
void fs_debug(const char* format, ...);

// 用法和 printf 一致，比如 `fs_info("1+1=%d\n", 1 + 1);`
void fs_info(const char *format, ...);

// 用法和 printf 一致，比如 `fs_important("1+1=%d\n", 1 + 1);`
void fs_important(const char *format, ...);

// 用法和 printf 一致，比如 `fs_warning("1+1=%d\n", 1 + 1);`
void fs_warning(const char *format, ...);

// 用法和 printf 一致，比如 `fs_error("1+1=%d\n", 1 + 1);`
void fs_error(const char *format, ...);
