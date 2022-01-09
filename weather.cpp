#include <WiFi101.h>

#include "weather.h"
#include "config.h"
#include "jsmn.h"
#include "http.h"
#include "urlencode.h"
#include "util.h"

class GetFullBodyCtx {
  public:
    String body;
};

void get_full_body_cb(struct http_request *req) {
  GetFullBodyCtx *ctx = (GetFullBodyCtx *) req->caller_ctx;

  // Read the whole body
  while (req->client->connected()) {
    int c = req->client->read();
    if (c != -1) {
      ctx->body += (char) c;
    }
  }
}

bool parse_geocode_response(String geocode_json, String &lat, String &lon) {
  const char * json = geocode_json.c_str();

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

  lon = geocode_json.substring(tokens[x_i].start, tokens[x_i].end);
  lat = geocode_json.substring(tokens[y_i].start, tokens[y_i].end);
  return true;
}

bool resolve_location_to_lat_lon(const char *location, String &lat, String &lon) {
  Serial.print("Resolving location: ");
  Serial.println(location);

  String path_and_query("/arcgis/rest/services/World/GeocodeServer/find?f=json&text=");
  path_and_query += urlencode(NWS_LOCATION);

  GetFullBodyCtx ctx;

  struct http_request req;
  http_request_init(&req);
  req.host = "geocode.arcgis.com";
  req.port = 443;
  req.ssl = true;
  req.path_and_query = path_and_query.c_str();
  req.header_cb = NULL;
  req.body_cb = get_full_body_cb;
  req.caller_ctx = &ctx;

  http_get(&req);

  if (req.status != 200) {
    Serial.print("HTTP error getting geocode: ");
    Serial.println(req.status, DEC);
    return false;
  }

  if (ctx.body.length() == 0) {
    Serial.println("Got empty geocode response");
    return false;
  }

  Serial.print("Geocode response: ");
  Serial.println(ctx.body);

  if (!parse_geocode_response(ctx.body, lat, lon)) {
    Serial.println("Error parsing response");
    return false;
  }

  Serial.print("Resolved location to: ");
  Serial.print(lat);
  Serial.print(",");
  Serial.println(lon);

  return true;
}

bool parse_grid_response(String grid_json, String &grid_x, String &grid_y) {
  const char * json = grid_json.c_str();

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

  int properties_i = find_json_prop(json, tokens, num_tokens, 0, "properties");
  if (properties_i == -1) {
    Serial.println("JSON missing properties");
    return false;
  }

  int grid_x_i = find_json_prop(json, tokens, num_tokens, properties_i, "gridX");
  if (grid_x_i == -1) {
    Serial.println("JSON missing properties.gridX");
    return false;
  }

  int grid_y_i = find_json_prop(json, tokens, num_tokens, properties_i, "gridY");
  if (grid_y_i == -1) {
    Serial.println("JSON missing properties.gridY");
    return false;
  }

  grid_x = grid_json.substring(tokens[grid_x_i].start, tokens[grid_x_i].end);
  grid_y = grid_json.substring(tokens[grid_y_i].start, tokens[grid_y_i].end);
  return true;
}

bool resolve_lat_lon_to_grid(const String &lat, const String &lon, String &grid_x, String &grid_y) {
  // NWS requires no more than 4 digits after the decimal point in the URL
  String short_lat = lat.substring(0, lat.indexOf(".") + 5);
  String short_lon = lon.substring(0, lon.indexOf(".") + 5);
  
  Serial.print("Resolving lat, lon: ");
  Serial.print(short_lat);
  Serial.print(",");
  Serial.println(short_lon);

  String path_and_query("/points/");
  path_and_query += short_lat;
  path_and_query += ",";
  path_and_query += short_lon;

  GetFullBodyCtx ctx;

  struct http_request req;
  http_request_init(&req);
  req.host = "api.weather.gov";
  req.port = 443;
  req.ssl = true;
  req.path_and_query = path_and_query.c_str();
  req.header_cb = NULL;
  req.body_cb = get_full_body_cb;
  req.caller_ctx = &ctx;

  http_get(&req);

  if (req.status != 200) {
    Serial.print("HTTP error getting grid: ");
    Serial.println(req.status, DEC);
    return false;
  }

  if (ctx.body.length() == 0) {
    Serial.println("Got empty grid response");
    return false;
  }

  Serial.print("Grid response: ");
  Serial.println(ctx.body);

  if (!parse_grid_response(ctx.body, grid_x, grid_y)) {
    Serial.println("Error parsing response");
    return false;
  }

  Serial.print("Resolved grid to: ");
  Serial.print(grid_x);
  Serial.print(",");
  Serial.println(grid_y);

  return true;
}

bool get_forecast(const String &grid_x, const String &grid_y) {
    return false;    
}

void weather_setup(void) {
  WiFi.setPins(8, 7, 4, 2);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
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
  static String lat;
  static String lon;
  static String grid_x;
  static String grid_y;
  static bool lat_lon_resolved = false;
  static bool grid_resolved = false;

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
      lat_lon_resolved = resolve_location_to_lat_lon(NWS_LOCATION, lat, lon);
      if (!lat_lon_resolved) {
        next_time = now + (1000 * 10);
        return;
      }
    }

    // Resolve the lat, lon to NWS grid x, grid y
    if (!grid_resolved) {
      grid_resolved = resolve_lat_lon_to_grid(lat, lon, grid_x, grid_y);
      if (!grid_resolved) {
        next_time = now + (1000 * 10);
        return;
      }
    }

    // Get the forecast
    bool got_forecast = false;
    Serial.print("Getting forecast at ");
    Serial.println(WiFi.getTime());
    got_forecast = get_forecast(grid_x, grid_y);

    if (!got_forecast) {
      Serial.println("Error getting forecast");
      next_time = now + (1000 * 10);
      return;
    }
    Serial.println("Got forecast");

    next_time = now + (1000 * 60 * FORECAST_PERIOD_MINUTES);
  }
}
