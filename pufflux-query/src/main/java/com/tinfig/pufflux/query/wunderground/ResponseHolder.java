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

	public Alert findFirst(Significance significance) {
		for (Alert alert : getAlerts()) {
			if (alert.getSignificance() == significance) {
				return alert;
			}
		}
		return null;
	}
}
