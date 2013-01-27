package com.tinfig.pufflux.query;

import java.text.MessageFormat;
import java.util.Arrays;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadPoolExecutor;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.stereotype.Component;

@Component
public class TaskRejectedExecutionHandler implements RejectedExecutionHandler {
	private final static Logger LOG = LoggerFactory.getLogger(TaskRejectedExecutionHandler.class);

	@Autowired
	private ApplicationContext appContext;

	@Override
	public void rejectedExecution(Runnable r, ThreadPoolExecutor executor) {
		String[] beanNamesForType = appContext.getBeanNamesForType(r.getClass());
		LOG.error(MessageFormat.format("Executor {0} rejected task {1} (bean names: {2})", executor, r,
				Arrays.toString(beanNamesForType)));
	}
}
