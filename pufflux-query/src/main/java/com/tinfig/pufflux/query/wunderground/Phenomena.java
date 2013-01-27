package com.tinfig.pufflux.query.wunderground;

/**
 * Enumerates P-VTEC-format Phenomena (pp field). Should probably be called
 * "phenomenon", but Wunderground and VTEC use the plural.
 */
public enum Phenomena {
	ASHFALL("AF", "Ashfall", PhenomenaCategory.AIR_QUALITY),

	AIR_STAGNATION("AS", "Air Stagnation", PhenomenaCategory.AIR_QUALITY),

	BLOWING_SNOW("BS", "Blowing Snow", PhenomenaCategory.SNOW),

	BRISK_WIND("BW", "Brisk Wind", PhenomenaCategory.WIND),

	BLIZZARD("BZ", "Blizzard", PhenomenaCategory.SNOW),

	COASTAL_FLOOD("CF", "Coastal Flood", PhenomenaCategory.FLOOD),

	DUST_STORM("DS", "Dust Storm", PhenomenaCategory.DUST),

	BLOWING_DUST("DU", "Blowing Dust", PhenomenaCategory.DUST),

	EXTREME_COLD("EC", "Extreme Cold", PhenomenaCategory.COLD),

	EXCESSIVE_HEAT("EH", "Excessive Heat", PhenomenaCategory.HEAT),

	EXTREME_WIND("EW", "Extreme Wind", PhenomenaCategory.WIND),

	AREAL_FLOOD("FA", "Areal Flood", PhenomenaCategory.FLOOD),

	FLASH_FLOOD("FF", "Flash Flood", PhenomenaCategory.FLOOD),

	DENSE_FOG("FG", "Dense Fog", PhenomenaCategory.FOG),

	FLOOD("FL", "Flood", PhenomenaCategory.FLOOD),

	FROST("FR", "Frost", PhenomenaCategory.FREEZE),

	FIRE_WEATHER("FW", "Fire Weather", PhenomenaCategory.FIRE),

	FREEZE("FZ", "Freeze", PhenomenaCategory.FREEZE),

	GALE("GL", "Gale", PhenomenaCategory.WIND),

	HURRICANE_FORCE_WIND("HF", "Hurricane Force Wind", PhenomenaCategory.WIND),

	INLAND_HURRICANE("HI", "Inland Hurricane", PhenomenaCategory.WIND),

	HEAVY_SNOW("HS", "Heavy Snow", PhenomenaCategory.SNOW),

	HEAT("HT", "Heat", PhenomenaCategory.HEAT),

	HURRICANE("HU", "Hurricane", PhenomenaCategory.STORM),

	HIGH_WIND("HW", "High Wind", PhenomenaCategory.WIND),

	HYDROLOGIC("HY", "Hydrologic", PhenomenaCategory.FLOOD),

	HARD_FREEZE("HZ", "Hard Freeze", PhenomenaCategory.FREEZE),

	SLEET("IP", "Sleet", PhenomenaCategory.ICE),

	ICE_STORM("IS", "Ice Storm", PhenomenaCategory.ICE),

	LAKE_EFFECT_SNOW_AND_BLOWING_SNOW("LB", "Lake Effect Snow and Blowing Snow", PhenomenaCategory.SNOW),

	LAKE_EFFECT_SNOW("LE", "Lake Effect Snow", PhenomenaCategory.SNOW),

	LOW_WATER("LO", "Low Water", PhenomenaCategory.LOW_WATER),

	LAKESHORE_FLOOD("LS", "Lakeshore Flood", PhenomenaCategory.FLOOD),

	LAKE_WIND("LW", "Lake Wind", PhenomenaCategory.WIND),

	MARINE("MA", "Marine", PhenomenaCategory.MARINE),

	SMALL_CRAFT_FOR_ROUGH_BAR("RB", "Small Craft for Rough Bar", PhenomenaCategory.MARINE),

	SNOW_AND_BLOWING_SNOW("SB", "Snow and Blowing Snow", PhenomenaCategory.SNOW),

	SMALL_CRAFT("SC", "Small Craft", PhenomenaCategory.MARINE),

	HAZARDOUS_SEAS("SE", "Hazardous Seas", PhenomenaCategory.MARINE),

	SMALL_CRAFT_FOR_WINDS("SI", "Small Craft for Winds", PhenomenaCategory.MARINE),

	DENSE_SMOKE("SM", "Dense Smoke", PhenomenaCategory.AIR_QUALITY),

	SNOW("SN", "Snow", PhenomenaCategory.SNOW),

	STORM("SR", "Storm", PhenomenaCategory.STORM),

	HIGH_SURF("SU", "High Surf", PhenomenaCategory.MARINE),

	SEVERE_THUNDERSTORM("SV", "Severe Thunderstorm", PhenomenaCategory.STORM),

	SMALL_CRAFT_FOR_HAZARDOUS_SEAS("SW", "Small Craft for Hazardous Seas", PhenomenaCategory.MARINE),

	INLAND_TROPICAL_STORM("TI", "Inland Tropical Storm", PhenomenaCategory.STORM),

	TORNADO("TO", "Tornado", PhenomenaCategory.TORNADO),

	TROPICAL_STORm("TR", "Tropical Storm", PhenomenaCategory.STORM),

	TSUNAMI("TS", "Tsunami", PhenomenaCategory.MARINE),

	TYPHOON("TY", "Typhoon", PhenomenaCategory.STORM),

	ICE_ACCRETION("UP", "Ice Accretion", PhenomenaCategory.ICE),

	WIND_CHILL("WC", "Wind Chill", PhenomenaCategory.COLD),

	WIND("WI", "Wind", PhenomenaCategory.WIND),

	WINTER_STORM("WS", "Winter Storm", PhenomenaCategory.STORM),

	WINTER_WEATHER("WW", "Winter Weather", PhenomenaCategory.ICE),

	FREEZING_FOG("ZF", "Freezing Fog", PhenomenaCategory.ICE),

	FREEZING_RAIN("ZR", "Freezing Rain", PhenomenaCategory.ICE);

	private final String vtec;
	private final String description;
	private final PhenomenaCategory category;

	private Phenomena(String vtec, String description, PhenomenaCategory category) {
		this.vtec = vtec;
		this.description = description;
		this.category = category;
	}

	public String getVtec() {
		return vtec;
	}

	public String getDescription() {
		return description;
	}

	public PhenomenaCategory getCategory() {
		return category;
	}

	public static Phenomena fromVtec(String vtec) {
		if (vtec == null) {
			return null;
		}
		for (Phenomena phen : Phenomena.values()) {
			if (phen.getVtec().equals(vtec)) {
				return phen;
			}
		}
		return null;
	}
}
