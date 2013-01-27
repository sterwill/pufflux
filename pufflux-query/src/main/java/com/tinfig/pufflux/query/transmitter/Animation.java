package com.tinfig.pufflux.query.transmitter;

/**
 * A 3 bit (up to 8 item) enumeration of animations.
 */
public enum Animation {
	/**
	 * Tells the cloud to do its default animation. The speed and color bits in
	 * the command will be ignored.
	 */
	DEFAULT(0),

	PRECIPITATION(1),

	FLOOD(2),

	PULSE(3),

	SWIRL(4);

	private final byte id;

	Animation(int id) {
		this.id = (byte) id;
	}

	public byte getId() {
		return id;
	}
}
