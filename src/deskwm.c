#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "config.h"
#include "cursor.h"
#include "desk.h"
#include "deskwm.h"
#include "keyboard.h"
#include "log.h"
#include "output.h"
#include "shell.h"
#include "types.h"


static const char usage[] =
    "usage: deskwm [options...]\n"
    "    -h         show this help message\n"
    "    -c <file>  specify config file\n"
    "    -d         set logging to debug mode\n"
    "    -i         set logging to info mode\n"
;


int main(int argc, char *argv[])
{
    int opt;
    int log_level = WLR_ERROR;
    struct server server;
    server.config_file = NULL;

    while ((opt = getopt(argc, argv, "hc:di")) != -1) {
        switch (opt) {
	    case 'h':
		printf(usage);
		return EXIT_SUCCESS;
		break;
	    case 'c':
		server.config_file = strdup(optarg);
		break;
	    case 'd':
		log_level = WLR_DEBUG;
		break;
	    case 'i':
		log_level = WLR_INFO;
		break;
        }
    }

    init_log(log_level);

    // create
    server.display = wl_display_create();
    server.backend = wlr_backend_autocreate(server.display, NULL);
    server.renderer = wlr_backend_get_renderer(server.backend);
    wlr_renderer_init_wl_display(server.renderer, server.display);
    wlr_compositor_create(server.display, server.renderer);

    // add some managers
    wlr_screencopy_manager_v1_create(server.display);
    wlr_data_device_manager_create(server.display);
    wlr_primary_selection_v1_device_manager_create(server.display);

    // initialise
    wl_list_init(&server.desks);
    wl_list_init(&server.key_bindings);
    wl_list_init(&server.mouse_bindings);
    wl_list_init(&server.marks);
    server.mark_waiting = false;
    server.on_mouse_motion = NULL;
    server.on_mouse_scroll = NULL;
    server.can_steal_focus = true;
    server.mark_indicator.box.width = 25;
    server.mark_indicator.box.height = 25;
    server.mark_indicator.box.x = 0;
    server.mark_indicator.box.y = 0;
    server.desk_count = 0;
    add_desk(&server);
    server.current_desk = wl_container_of(server.desks.next, server.current_desk, link);

    // configure
    locate_config(&server);
    load_config(&server);
    set_up_outputs(&server);
    set_up_shell(&server);
    set_up_cursor(&server);
    set_up_keyboard(&server);

    // start
    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
	wlr_backend_destroy(server.backend);
	wl_display_destroy(server.display);
	return EXIT_FAILURE;
    }
    if (!wlr_backend_start(server.backend)) {
	wlr_backend_destroy(server.backend);
	wl_display_destroy(server.display);
	return 1;
    }
    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Starting with WAYLAND_DISPLAY=%s", socket);
    wl_display_run(server.display);

    // stop
    wl_display_destroy_clients(server.display);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.display);
    return EXIT_SUCCESS;
}
