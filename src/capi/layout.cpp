#include "gran/capi/gran.h"

// Раскладка публичного ABI на поддерживаемых 64-битных платформах.
// Public ABI layout on the supported 64-bit platforms.
#define GRAN_FIELD(type, field, offset) static_assert(offsetof(type, field) == offset, #type "." #field)
#define GRAN_LAYOUT(type, bytes) static_assert(sizeof(type) == bytes && alignof(type) == 8, #type)

GRAN_FIELD(gran_options, size, 0);
GRAN_FIELD(gran_options, backend, 4);
GRAN_FIELD(gran_options, debug_layer, 8);
GRAN_FIELD(gran_options, force_warp, 12);
GRAN_FIELD(gran_options, watch_markup, 16);
GRAN_FIELD(gran_options, no_vsync, 20);
GRAN_FIELD(gran_options, exit_after, 24);
GRAN_LAYOUT(gran_options, 32);

GRAN_FIELD(gran_update, target, 0);
GRAN_FIELD(gran_update, property, 8);
GRAN_FIELD(gran_update, value, 16);
GRAN_LAYOUT(gran_update, 24);

GRAN_FIELD(gran_field, name, 0);
GRAN_FIELD(gran_field, value, 8);
GRAN_LAYOUT(gran_field, 16);

GRAN_FIELD(gran_event, target, 0);
GRAN_FIELD(gran_event, model, 8);
GRAN_FIELD(gran_event, index, 16);
GRAN_FIELD(gran_event, text, 24);
GRAN_FIELD(gran_event, option, 32);
GRAN_FIELD(gran_event, message, 40);
GRAN_FIELD(gran_event, value, 48);
GRAN_FIELD(gran_event, checked, 56);
GRAN_LAYOUT(gran_event, 64);

GRAN_FIELD(gran_timer, seconds, 0);
GRAN_FIELD(gran_timer, repeat, 8);
GRAN_FIELD(gran_timer, callback, 16);
GRAN_FIELD(gran_timer, user, 24);
GRAN_LAYOUT(gran_timer, 32);

GRAN_FIELD(gran_record, index, 0);
GRAN_FIELD(gran_record, fields, 8);
GRAN_FIELD(gran_record, count, 16);
GRAN_LAYOUT(gran_record, 24);

GRAN_FIELD(gran_input_event, size, 0);
GRAN_FIELD(gran_input_event, phase, 4);
GRAN_FIELD(gran_input_event, type, 8);
GRAN_FIELD(gran_input_event, target, 16);
GRAN_FIELD(gran_input_event, current, 24);
GRAN_FIELD(gran_input_event, related, 32);
GRAN_FIELD(gran_input_event, key, 40);
GRAN_FIELD(gran_input_event, detail, 48);
GRAN_FIELD(gran_input_event, drag_format, 56);
GRAN_FIELD(gran_input_event, drag_data, 64);
GRAN_FIELD(gran_input_event, model, 72);
GRAN_FIELD(gran_input_event, related_model, 80);
GRAN_FIELD(gran_input_event, index, 88);
GRAN_FIELD(gran_input_event, related_index, 96);
GRAN_FIELD(gran_input_event, x, 104);
GRAN_FIELD(gran_input_event, y, 112);
GRAN_FIELD(gran_input_event, window_x, 120);
GRAN_FIELD(gran_input_event, window_y, 128);
GRAN_FIELD(gran_input_event, wheel_x, 136);
GRAN_FIELD(gran_input_event, wheel_y, 144);
GRAN_FIELD(gran_input_event, button, 152);
GRAN_FIELD(gran_input_event, clicks, 156);
GRAN_FIELD(gran_input_event, modifiers, 160);
GRAN_FIELD(gran_input_event, dropped, 164);
GRAN_FIELD(gran_input_event, cell_row, 168);
GRAN_FIELD(gran_input_event, cell_field, 176);
GRAN_FIELD(gran_input_event, cell_old, 184);
GRAN_FIELD(gran_input_event, cell_value, 192);
GRAN_LAYOUT(gran_input_event, 200);

GRAN_FIELD(gran_listener, size, 0);
GRAN_FIELD(gran_listener, capture, 4);
GRAN_FIELD(gran_listener, callback, 8);
GRAN_FIELD(gran_listener, user, 16);
GRAN_FIELD(gran_listener, release, 24);
GRAN_LAYOUT(gran_listener, 32);

GRAN_FIELD(gran_undo_step, size, 0);
GRAN_FIELD(gran_undo_step, undo, 8);
GRAN_FIELD(gran_undo_step, redo, 16);
GRAN_FIELD(gran_undo_step, release, 24);
GRAN_FIELD(gran_undo_step, user, 32);
GRAN_LAYOUT(gran_undo_step, 40);

GRAN_FIELD(gran_window_options, size, 0);
GRAN_FIELD(gran_window_options, owner, 8);
GRAN_FIELD(gran_window_options, modal, 16);
GRAN_FIELD(gran_window_options, on_closed, 24);
GRAN_FIELD(gran_window_options, user, 32);
GRAN_LAYOUT(gran_window_options, 40);

GRAN_FIELD(gran_background, size, 0);
GRAN_FIELD(gran_background, work, 8);
GRAN_FIELD(gran_background, done, 16);
GRAN_FIELD(gran_background, progress, 24);
GRAN_FIELD(gran_background, user, 32);
GRAN_LAYOUT(gran_background, 40);

GRAN_FIELD(gran_column, name, 0);
GRAN_FIELD(gran_column, title, 8);
GRAN_FIELD(gran_column, type, 16);
GRAN_LAYOUT(gran_column, 24);

GRAN_FIELD(gran_source, size, 0);
GRAN_FIELD(gran_source, format, 4);
GRAN_FIELD(gran_source, path, 8);
GRAN_FIELD(gran_source, select, 16);
GRAN_FIELD(gran_source, children, 24);
GRAN_FIELD(gran_source, fetch, 32);
GRAN_FIELD(gran_source, fields, 40);
GRAN_FIELD(gran_source, field_count, 48);
GRAN_LAYOUT(gran_source, 56);

GRAN_FIELD(gran_view, size, 0);
GRAN_FIELD(gran_view, kind, 4);
GRAN_FIELD(gran_view, name, 8);
GRAN_FIELD(gran_view, source, 16);
GRAN_FIELD(gran_view, sort, 24);
GRAN_FIELD(gran_view, filter, 32);
GRAN_FIELD(gran_view, mode, 40);
GRAN_FIELD(gran_view, expanded, 48);
GRAN_LAYOUT(gran_view, 56);

GRAN_FIELD(gran_tree_row, node, 0);
GRAN_FIELD(gran_tree_row, depth, 8);
GRAN_FIELD(gran_tree_row, expanded, 16);
GRAN_FIELD(gran_tree_row, has_children, 20);
GRAN_LAYOUT(gran_tree_row, 24);
