#include <WiFi101.h>

#include "weather.h"
#include "config.h"
#include "jsmn.h"
#include "http.h"
#include "urlencode.h"
#include "util.h"
#include "lights.h"

// Arbitrary categories of types of VTEC "phenomena" (pp) field
typedef enum {
  CAT_UNKNOWN, // our own value
  CAT_AIR_QUALITY,
  CAT_COLD,
  CAT_HEAT,
  CAT_FLOOD,
  CAT_LOW_WATER,
  CAT_MARINE,
  CAT_SNOW,
  CAT_WIND,
  CAT_DUST,
  CAT_FOG,
  CAT_FREEZE,
  CAT_FIRE,
  CAT_STORM,
  CAT_ICE,
  CAT_TORNADO,
} phen_cat;

// P-VTEC "significance" (s) field.  Ordered least- to most-significant.
typedef enum {
  SIG_UNKNOWN, // our own value
  SIG_SYNOPSIS,
  SIG_OUTLOOK,
  SIG_FORECAST,
  SIG_STATEMENT,
  SIG_ADVISTORY,
  SIG_WATCH,
  SIG_WARNING,
} phen_sig;

// Mappings of P-VTEC "pp" (phenomena) fields to our own categories.
int pp_cats[][3] = {
  {'A', 'F', CAT_AIR_QUALITY},
  {'A', 'S', CAT_AIR_QUALITY},
  {'S', 'M', CAT_AIR_QUALITY},
  {'E', 'C', CAT_COLD},
  {'W', 'C', CAT_COLD},
  {'E', 'H', CAT_HEAT},
  {'H', 'T', CAT_HEAT},
  {'C', 'F', CAT_FLOOD},
  {'F', 'A', CAT_FLOOD},
  {'F', 'F', CAT_FLOOD},
  {'F', 'L', CAT_FLOOD},
  {'H', 'Y', CAT_FLOOD},
  {'L', 'S', CAT_FLOOD},
  {'L', 'O', CAT_LOW_WATER},
  {'M', 'A', CAT_MARINE},
  {'R', 'B', CAT_MARINE},
  {'S', 'C', CAT_MARINE},
  {'S', 'E', CAT_MARINE},
  {'S', 'I', CAT_MARINE},
  {'S', 'U', CAT_MARINE},
  {'S', 'W', CAT_MARINE},
  {'T', 'S', CAT_MARINE},
  {'B', 'S', CAT_SNOW},
  {'B', 'Z', CAT_SNOW},
  {'H', 'S', CAT_SNOW},
  {'L', 'B', CAT_SNOW},
  {'L', 'E', CAT_SNOW},
  {'S', 'B', CAT_SNOW},
  {'S', 'N', CAT_SNOW},
  {'B', 'W', CAT_WIND},
  {'E', 'W', CAT_WIND},
  {'G', 'L', CAT_WIND},
  {'H', 'F', CAT_WIND},
  {'H', 'I', CAT_WIND},
  {'H', 'W', CAT_WIND},
  {'L', 'W', CAT_WIND},
  {'W', 'I', CAT_WIND},
  {'D', 'S', CAT_DUST},
  {'D', 'U', CAT_DUST},
  {'F', 'G', CAT_FOG},
  {'F', 'R', CAT_FREEZE},
  {'F', 'Z', CAT_FREEZE},
  {'H', 'Z', CAT_FREEZE},
  {'F', 'W', CAT_FIRE},
  {'H', 'U', CAT_STORM},
  {'S', 'R', CAT_STORM},
  {'S', 'V', CAT_STORM},
  {'T', 'I', CAT_STORM},
  {'T', 'R', CAT_STORM},
  {'T', 'Y', CAT_STORM},
  {'W', 'S', CAT_STORM},
  {'I', 'P', CAT_ICE},
  {'I', 'S', CAT_ICE},
  {'U', 'P', CAT_ICE},
  {'W', 'W', CAT_ICE},
  {'Z', 'F', CAT_ICE},
  {'Z', 'R', CAT_ICE},
  {'T', 'O', CAT_TORNADO},
};

// Most recently read alert phenomena and significance
phen_cat most_significant_cat;
phen_sig most_significant_sig;

// URL-encoded location from config.h
char encoded_location[64];

phen_cat lookup_phen_cat(char p0, char p1) {
  for (int i = 0; i < (sizeof(pp_cats) / sizeof(pp_cats[0])); i++) {
    if (pp_cats[i][0] == p0 && pp_cats[i][1] == p1) {
      return (phen_cat) pp_cats[i][2];
    }
  }
  return CAT_UNKNOWN;
}

phen_sig lookup_phen_sig(char s0) {
  switch (s0) {
    case 'W':
      return SIG_WARNING;
    case 'A':
      return SIG_WATCH;
    case 'Y':
      return SIG_ADVISTORY;
    case 'S':
      return SIG_STATEMENT;
    case 'F':
      return SIG_FORECAST;
    case 'O':
      return SIG_OUTLOOK;
    case 'N':
      return SIG_SYNOPSIS;
    default:
      return SIG_UNKNOWN;
  }
}

bool parse_vtec(const char *p_vtec, phen_cat *cat, phen_sig *sig) {
  // VTEC is explained at https://www.weather.gov/vtec/.  It's a simple text encoding
  // for weather phenomena.  P-VTEC format follows this format:
  //
  //   /k.aaa.cccc.pp.s.####.yymmddThhnnZ-yymmddThhnnZ/
  //
  // An example (with indexes):
  //
  //   /O.EXT.KCAE.LW.Y.0003.000000T0000Z-220117T1500Z/
  //   012345678911111111112222222222333333333344444444
  //             01234567890123456789012345678901234567
  if (p_vtec[0] != '/' || p_vtec[47] != '/') {
    return false;
  }
  if (p_vtec[2] != '.' || p_vtec[6] != '.' || p_vtec[11] != '.' ||
      p_vtec[14] != '.' || p_vtec[16] != '.' || p_vtec[21] != '.') {
    return false;
  }
  if (p_vtec[28] != 'T' || p_vtec[41] != 'T') {
    return false;
  }
  if (p_vtec[33] != 'Z' || p_vtec[46] != 'Z') {
    return false;
  }
  if (p_vtec[34] != '-') {
    return false;
  }

  *cat = lookup_phen_cat(p_vtec[12], p_vtec[13]);
  *sig = lookup_phen_sig(p_vtec[15]);
  return true;
}

typedef struct {
  // Response body buffer
  char buf[2048];
  // Write position in buffer
  size_t pos;
} get_full_body_ctx;

void get_full_body_cb(http_request *req) {
  get_full_body_ctx * ctx = (get_full_body_ctx*) req->caller_ctx;

  // Read the whole body
  while (req->client->connected() && ctx->pos < sizeof(ctx->buf) - 1) {
    int c = req->client->read();
    if (c != -1) {
      ctx->buf[ctx->pos++] = (char) c;
    }
  }
}

typedef struct {
  // Most recently read chars.  We need enough to hold a full
  // P-VTEC string, which is exactly 48 chars.
  char buf[48];
  // Write position in buffer
  size_t pos;

  // Phenomena category of VTEC with highest significance
  phen_cat  cat;
  // Highest significance VTEC
  phen_sig sig;
} parse_vtecs_ctx;

void rotate_left(char array[], size_t size) {
  if (size < 2) {
    return;
  }
  char head = array[0];
  for (int i = 0; i < size; i++) {
    array[i] = array[i + 1];
  }
  array[size - 1] = head;
}

void parse_vtecs_cb(http_request *req) {
  parse_vtecs_ctx * ctx = (parse_vtecs_ctx*) req->caller_ctx;
  memset(ctx->buf, 0, sizeof(ctx->buf));
  phen_cat cat;
  phen_sig sig;

  // Read one characer at a time into the buffer, checking each time if we
  // have a P-VTEC string there to parse.  The buffer is big enough for
  // exactly one P-VTEC string, so we shift down by one to make room.
  // This is not very CPU efficient, but it's very memory efficient.
  while (req->client->connected()) {
    char c = req->client->read();
    if (c == -1) {
      break;
    }

    // If we're at the end of the buffer, shift everything to the left
    // one character.
    if (ctx->pos == sizeof(ctx->buf)) {
      rotate_left(ctx->buf, sizeof(ctx->buf));
      ctx->pos--;
    }

    // Read one character
    ctx->buf[ctx->pos++] = (char) c;

    // Try to parse it as a P-VTEC.  If it is, and it's more significant than
    // previously parsed ones, keep it.
    if (parse_vtec(ctx->buf, &cat, &sig)) {
      if (sig >= ctx->sig) {
        ctx->cat = cat;
        ctx->sig = sig;
      }
    }
  }
}

bool parse_geocode_response(const char *json, char *lat, size_t lat_size, char *lon, size_t lon_size) {
  const int tokens_size = 500;
  jsmntok_t tokens[tokens_size];
  jsmn_parser parser;

  jsmn_init(&parser);
  int num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, tokens_size);
  if (num_tokens < 0) {
    Serial.println("Failed to parse the geocode JSON");
    return false;
  }

  if (num_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
    Serial.println("Top level geocode item was not an object.");
    return false;
  }

  int locations_i = find_json_prop(json, tokens, num_tokens, 0, "locations");
  if (locations_i == -1) {
    Serial.println("JSON missing locations");
    return false;
  }

  // The next token is the first object in the array.  We only need to read
  // one object, so we don't need a function that indexes into the array
  // in a general way.
  int first_location_i = locations_i + 1;
  if (tokens[first_location_i].type != JSMN_OBJECT) {
    Serial.println("JSON missing locations[0]");
    return false;
  }

  int feature_i = find_json_prop(json, tokens, num_tokens, first_location_i, "feature");
  if (feature_i == -1) {
    Serial.println("JSON missing locations[0].feature");
    return false;
  }

  int geometry_i = find_json_prop(json, tokens, num_tokens, feature_i, "geometry");
  if (geometry_i == -1) {
    Serial.println("JSON missing locations[0].feature.geometry");
    return false;
  }

  int x_i = find_json_prop(json, tokens, num_tokens, geometry_i, "x");
  if (x_i == -1) {
    Serial.println("JSON missing locations[0].feature.geometry.x");
    return false;
  }

  int y_i = find_json_prop(json, tokens, num_tokens, geometry_i, "y");
  if (y_i == -1) {
    Serial.println("JSON missing locations[0].feature.geometry.y");
    return false;
  }

  memset(lon, 0, lon_size);
  strncpy(lon, json + tokens[x_i].start, lon_size - 1);

  memset(lat, 0, lat_size);
  strncpy(lat, json + tokens[y_i].start, lat_size - 1);

  return true;
}

bool resolve_location_to_lat_lon(const char *location, char *lat, size_t lat_size, char *lon, size_t lon_size) {
  Serial.print("Resolving location: ");
  Serial.println(location);

  char path[128];
  memset(path, 0, sizeof(path));
  int path_i = strlen(path);

  strncpy(path + path_i, "/arcgis/rest/services/World/GeocodeServer/find?f=json&text=", sizeof(path) - path_i - 1);
  path_i = strlen(path);
  strncpy(path + path_i, location, sizeof(path) - path_i - 1);
  path_i = strlen(path);

  // The whole response can fit in memory
  get_full_body_ctx ctx;
  memset(&ctx, 0, sizeof(ctx));

  http_request req;
  http_request_init(&req);
  req.host = "geocode.arcgis.com";
  req.port = 443;
  req.ssl = true;
  req.path_and_query = path;
  req.header_cb = NULL;
  req.body_cb = get_full_body_cb;
  req.caller_ctx = &ctx;

  http_get(&req);

  if (req.status != 200) {
    Serial.print("HTTP error getting geocode: ");
    Serial.println(req.status, DEC);
    return false;
  }

  if (ctx.pos == 0) {
    Serial.println("Got empty geocode response");
    return false;
  }

  // Ensure the response is NUL-terminated so we can use it as a normal string.
  ctx.buf[min(ctx.pos, sizeof(ctx.buf) - 1)] = '\0';

  Serial.print("Geocode response: ");
  Serial.println(ctx.buf);

  if (!parse_geocode_response(ctx.buf, lat, lat_size, lon, lon_size)) {
    Serial.println("Error parsing response");
    return false;
  }

  Serial.print("Resolved location to: ");
  Serial.print(lat);
  Serial.print(",");
  Serial.println(lon);

  return true;
}

bool get_active_alert(const char *lat, const char *lon, phen_cat *cat, phen_sig *sig) {
  Serial.println("Getting alerts");

  char path[128];
  memset(path, 0, sizeof(path));
  int path_i = strlen(path);

  strncpy(path + path_i, "/alerts/active?status=actual&point=", sizeof(path) - path_i - 1);
  path_i = strlen(path);
  strncpy(path + path_i, lat, sizeof(path) - path_i - 1);
  path_i = strlen(path);
  strncpy(path + path_i, "%2C", sizeof(path) - path_i - 1);
  path_i = strlen(path);
  strncpy(path + path_i, lon, sizeof(path) - path_i - 1);
  path_i = strlen(path);

  // Alert responses may be so large they can't fit in memory.  Use a
  // streaming body callback that just extracts VTEC strings.
  parse_vtecs_ctx ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.cat = CAT_UNKNOWN;
  ctx.sig = SIG_UNKNOWN;

  http_request req;
  http_request_init(&req);
  req.host = "api.weather.gov";
  req.port = 443;
  req.ssl = true;
  req.path_and_query = path;
  req.header_cb = NULL;
  req.body_cb = parse_vtecs_cb;
  req.caller_ctx = &ctx;

  http_get(&req);

  if (req.status != 200) {
    Serial.print("HTTP error getting alerts: ");
    Serial.println(req.status, DEC);
    return false;
  }

  *cat = ctx.cat;
  *sig = ctx.sig;
  return true;
}

void weather_setup(void) {
  WiFi.setPins(8, 7, 4, 2);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);

  memset(encoded_location, 0, sizeof(encoded_location));
  String loc = urlencode(NWS_LOCATION);
  strncpy(encoded_location, loc.c_str(), sizeof(encoded_location) - 1);
}

const char *get_status_description(int status) {
  switch (status) {
    case WL_CONNECTED:
      return "connected";
    case WL_NO_SHIELD:
      return "no shield";
    case WL_IDLE_STATUS:
      return "idle";
    case WL_NO_SSID_AVAIL:
      return "no ssid";
    case WL_SCAN_COMPLETED:
      return "scan completed";
    case WL_CONNECT_FAILED:
      return "connect failed";
    case WL_CONNECTION_LOST:
      return "connection lost";
    case WL_DISCONNECTED:
      return "disconnected";
    default:
      return "unknown";
  }
}

void weather_loop(void) {
  static unsigned long next_time = 0;
  static char lat[10];
  static char lon[10];
  static bool lat_lon_resolved = false;

  unsigned long now = millis();
  if (now >= next_time) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Not connected");
      next_time = now + (1000 * 2);
      return;
    }
    Serial.println("Connected");

    // Resolve the location to lat, lon
    if (!lat_lon_resolved) {
      lat_lon_resolved = resolve_location_to_lat_lon(encoded_location, lat, sizeof(lat), lon, sizeof(lon));
      if (!lat_lon_resolved) {
        next_time = now + (1000 * 10);
        return;
      }
    }

    // Get the most significant phenomenon for current alerts.
    phen_cat cat;
    phen_sig sig;
    if (!get_active_alert(lat, lon, &cat, &sig)) {
      next_time = now + (1000 * 10);
      return;
    }

    Serial.print("Active phenomenon category: ");
    Serial.println(cat);
    Serial.print("Significance: ");
    Serial.println(sig);

    // Update the lights.  Warnings get high speed, all else low.
    bool fast = sig == SIG_WARNING;
    switch (cat) {
      case CAT_AIR_QUALITY:
        lights_configure(ANIM_PULSE, fast, COLOR_LIGHT_GRAY, COLOR_YELLOW);
        break;
      case CAT_COLD:
        lights_configure(ANIM_PULSE, fast, COLOR_LIGHT_GRAY, COLOR_DARK_BLUE);
        break;
      case CAT_HEAT:
        lights_configure(ANIM_PULSE, fast, COLOR_WHITE, COLOR_ORANGE);
        break;
      case CAT_FLOOD:
        lights_configure(ANIM_FLOOD, fast, COLOR_BLACK, COLOR_DARK_BLUE);
        break;
      case CAT_LOW_WATER:
        lights_configure(ANIM_FLOOD, fast, COLOR_BLACK, COLOR_YELLOW);
        break;
      case CAT_MARINE:
        lights_configure(ANIM_FLOOD, fast, COLOR_DARK_BLUE, COLOR_LIGHT_BLUE);
        break;
      case CAT_SNOW:
        lights_configure(ANIM_PRECIP, fast, COLOR_BLACK, COLOR_WHITE);
        break;
      case CAT_WIND:
        lights_configure(ANIM_SWIRL, fast, COLOR_DARK_GRAY, COLOR_LIGHT_GRAY);
        break;
      case CAT_DUST:
        lights_configure(ANIM_PULSE, fast, COLOR_LIGHT_GRAY, COLOR_YELLOW);
        break;
      case CAT_FOG:
        lights_configure(ANIM_PULSE, fast, COLOR_DARK_GRAY, COLOR_LIGHT_GRAY);
        break;
      case CAT_FREEZE:
        lights_configure(ANIM_PULSE, fast, COLOR_LIGHT_GRAY, COLOR_LIGHT_BLUE);
        break;
      case CAT_FIRE:
        lights_configure(ANIM_PULSE, fast, COLOR_BLACK, COLOR_ORANGE);
        break;
      case CAT_STORM:
        lights_configure(ANIM_PRECIP, fast, COLOR_DARK_BLUE, COLOR_LIGHT_BLUE);
        break;
      case CAT_ICE:
        lights_configure(ANIM_PRECIP, fast, COLOR_BLACK, COLOR_LIGHT_BLUE);
        break;
      case CAT_TORNADO:
        lights_configure(ANIM_SWIRL, fast, COLOR_WHITE, COLOR_RED);
        break;
      default:
        // This animation doesn't care about speed or colors
        lights_configure(ANIM_DEFAULT, fast, COLOR_BLACK, COLOR_BLACK);
        break;
    }

    next_time = now + (1000 * 60 * FORECAST_PERIOD_MINUTES);
  }
}
