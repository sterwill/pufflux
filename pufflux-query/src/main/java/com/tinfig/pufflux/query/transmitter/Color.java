package com.tinfig.pufflux.query.transmitter;

/**
 * A 4 bit (up to 16 item) indexed palette.
 */
public enum Color {
	BLACK(0),

	WHITE(1),

	LIGHT_BLUE(2),

	DARK_BLUE(3),

	LIGHT_GRAY(4),

	DARK_GRAY(5),

	YELLOW(6),

	ORANGE(7),

	RED(8);

	private final byte id;

	Color(int id) {
		this.id = (byte) id;
	}

	public byte getId() {
		return id;
	}
}
