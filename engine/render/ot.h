#ifndef ENGINE_OT_H
#define ENGINE_OT_H

/* Object Table depth — 0 = front (drawn last), 7 = back (drawn first) */
#define ENGINE_OTLEN 8

#define ENGINE_OT_OVERLAY 0 /* death flash, full-screen overlays */
#define ENGINE_OT_UI 1      /* HUD, menus */
#define ENGINE_OT_PLAYER 2  /* player / ship */
#define ENGINE_OT_OBJ 4     /* obstacles, collectibles */
#define ENGINE_OT_TILES 6   /* tilemap layers */
#define ENGINE_OT_BG 7      /* sky / background */

#endif /* ENGINE_OT_H */
