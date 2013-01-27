package com.tinfig.pufflux.query.wunderground;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class Error {
	@JsonProperty("type")
	private String type;

	@JsonProperty("description")
	private String description;

	public String getType() {
		return type;
	}

	public String getDescription() {
		return description;
	}
}
