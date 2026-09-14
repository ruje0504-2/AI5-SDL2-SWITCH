/* Single-window menus for Horizon: SDL cannot create desktop popup windows. */
static void switch_popup_run(struct menu *m)
{
    SDL_Surface *s = gfx_get_overlay(2);
    int rows[32], count = 0;
    for (unsigned i = 0; i < vector_length(m->entries) && count < 32; i++)
        if (vector_A(m->entries,i).type != MENU_ENTRY_SEPARATOR) rows[count++] = i;
    if (!count) return;
    const int left = 140, top = 36, width = 360, row_h = 28;
    int selected = 0;
    unsigned old_x, old_y; cursor_get_pos(&old_x, &old_y);
    bool was_up = false, was_down = false, was_a = input_down(INPUT_ACTIVATE), was_b = input_down(INPUT_CANCEL);
    struct menu_entry *chosen = NULL;
    gfx_overlay_enable(2);
    for (;;) {
        handle_events();
        bool up = input_down(INPUT_UP), down = input_down(INPUT_DOWN);
        bool a = input_down(INPUT_ACTIVATE), b = input_down(INPUT_CANCEL);
        if (up && !was_up) selected = (selected + count - 1) % count;
        if (down && !was_down) selected = (selected + 1) % count;
        unsigned x,y;cursor_get_pos(&x,&y);
        if ((x != old_x || y != old_y) && x >= left && x < left + width
                && y >= top && y < top + count * row_h) selected = (y - top) / row_h;
        old_x=x;old_y=y;
        if (b && !was_b) break;
        if (a && !was_a) {
            struct menu_entry *e = &vector_A(m->entries,rows[selected]);
            if (e->active) {chosen=e;break;}
        }
        was_up=up;was_down=down;was_a=a;was_b=b;
        SDL_FillRect(s,NULL,SDL_MapRGBA(s->format,0,0,0,192));
        for(int i=0;i<count;i++) {
            struct menu_entry *e=&vector_A(m->entries,rows[i]);
            SDL_Rect r={left,top+i*row_h,width,row_h};
            SDL_FillRect(s,&r,SDL_MapRGBA(s->format,i==selected?48:16,i==selected?64:16,i==selected?96:16,255));
            SDL_Color c=e->active?(SDL_Color){255,255,255,255}:(SDL_Color){100,100,100,255};
            ui_draw_text(s,left+12,r.y+5,e->text,c);
        }
        gfx_screen_dirty();gfx_update();SDL_Delay(16);
    }
    while (input_down(INPUT_ACTIVATE) || input_down(INPUT_CANCEL)) {handle_events();SDL_Delay(16);}
    gfx_overlay_disable(2);gfx_screen_dirty();
    if (!chosen) return;
    switch(chosen->type) {
    case MENU_ENTRY_LABEL: if(chosen->label.on_click)chosen->label.on_click(chosen->label.data);break;
    case MENU_ENTRY_RADIO: if(chosen->radio.on_click)chosen->radio.on_click(chosen->radio.index,chosen->radio.data);break;
    case MENU_ENTRY_SUBMENU: switch_popup_run(chosen->submenu);break;
    default:break;
    }
}
