package com.tinfig.pufflux.query.wunderground;

import com.tinfig.pufflux.query.transmitter.Animation;
import com.tinfig.pufflux.query.transmitter.Color;

;

/**
 * Arbitrary categories for types of VTEC phenomena. Defines the animation and
 * colors to use for each.
 */
public enum PhenomenaCategory {
	AIR_QUALITY(Animation.PULSE, Color.LIGHT_GRAY, Color.YELLOW),

	COLD(Animation.PULSE, Color.LIGHT_GRAY, Color.DARK_BLUE),

	HEAT(Animation.PULSE, Color.WHITE, Color.ORANGE),

	FLOOD(Animation.FLOOD, Color.BLACK, Color.DARK_BLUE),

	// A flood of muddy water
	LOW_WATER(Animation.FLOOD, Color.BLACK, Color.YELLOW),

	// Blue waves rippling
	MARINE(Animation.FLOOD, Color.DARK_BLUE, Color.LIGHT_BLUE),

	SNOW(Animation.PRECIPITATION, Color.BLACK, Color.WHITE),

	WIND(Animation.SWIRL, Color.DARK_GRAY, Color.LIGHT_GRAY),

	DUST(Animation.PULSE, Color.LIGHT_GRAY, Color.YELLOW),

	FOG(Animation.PULSE, Color.DARK_GRAY, Color.LIGHT_GRAY),

	FREEZE(Animation.PULSE, Color.LIGHT_GRAY, Color.LIGHT_BLUE),

	FIRE(Animation.PULSE, Color.BLACK, Color.ORANGE),

	STORM(Animation.PRECIPITATION, Color.DARK_BLUE, Color.LIGHT_BLUE),

	ICE(Animation.PRECIPITATION, Color.BLACK, Color.LIGHT_BLUE),

	SMOKE(Animation.PULSE, Color.BLACK, Color.DARK_GRAY),

	// This one should get some attention
	TORNADO(Animation.SWIRL, Color.WHITE, Color.RED);

	private final Animation animation;
	private final Color baseColor;
	private final Color highlightColor;

	PhenomenaCategory(Animation animation, Color baseColor, Color highlightColor) {
		this.animation = animation;
		this.baseColor = baseColor;
		this.highlightColor = highlightColor;
	}

	public Animation getAnimation() {
		return animation;
	}

	public Color getBaseColor() {
		return baseColor;
	}

	public Color getHighlightColor() {
		return highlightColor;
	}
}
