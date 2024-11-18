# pragma once


#ifndef LOG_TAG
# define LOG_TAG ""
#endif


# ifndef  __clang__

#define LOGE(format, ...) ({ \
  pr_err(LOG_TAG, format  , ##__VA_ARGS__); \
})
#define LOGD(format, ...) ({ \
  pr_debug(LOG_TAG, format , ##__VA_ARGS__); \
})

#define LOGI(format, ...) ({ \
  pr_info(LOG_TAG, format , ##__VA_ARGS__); \
})
#define LOGW(format, ...) ({ \
  pr_warn(LOG_TAG, format , ##__VA_ARGS__); \
})

# else
#define LOGE(format, ...) ({ \
  pr_err(LOG_TAG, format  , ##__VA_ARGS__); \
})
#define LOGD(format, ...) ({ \
  pr_debug(LOG_TAG, format , ##__VA_ARGS__); \
})

#define LOGI(format, ...) ({ \
  pr_info(LOG_TAG, format , ##__VA_ARGS__); \
})
#define LOGW(format, ...) ({ \
  pr_warn(LOG_TAG, format , ##__VA_ARGS__); \
})
# endif

extern int prepare_log ();
extern int pr_debug(const char *tag, const char *format, ...);
extern int pr_err(const char *tag, const char *format, ...);
extern int pr_info(const char *tag, const char *format, ...);
extern int pr_warn(const char *tag, const char *format, ...);
