package com.tinfig.pufflux.query.wunderground;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class Response {
	@JsonProperty("error")
	private Error error;

	public Error getError() {
		return error;
	}
}
