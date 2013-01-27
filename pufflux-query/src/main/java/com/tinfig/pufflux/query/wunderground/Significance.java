package com.tinfig.pufflux.query.wunderground;

/**
 * Enumerates P-VTEC-format Significance (s field)
 */
public enum Significance {
	W("Warning"),

	A("Watch"),

	Y("Advisory"),

	S("Statement"),

	F("Forecast"),

	O("Outlook"),

	N("Synopsis");

	private final String description;

	private Significance(String description) {
		this.description = description;
	}

	public String getDescription() {
		return description;
	}
}
