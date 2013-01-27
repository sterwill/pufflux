package com.tinfig.pufflux.query.wunderground;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class Alert {
	@JsonProperty("phenomena")
	private Phenomena phenomena;

	@JsonProperty("significance")
	private Significance significance;

	public Phenomena getPhenomena() {
		return phenomena;
	}

	public Significance getSignificance() {
		return significance;
	}

	@Override
	public String toString() {
		return "Alert [phenomena=" + phenomena.getDescription() + ", significance=" + significance.getDescription()
				+ "]";
	}
}
