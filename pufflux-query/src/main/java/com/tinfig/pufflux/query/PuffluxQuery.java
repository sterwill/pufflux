package com.tinfig.pufflux.query;

import java.text.MessageFormat;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.config.AutowireCapableBeanFactory;
import org.springframework.context.ApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;
import org.springframework.stereotype.Component;

@Component
public class PuffluxQuery {
	private final static Logger LOG = LoggerFactory.getLogger(PuffluxQuery.class);

	private static final String[] APPLICATION_CONTEXTS = new String[] { "/applicationContext.xml", };

	@Autowired
	private AutowireCapableBeanFactory beanFactory;

	@Autowired
	private ForecastTask forecastTask;

	private String wundergroundApiKey;
	private String state;
	private String city;

	public static void main(String[] args) {
		if (args.length != 3) {
			System.err.println(MessageFormat.format("usage: {0} <wunderground-api-key> <state> <city>",
					PuffluxQuery.class.getName()));
			System.exit(1);
		}

		ApplicationContext ac = new ClassPathXmlApplicationContext(APPLICATION_CONTEXTS);
		AutowireCapableBeanFactory factory = ac.getAutowireCapableBeanFactory();
		PuffluxQuery program = factory.getBean(PuffluxQuery.class);
		program.setWundergroundApiKey(args[0]);
		program.setState(args[1]);
		program.setCity(args[2]);
		program.run();
	}

	public PuffluxQuery() {
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

	public void run() {
		forecastTask.setWundergroundApiKey(wundergroundApiKey);
		forecastTask.setState(state);
		forecastTask.setCity(city);
		try {
			forecastTask.run();
		} catch (Exception e) {
			LOG.error("Unhandled exception", e);
			System.exit(2);
		}
	}
}
