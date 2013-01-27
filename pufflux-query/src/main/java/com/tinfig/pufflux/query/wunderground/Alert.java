package com.tinfig.pufflux.query.wunderground;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class Alert {
	@JsonProperty("phenomena")
	private String phenomenaVtec;

	@JsonProperty("significance")
	private String significanceVtec;

	@JsonIgnoreProperties(ignoreUnknown = true)
	public Phenomena getPhenomena() {
		return Phenomena.fromVtec(phenomenaVtec);
	}

	public Significance getSignificance() {
		return Significance.fromVtec(significanceVtec);
	}

	@Override
	public String toString() {
		return "Alert [phenomena=" + getPhenomena() + ", significance=" + getSignificance() + "]";
	}
}
