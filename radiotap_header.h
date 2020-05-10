#ifndef RADIOTAP_HEADER_H
#define RADIOTAP_HEADER_H

struct ieee80211_radiotap_header {
	/**
	 * @it_version: radiotap version, always 0
	 */
	uint8_t it_version;

	/**
	 * @it_pad: padding (or alignment)
	 */
	uint8_t it_pad;

	/**
	 * @it_len: overall radiotap header length
	 */
	uint16_t it_len;

	/**
	 * @it_present: (first) present word
	 */
	uint32_t it_present;
} __packed;

#endif
