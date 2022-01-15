#include <WiFi101.h>

#include "weather.h"
#include "config.h"
#include "jsmn.h"
#include "http.h"
#include "urlencode.h"
#include "util.h"
#include "lights.h"

// Most recently read alert phenomena and significance
phen_cat_t phen_cat;
phen_sig_t phen_sig;

// URL-encoded location from config.h
char encoded_location[64];

// API responses can be large.  Pre-allocate one big buffer.
char response[10 * 1024];
int response_i;

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

phen_cat_t lookup_phen_cat(char p0, char p1) {
  for (int i = 0; i < (sizeof(pp_cats) / sizeof(pp_cats[0])); i++) {
    if (pp_cats[i][0] == p0 && pp_cats[i][1] == p1) {
      return (phen_cat_t) pp_cats[i][2];
    }
  }
  return CAT_UNKNOWN;
}

phen_sig_t lookup_phen_sig(char s0) {
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

bool parse_vtec(const char *p_vtec, phen_cat_t *phen_cat, phen_sig_t *phen_sig) {
  // VTEC is explained at https://www.weather.gov/vtec/.  It's a simple text encoding
  // for weather phenomena.  P-VTEC format follows this format:
  //
  //   /k.aaa.cccc.pp.s.####.yymmddThhnnZ-yymmddThhnnZ/
  if (strlen(p_vtec) < 48) {
    Serial.println("P-VTEC too short");
    return false;
  }

  *phen_cat = lookup_phen_cat(p_vtec[12], p_vtec[13]);
  *phen_sig = lookup_phen_sig(p_vtec[15]);
  return true;
}

void get_full_body_cb(struct http_request *req) {
  // Read the whole body
  while (req->client->connected() && response_i < sizeof(response) - 1) {
    int c = req->client->read();
    if (c != -1) {
      response[response_i++] = (char) c;
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

  Serial.println(path);
  memset(response, 0, sizeof(response));
  response_i = 0;

  struct http_request req;
  http_request_init(&req);
  req.host = "geocode.arcgis.com";
  req.port = 443;
  req.ssl = true;
  req.path_and_query = path;
  req.header_cb = NULL;
  req.body_cb = get_full_body_cb;

  http_get(&req);

  if (req.status != 200) {
    Serial.print("HTTP error getting geocode: ");
    Serial.println(req.status, DEC);
    return false;
  }

  if (response_i == 0) {
    Serial.println("Got empty geocode response");
    return false;
  }

  Serial.print("Geocode response: ");
  Serial.println(response);

  if (!parse_geocode_response(response, lat, lat_size, lon, lon_size)) {
    Serial.println("Error parsing response");
    return false;
  }

  Serial.print("Resolved location to: ");
  Serial.print(lat);
  Serial.print(",");
  Serial.println(lon);

  return true;
}

bool parse_active_alerts_response(const char *json, phen_cat_t *phen_cat, phen_sig_t * phen_sig) {
  const int tokens_size = 500;
  jsmntok_t tokens[tokens_size];
  jsmn_parser parser;

  jsmn_init(&parser);
  int num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, tokens_size);
  if (num_tokens < 0) {
    Serial.println("Failed to parse the grid JSON");
    return false;
  }

  if (num_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
    Serial.println("Top level grid item was not an object.");
    return false;
  }

  int features_i = find_json_prop(json, tokens, num_tokens, 0, "features");
  if (features_i == -1) {
    Serial.println("JSON missing features");
    return false;
  }

  // Walk through the remaining tokens looking for feature objects in the features array.
  // Each feature is a watch, warning, advisory, or similar.  We'll select the phenomena
  // for the one with the highest significance (if there's a tie, we'll have the last one).
  for (int feature_i = features_i + 1; feature_i < num_tokens; feature_i++) {
    if (tokens[feature_i].type == JSMN_OBJECT && tokens[feature_i].parent == features_i) {
      int properties_i = find_json_prop(json, tokens, num_tokens, feature_i, "properties");
      if (properties_i == -1) {
        Serial.println("JSON missing features[].properties");
        continue;
      }

      int parameters_i = find_json_prop(json, tokens, num_tokens, properties_i, "parameters");
      if (parameters_i == -1) {
        Serial.println("JSON missing features[].properties.parameters");
        continue;
      }

      int vtecs_i = find_json_prop(json, tokens, num_tokens, parameters_i, "VTEC");
      if (vtecs_i == -1) {
        Serial.println("JSON missing features[].properties.parameters.VTEC");
        continue;
      }

      // The VTEC value is an array of strings, but there's just going to be one (probably)
      int vtec_i = vtecs_i + 1;

      char vtec_str[49];
      memset(vtec_str, 0, sizeof(vtec_str));
      size_t vtec_len = tokens[vtec_i].end - tokens[vtec_i].start;
      strncpy(vtec_str, json + tokens[vtec_i].start, min(vtec_len, sizeof(vtec_str) - 1));

      // Parse out the phenomena and significance
      phen_cat_t this_cat = CAT_UNKNOWN;
      phen_sig_t this_sig = SIG_UNKNOWN;
      if (parse_vtec(json + tokens[vtec_i].start, phen_cat, phen_sig)) {
        // Write out the values if they're at least as significant as previous ones
        if (this_sig >= *phen_sig) {
          *phen_cat = this_cat;
          *phen_sig = this_sig;
        }
      }
    }
  }

  return true;
}

bool get_active_alert(const char *lat, const char *lon, phen_cat_t *phen_cat, phen_sig_t *phen_sig) {
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

  memset(response, 0, sizeof(response));
  response_i = 0;

  struct http_request req;
  http_request_init(&req);
  req.host = "api.weather.gov";
  req.port = 443;
  req.ssl = true;
  req.path_and_query = path;
  req.header_cb = NULL;
  req.body_cb = get_full_body_cb;

  http_get(&req);

  if (req.status != 200) {
    Serial.print("HTTP error getting alerts: ");
    Serial.println(req.status, DEC);
    return false;
  }

  if (response_i == 0) {
    Serial.println("Got empty alerts response");
    return false;
  }

  Serial.print("Alerts response: ");
  Serial.println(response);

  if (!parse_active_alerts_response(response, phen_cat, phen_sig)) {
    Serial.println("Error parsing response");
    return false;
  }

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

    // Get the phenomenon category for the current active alert.
    // If there is more than 1 active alert, we'll pick one arbitrarily.
    if (!get_active_alert(lat, lon, &phen_cat, &phen_sig)) {
      next_time = now + (1000 * 10);
      return;
    }

    Serial.print("Active phenomenon category: ");
    Serial.println(phen_cat);
    Serial.print("Significance: ");
    Serial.println(phen_sig);

    // Update the lights.  Warnings get high speed, all else low.
    bool fast = phen_sig == SIG_WARNING;
    switch (phen_cat) {
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

    //next_time = now + (1000 * 60 * FORECAST_PERIOD_MINUTES);
    next_time = now + (1000);
  }
}

void weather_get_active_phenomena(phen_cat_t *cat, phen_sig_t *sig) {
  *cat = phen_cat;
  *sig = phen_sig;
}
