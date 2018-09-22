#ifndef COMMON_H__
#define COMMON_H__


#ifdef __cplusplus
extern "C" {
#endif


#define ASSERT(x) ((void)0)
void DbgPrint(const char* fmt, ...);
void common_init(void);


#ifdef __cplusplus
}
#endif


#endif // COMMON_H__
