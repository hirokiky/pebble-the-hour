#include <pebble.h>


static Window *s_main_window;
static TextLayer *s_current_time_layer;
static TextLayer *s_next_time_layer;
static PropertyAnimation *s_next_property_animation;

static GFont s_font;

const int WIDTH = 144;
const int HEIGHT = 168;
const int ANIM_DULATION = 300;
const int ANIM_DELAY = 100;


static int calculate_next_hour(int tm_hour) {
  int next_hour;

  if(tm_hour == 23) {
    next_hour = 0;
  } else {
    next_hour = tm_hour + 1;
  }
  return next_hour;
}


static PropertyAnimation *make_animation(TextLayer *text_layer, int y) {
  PropertyAnimation *animation;
  Layer *layer_text_layer = text_layer_get_layer(text_layer);
  GRect from_frame = layer_get_frame(layer_text_layer);
  GRect to_frame = GRect(0, y, WIDTH, HEIGHT);
  animation = property_animation_create_layer_frame(layer_text_layer, &from_frame, &to_frame);
  animation_set_duration((Animation *)animation, ANIM_DULATION);
  animation_set_delay((Animation *)animation, ANIM_DELAY);
  animation_set_curve((Animation*)animation, AnimationCurveEaseInOut);
  return animation;
}


static void update_time_buffer(struct tm *tick_time) {

  // Create long-lived buffer;
  static char current_hour_buffer[] = "00";
  static char next_hour_buffer[] = "00";

  int next_hour = calculate_next_hour(tick_time->tm_hour);
  
  if(clock_is_24h_style() == true) {
    snprintf(current_hour_buffer, sizeof("00"), "%02d", 12); //tick_time->tm_hour);
    snprintf(next_hour_buffer, sizeof("00"), "%02d", next_hour);
  } else {
    snprintf(current_hour_buffer, sizeof("00"), "%02d", tick_time->tm_hour % 12);
    snprintf(next_hour_buffer, sizeof("00"), "%02d", next_hour % 12);
  }

  text_layer_set_text(s_current_time_layer, current_hour_buffer);
  text_layer_set_text(s_next_time_layer, next_hour_buffer);
}


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Animate
  s_next_property_animation = make_animation(s_next_time_layer,
                                             HEIGHT - HEIGHT * tick_time->tm_min / 60);
  // animation_schedule((Animation*) s_next_property_animation);

  // Update time string
  update_time_buffer(tick_time);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}


static TextLayer *make_text_layer(int x, int y, int w, int h, GColor back, GColor text) {
  // Create main Window element and assign to pointer
  TextLayer *text_layer;
  text_layer = text_layer_create(GRect(x, y, w, h));
  text_layer_set_background_color(text_layer, back);
  text_layer_set_text_color(text_layer, text);
  text_layer_set_text(text_layer, "00");

  // Improve the layout to be more like a watchface
  text_layer_set_font(text_layer, s_font);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  return text_layer;
}


static void main_window_load(Window *window) {
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BONK_OFFSET_62));
  s_current_time_layer = make_text_layer(0, 0, WIDTH, HEIGHT,
                                         GColorCyan, GColorRed);
  s_next_time_layer = make_text_layer(0, HEIGHT, WIDTH, HEIGHT,
                                      GColorYellow, GColorMagenta);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_current_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_next_time_layer));
}


static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_font);
  text_layer_destroy(s_current_time_layer);
  text_layer_destroy(s_next_time_layer);
}
  

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  update_time_buffer(tick_time);

  // Register with TickTimeService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}


static void deinit() {
  // Destroy Window
  animation_unschedule_all();
  window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
