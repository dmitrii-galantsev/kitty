#define PHASE_BOTH 1
#define PHASE_BACKGROUND 2
#define PHASE_SPECIAL 3
#define PHASE_FOREGROUND 4

#define PHASE {WHICH_PHASE}
#define HAS_TRANSPARENCY {TRANSPARENT}
#define FG_OVERRIDE {FG_OVERRIDE}
#define FG_OVERRIDE_THRESHOLD {FG_OVERRIDE_THRESHOLD}
#define APPLY_MIN_CONTRAST_RATIO {APPLY_MIN_CONTRAST_RATIO}
#define MIN_CONTRAST_RATIO {MIN_CONTRAST_RATIO}
#define TEXT_NEW_GAMMA {TEXT_NEW_GAMMA}

#define DECORATION_SHIFT {DECORATION_SHIFT}
#define REVERSE_SHIFT {REVERSE_SHIFT}
#define STRIKE_SHIFT {STRIKE_SHIFT}
#define DIM_SHIFT {DIM_SHIFT}
#define MARK_SHIFT {MARK_SHIFT}
#define MARK_MASK {MARK_MASK}
#define USE_SELECTION_FG
#define NUM_COLORS 256

#if (PHASE == PHASE_BOTH) || (PHASE == PHASE_BACKGROUND) || (PHASE == PHASE_SPECIAL)
#define NEEDS_BACKROUND
#endif

#if (PHASE == PHASE_BOTH) || (PHASE == PHASE_FOREGROUND)
#define NEEDS_FOREGROUND
#endif

#if (HAS_TRANSPARENCY == 1)
#define TRANSPARENT
#endif
