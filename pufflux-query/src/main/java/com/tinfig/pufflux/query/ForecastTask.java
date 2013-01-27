package com.tinfig.pufflux.query;

import java.net.URI;
import java.net.URISyntaxException;
import java.text.MessageFormat;

import javax.ws.rs.core.UriBuilder;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.ClientConfig;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.json.JSONConfiguration;
import com.tinfig.pufflux.query.transmitter.Animation;
import com.tinfig.pufflux.query.transmitter.CommandBuilder;
import com.tinfig.pufflux.query.wunderground.Alert;
import com.tinfig.pufflux.query.wunderground.PhenomenaCategory;
import com.tinfig.pufflux.query.wunderground.ResponseHolder;
import com.tinfig.pufflux.query.wunderground.Significance;

@Component
public class ForecastTask {
	private final static Logger LOG = LoggerFactory.getLogger(ForecastTask.class);
	private static final String WUNDERGROUND_API_BASE = "http://api.wunderground.com";
	private static final String WUNDERGROUND_API_PATH = "/api/{apiKey}/alerts/q/{state}/{city}.json";

	private final Client client;

	private String wundergroundApiKey;
	private String state;
	private String city;

	public ForecastTask() {
		ClientConfig clientConfig = new DefaultClientConfig();
		clientConfig.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
		client = Client.create(clientConfig);
	}

	public void setWundergroundApiKey(String wundergroundApiKey) {
		this.wundergroundApiKey = wundergroundApiKey;
	}

	public void setState(String state) {
		this.state = state;
	}

	public void setCity(String city) {
		this.city = city;
	}

	public void run() throws URISyntaxException {
		URI baseUri = new URI(WUNDERGROUND_API_BASE);
		URI uri = UriBuilder.fromUri(baseUri).path(WUNDERGROUND_API_PATH).build(wundergroundApiKey, state, city);

		WebResource resource = client.resource(uri);
		LOG.info(MessageFormat.format("Querying {0}", resource.getURI().toString()));
		ResponseHolder holder = resource.get(ResponseHolder.class);

		if (holder.getResponse() != null && holder.getResponse().getError() != null) {
			LOG.error("Error querying weather information: " + holder.getResponse().getError().getType() + "; "
					+ holder.getResponse().getError().getDescription());
			return;
		}

		Alert warning = holder.findFirst(Significance.WARNING);
		if (warning != null) {
			LOG.info("Found warning-level alert " + warning.toString());
			write(warning);
			return;
		}

		Alert watch = holder.findFirst(Significance.WATCH);
		if (watch != null) {
			LOG.info("Found watch-level alert " + watch.toString());
			write(watch);
			return;
		}

		Alert advisory = holder.findFirst(Significance.ADVISORY);
		if (advisory != null) {
			LOG.info("Found advisory-level alert " + advisory.toString());
			write(advisory);
			return;
		}

		// No alerts, do default
		LOG.info("No alerts, sending default animation command");
		write(new CommandBuilder().animate(Animation.DEFAULT).build());
	}

	private void write(Alert alert) {
		PhenomenaCategory category = alert.getPhenomena().getCategory();
		boolean warning = alert.getSignificance() == Significance.WARNING;
		short command = new CommandBuilder().animate(category.getAnimation()).baseColor(category.getBaseColor())
				.highlightColor(category.getHighlightColor()).speed(warning).build();
		write(command);
	}

	private void write(short command) {
		byte[] bytes = new byte[2];
		bytes[0] = (byte) (command & 0xff);
		bytes[1] = (byte) ((command >>> 8) & 0xff);

		// Write MSB first
		LOG.info(MessageFormat.format("Writing [0x{0},0x{1}] (0b{2},0b{3})", toHexString(bytes[1]),
				toHexString(bytes[0]), toBinaryString(bytes[1]), toBinaryString(bytes[0])));

		System.out.write(bytes[1]);
		System.out.write(bytes[0]);
		System.out.flush();
	}

	private String toHexString(byte b) {
		String hex = Integer.toHexString(b);
		if (hex.length() < 2) {
			return "0" + hex;
		}
		return hex;
	}

	private String toBinaryString(byte b) {
		int x = b;
		char[] chars = new char[8];
		for (int i = 7; i >= 0; i--) {
			chars[i] = Integer.lowestOneBit(x) == 1 ? '1' : '0';
			x = x >>> 1;
		}
		return new String(chars);
	}
}
