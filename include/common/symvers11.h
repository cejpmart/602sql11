// Prevents binaries from depending on higher versions of GCLIBC when compiled with a new set of libraries
__asm__(".symver pthread_cond_broadcast,pthread_cond_broadcast@GLIBC_2.0");
__asm__(".symver pthread_cond_destroy,pthread_cond_destroy@GLIBC_2.0");
__asm__(".symver pthread_cond_init,pthread_cond_init@GLIBC_2.0");
__asm__(".symver pthread_cond_signal,pthread_cond_signal@GLIBC_2.0");
__asm__(".symver pthread_cond_timedwait,pthread_cond_timedwait@GLIBC_2.0");
__asm__(".symver pthread_cond_wait,pthread_cond_wait@GLIBC_2.0");
