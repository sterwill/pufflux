package com.tinfig.pufflux.query.transmitter;

/**
 * A 4 bit (up to 16 item) indexed palette.
 */
public enum Color {
	BLACK(0),

	WHITE(1),

	RED(2),

	GREEN(3),

	BLUE(4),

	LIGHT_BLUE(5),

	DARK_BLUE(6),

	LIGHT_GRAY(7),

	DARK_GRAY(8),

	YELLOW(9),

	ORANGE(10);

	private final byte id;

	Color(int id) {
		this.id = (byte) id;
	}

	public byte getId() {
		return id;
	}
}
