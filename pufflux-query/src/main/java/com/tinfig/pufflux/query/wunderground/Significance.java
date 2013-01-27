package com.tinfig.pufflux.query.wunderground;

/**
 * Enumerates P-VTEC-format Significance (s field)
 */
public enum Significance {
	WARNING("W", "Warning"),

	WATCH("A", "Watch"),

	ADVISORY("Y", "Advisory"),

	STATEMENT("S", "Statement"),

	FORECAST("F", "Forecast"),

	OUTLOOK("O", "Outlook"),

	SYNOPSIS("N", "Synopsis");

	private final String vtec;
	private final String description;

	private Significance(String vtec, String description) {
		this.vtec = vtec;
		this.description = description;
	}

	public String getVtec() {
		return vtec;
	}

	public String getDescription() {
		return description;
	}

	public static Significance fromVtec(String vtec) {
		if (vtec == null) {
			return null;
		}
		for (Significance sig : Significance.values()) {
			if (sig.getVtec().equals(vtec)) {
				return sig;
			}
		}
		return null;
	}
}
