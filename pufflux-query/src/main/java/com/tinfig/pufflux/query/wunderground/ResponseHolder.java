package com.tinfig.pufflux.query.wunderground;

import java.util.ArrayList;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Holds a Wunderground API response object.
 */
@JsonIgnoreProperties(ignoreUnknown = true)
public class ResponseHolder {

	@JsonProperty("alerts")
	private List<Alert> alerts = new ArrayList<>();

	@JsonProperty("response")
	private Response response;

	public List<Alert> getAlerts() {
		return alerts;
	}

	public Response getResponse() {
		return response;
	}

	public Alert getHighestWarning() {
		for (Alert alert : getAlerts()) {
			if (alert.getSignificance() == Significance.W) {
				return alert;
			}
		}
		return null;
	}

	public Alert getHighestWatch() {
		for (Alert alert : getAlerts()) {
			if (alert.getSignificance() == Significance.A) {
				return alert;
			}
		}
		return null;
	}

	public Alert getHighestAdvisory() {
		for (Alert alert : getAlerts()) {
			if (alert.getSignificance() == Significance.Y) {
				return alert;
			}
		}
		return null;
	}
}
