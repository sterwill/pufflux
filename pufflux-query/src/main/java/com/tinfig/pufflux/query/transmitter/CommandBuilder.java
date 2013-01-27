package com.tinfig.pufflux.query.transmitter;

/**
 * Builds 16-bit commands.
 */
public class CommandBuilder {
	// Bits 1-3 denote the type of animation
	private static final int ANIM_GET_MASK = 0b00000000_00000111;
	private static final int ANIM_SET_MASK = ~ANIM_GET_MASK;

	// Bit 4 is animation speed (1 fast, 0 slow)
	private static final int SPEED_GET_MASK = 0b00000000_00001000;
	private static final int SPEED_SET_MASK = ~SPEED_GET_MASK;

	// Bits 5-8 are unused (think of something!)

	// Bits 9-12 are the base color (some animations use this)
	private static final int BASE_COLOR_GET_MASK = 0b00001111_00000000;
	private static final int BASE_COLOR_SET_MASK = ~BASE_COLOR_GET_MASK;

	// Bits 13-16 are the highlight color (some animations use this)
	private static final int HIGHLIGHT_COLOR_GET_MASK = 0b11110000_00000000;
	private static final int HIGHLIGHT_COLOR_SET_MASK = ~HIGHLIGHT_COLOR_GET_MASK;

	private int command;

	public CommandBuilder() {
	}

	public CommandBuilder animate(Animation animation) {
		command = ((command & ANIM_SET_MASK) | (animation.getId() & ANIM_GET_MASK));
		return this;
	}

	public CommandBuilder speed(boolean fast) {
		command = ((command & SPEED_SET_MASK) | ((fast ? 1 : 0) << 5));
		return this;
	}

	public CommandBuilder baseColor(Color color) {
		command = ((command & BASE_COLOR_SET_MASK) | (color.getId() << 8 & BASE_COLOR_GET_MASK));
		return this;
	}

	public CommandBuilder highlightColor(Color color) {
		command = ((command & HIGHLIGHT_COLOR_SET_MASK) | (color.getId() << 12 & HIGHLIGHT_COLOR_GET_MASK));
		return this;
	}

	public short build() {
		return (short) command;
	}
}
