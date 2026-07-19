#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Resolve the HDZero bandwidth to actually use: the auto-detected value when
// the source setting is Auto, otherwise the configured Wide/Narrow value.
// Use this anywhere HDZero_open()/OSD needs the real bandwidth.
int hdzero_effective_bw(void);

// Called each iteration of the source-signal poll loop. When the HDZero
// bandwidth setting is Auto and the receiver is not locked, it cycles the
// bandwidth to find a lock; while locked it does nothing, so it never
// disrupts a good signal.
void scan_core_hdz_bw_tick(void);

#ifdef __cplusplus
}
#endif
