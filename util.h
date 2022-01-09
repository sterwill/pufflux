#ifndef __UTIL_H_
#define __UTIL_H_

// Find the token index of the value of the specified property of the specified object 
inline int find_json_prop(const char *json, jsmntok_t *tokens, int num_tokens, int object_tok, const char *prop_name) {
  // Easy way: scan through all tokens looking for parentage
  for (int i = 0; i < num_tokens; i++) {
    jsmntok_t *tok = &tokens[i];
    if (tok->parent == object_tok) {
      if (strlen(prop_name) == tok->end - tok->start &&
          strncmp(json + tok->start, prop_name, (size_t) tok->end - tok->start) == 0) {
        // The next token is the value
        return i + 1;
      }
    }
  }
  return -1;
}

#endif /* __UTIL_H_ */
