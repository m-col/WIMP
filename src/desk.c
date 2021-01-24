#include "config.h"
#include "desk.h"
#include "shell.h"
#include "types.h"


void add_desk(struct server *server) {
    struct desk *desk = calloc(1, sizeof(struct desk));
    wl_list_insert(server->desks.prev, &desk->link);
    desk->server = server;
    wl_list_init(&desk->views);
    assign_colour("#5D479D", desk->background);
    assign_colour("#3e3e73", desk->border_normal);
    assign_colour("#998dd1", desk->border_focus);
    desk->border_width = 4;
    desk->wallpaper = NULL;
    desk->index = server->desk_count;
    desk->zoom = 1;
    desk->fullscreened = NULL;
    server->desk_count++;
}


void set_desk(struct server *server, struct desk *desk) {
    struct view *view;
    server->can_steal_focus = false;
    wl_list_for_each(view, &server->current_desk->views, link) {
	unmap_view(view);
    }
    server->current_desk = desk;
    wl_list_for_each(view, &server->current_desk->views, link) {
	map_view(view);
    }
    server->can_steal_focus = true;
    if (!wl_list_empty(&desk->views)) {
	view = wl_container_of(desk->views.next, view, link);
	focus_view(view, view->surface->surface);
    }
}
