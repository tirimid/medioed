#ifndef EDITOR_H
#define EDITOR_H

int editor_init(int argc, char const *argv[]);
void editor_main_loop(void);
void editor_quit(void);
void editor_redraw(void);
struct buf *editor_add_buf(struct buf *b);
struct frame *editor_add_frame(struct frame *f);
void editor_arrange_frames(void);
void editor_reset_binds(void);
void editor_set_global_mode(void);

#endif
