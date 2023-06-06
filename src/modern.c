#include <pebble.h>
#include <lang.h>

static Window *s_main_window;
static Layer *s_window_layer;
static Layer *s_canvas_layer;
static Layer *s_battery_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weekday_layer;

static GFont s_date_font;
static GFont s_weekday_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static GRect bounds;
static GPoint s_center;
static int s_battery_level;

static const GPathInfo MINUTE_HAND_PATH_POINTS = {
  4,
  (GPoint []) {
    {-4, 15},
    {4, 15},
    {4, -70},
    {-4,  -70}
  }
};


static const GPathInfo HOUR_HAND_PATH_POINTS = {
  4,
  (GPoint []) {
    {-4, 15},
    {4, 15},
    {4, -50},
    {-4,  -50}
  }
};

static GPath *hour_hand_path = NULL;
static GPath *minute_hand_path = NULL;

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar (total width = 23px)
  int width = (s_battery_level * 23) / 100;

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void draw_date(struct tm *tick_time) {
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_MEDIUM_12));
  s_date_layer = text_layer_create(
    GRect(116, 77, 20, 20));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, s_date_font);

  // Write the current date into a buffer
  static char d_buffer[3];
  strftime(d_buffer, sizeof(d_buffer), "%d", tick_time);

  text_layer_set_text(s_date_layer, d_buffer);

  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));

  s_weekday_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITALDREAM_NARROW_14));
  s_weekday_layer = text_layer_create(
    GRect(32, 42, 80, 20));
  text_layer_set_text_color(s_weekday_layer, GColorWhite);
  text_layer_set_text_alignment(s_weekday_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_weekday_layer, GColorClear);
  text_layer_set_font(s_weekday_layer, s_weekday_font);

  // Write the current weekday into a buffer
  static char w_buffer[3];
  strftime(w_buffer, sizeof(w_buffer), "%u", tick_time);

  text_layer_set_text(s_weekday_layer, DAYS[atoi(w_buffer)]);

  layer_add_child(s_window_layer, text_layer_get_layer(s_weekday_layer));
}

static void update_date() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  draw_date(tick_time);
}

static void draw_center(GContext *ctx, GPoint s_center) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_center, 4);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_center, 3);
}

static void draw_hour_hand(GContext *ctx, GPoint s_center, struct tm *tick_time) {
  unsigned int angle = tick_time->tm_hour * 30 + tick_time->tm_min / 2;
  hour_hand_path = gpath_create(&HOUR_HAND_PATH_POINTS);
  gpath_move_to(hour_hand_path, s_center);
  gpath_rotate_to(hour_hand_path, (TRIG_MAX_ANGLE / 360) * angle);
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, hour_hand_path);
  // Stroke the path:
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, hour_hand_path);
}

static void draw_minute_hand(GContext *ctx, GPoint s_center, struct tm *tick_time) {
  unsigned int angle = tick_time->tm_min * 6 + tick_time->tm_sec / 10;
  minute_hand_path = gpath_create(&MINUTE_HAND_PATH_POINTS);
  gpath_move_to(minute_hand_path, s_center);
  gpath_rotate_to(minute_hand_path, (TRIG_MAX_ANGLE / 360) * angle);
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, minute_hand_path);
  // Stroke the path:
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, minute_hand_path);
}

static void update_time(Layer *layer, GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  GRect bounds = layer_get_unobstructed_bounds(layer);
  s_center = grect_center_point(&bounds);

  draw_hour_hand(ctx, s_center, tick_time);
  draw_minute_hand(ctx, s_center, tick_time);
  draw_center(ctx, s_center);
}

static void update_proc(Layer *layer, GContext *ctx) {
  update_time(layer, ctx);
}

static void main_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(s_window_layer);

  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(s_window_layer, bitmap_layer_get_layer(s_background_layer));

  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(s_window_layer, s_canvas_layer);

  // Create battery layer
  // s_battery_layer = layer_create(GRect(14, 54, 115, 2));
  s_battery_layer = layer_create(GRect(60, 26, 24, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(s_window_layer, s_battery_layer);
  
  // Make sure the date is displayed from the start
  update_date();
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_weekday_font);

  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);

  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weekday_layer);

  layer_destroy(s_window_layer);
  layer_destroy(s_canvas_layer);
  layer_destroy(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_date();
  layer_mark_dirty(s_canvas_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  battery_state_service_subscribe(battery_callback);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
