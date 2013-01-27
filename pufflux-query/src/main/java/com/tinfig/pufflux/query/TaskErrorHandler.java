package com.tinfig.pufflux.query;

import java.text.MessageFormat;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;
import org.springframework.util.ErrorHandler;

@Component
public class TaskErrorHandler implements ErrorHandler {
	private final static Logger LOG = LoggerFactory.getLogger(TaskErrorHandler.class);

	@Override
	public void handleError(Throwable t) {
		LOG.error(MessageFormat.format("Error in task {0}", t), t);
	}
}
