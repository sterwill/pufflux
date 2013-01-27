package com.tinfig.pufflux.query.wunderground;

/**
 * Enumerates P-VTEC-format Phenomena (pp field). Should probably be called
 * "phenomenon", but Wunderground and VTEC use the plural.
 */
public enum Phenomena {
	AF("Ashfall", PhenomenaCategory.SMOKE),

	AS("Air Stagnation", PhenomenaCategory.AIR_QUALITY),

	BS("Blowing Snow", PhenomenaCategory.SNOW),

	BW("Brisk Wind", PhenomenaCategory.WIND),

	BZ("Blizzard", PhenomenaCategory.SNOW),

	CF("Coastal Flood", PhenomenaCategory.FLOOD),

	DS("Dust Storm", PhenomenaCategory.DUST),

	DU("Blowing Dust", PhenomenaCategory.DUST),

	EC("Extreme Cold", PhenomenaCategory.COLD),

	EH("Excessive Heat", PhenomenaCategory.HEAT),

	EW("Extreme Wind", PhenomenaCategory.WIND),

	FA("Areal Flood", PhenomenaCategory.FLOOD),

	FF("Flash Flood", PhenomenaCategory.FLOOD),

	FG("Dense Fog", PhenomenaCategory.FOG),

	FL("Flood", PhenomenaCategory.FLOOD),

	FR("Frost", PhenomenaCategory.FREEZE),

	FW("Fire Weather", PhenomenaCategory.FIRE),

	FZ("Freeze", PhenomenaCategory.FREEZE),

	GL("Gale", PhenomenaCategory.WIND),

	HF("Hurricane Force Wind", PhenomenaCategory.WIND),

	HI("Inland Hurricane", PhenomenaCategory.WIND),

	HS("Heavy Snow", PhenomenaCategory.SNOW),

	HT("Heat", PhenomenaCategory.HEAT),

	HU("Hurricane", PhenomenaCategory.STORM),

	HW("High Wind", PhenomenaCategory.WIND),

	HY("Hydrologic", PhenomenaCategory.FLOOD),

	HZ("Hard Freeze", PhenomenaCategory.FREEZE),

	IP("Sleet", PhenomenaCategory.ICE),

	IS("Ice Storm", PhenomenaCategory.ICE),

	LB("Lake Effect Snow and Blowing Snow", PhenomenaCategory.SNOW),

	LE("Lake Effect Snow", PhenomenaCategory.SNOW),

	LO("Low Water", PhenomenaCategory.LOW_WATER),

	LS("Lakeshore Flood", PhenomenaCategory.FLOOD),

	LW("Lake Wind", PhenomenaCategory.WIND),

	MA("Marine", PhenomenaCategory.MARINE),

	RB("Small Craft for Rough Bar", PhenomenaCategory.MARINE),

	SB("Snow and Blowing Snow", PhenomenaCategory.SNOW),

	SC("Small Craft", PhenomenaCategory.MARINE),

	SE("Hazardous Seas", PhenomenaCategory.MARINE),

	SI("Small Craft for Winds", PhenomenaCategory.MARINE),

	SM("Dense Smoke", PhenomenaCategory.SMOKE),

	SN("Snow", PhenomenaCategory.SNOW),

	SR("Storm", PhenomenaCategory.STORM),

	SU("High Surf", PhenomenaCategory.MARINE),

	SV("Severe Thunderstorm", PhenomenaCategory.STORM),

	SW("Small Craft for Hazardous Seas", PhenomenaCategory.MARINE),

	TI("Inland Tropical Storm", PhenomenaCategory.STORM),

	TO("Tornado", PhenomenaCategory.TORNADO),

	TR("Tropical Storm", PhenomenaCategory.STORM),

	TS("Tsunami", PhenomenaCategory.MARINE),

	TY("Typhoon", PhenomenaCategory.STORM),

	UP("Ice Accretion", PhenomenaCategory.ICE),

	WC("Wind Chill", PhenomenaCategory.COLD),

	WI("Wind", PhenomenaCategory.WIND),

	WS("Winter Storm", PhenomenaCategory.STORM),

	WW("Winter Weather", PhenomenaCategory.ICE),

	ZF("Freezing Fog", PhenomenaCategory.ICE),

	ZR("Freezing Rain", PhenomenaCategory.ICE);

	private final String description;
	private final PhenomenaCategory category;

	private Phenomena(String description, PhenomenaCategory category) {
		this.description = description;
		this.category = category;
	}

	public String getDescription() {
		return description;
	}

	public PhenomenaCategory getCategory() {
		return category;
	}
}
