#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(ARDUINO)   // sketches already have a main()
int replay_main(void);
#else
int main(void);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
