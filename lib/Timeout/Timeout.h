#ifndef TIMEOUT_H
#define TIMEOUT_H

#ifdef __cplusplus
extern "C" {
#endif

void timeout_init(void);
void timeout_set_us(uint32_t timeout_us);
void timeout_start(void);
void timeout_stop(void);
void timeout_set_callback(void(*callback)(void));

#ifdef __cplusplus
}
#endif
#endif // TIMEOUT_H
