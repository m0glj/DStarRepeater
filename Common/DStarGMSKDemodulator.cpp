/*
 *	Copyright (C) 2009,2010,2013,2014,2015 by Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "DStarGMSKDemodulator.h"

// Generated by gaussfir(0.5, 4, 10)
const wxFloat32 COEFFS_TABLE[] = {
	0.000000000000003F, 0.000000000000065F, 0.000000000001037F, 0.000000000014448F, 0.000000000174579F,
	0.000000001829471F, 0.000000016627294F, 0.000000131062698F, 0.000000895979719F, 0.000005312253663F, 0.000027316243802F,
	0.000121821714020F, 0.000471183399421F, 0.001580581180127F, 0.004598383433830F, 0.011602594308899F, 0.025390226926262F,
	0.048188078330624F, 0.079318443411643F, 0.113232294527059F, 0.140193533802410F, 0.150538369557851F, 0.140193533802410F,
	0.113232294527059F, 0.079318443411643F, 0.048188078330623F, 0.025390226926262F, 0.011602594308899F, 0.004598383433830F,
	0.001580581180127F, 0.000471183399421F, 0.000121821714020F, 0.000027316243802F, 0.000005312253663F, 0.000000895979719F,
	0.000000131062698F, 0.000000016627294F, 0.000000001829471F, 0.000000000174579F, 0.000000000014448F, 0.000000000001037F,
	0.000000000000065F, 0.000000000000003F};

const unsigned int COEFFS_LENGTH = 43U;

const unsigned int DCOFFSET_COUNT = 4800U;		// 5 seconds

const unsigned int PLLMAX     = 0x10000U;
const unsigned int PLLINC     = PLLMAX / DSTAR_RADIO_BIT_LENGTH;
const unsigned int INC_LOCK   = PLLINC / 64U;
const unsigned int INC_UNLOCK = PLLINC / 32U;

CDStarGMSKDemodulator::CDStarGMSKDemodulator() :
m_filter(COEFFS_TABLE, COEFFS_LENGTH),
m_invert(false),
m_pll(0U),
m_prev(false),
m_inc(INC_UNLOCK),
m_locked(false),
m_offset(0.0F),
m_accum(0.0F),
m_count(0U)
{
}

CDStarGMSKDemodulator::~CDStarGMSKDemodulator()
{
}

TRISTATE CDStarGMSKDemodulator::decode(wxFloat32 val)
{
	TRISTATE state = STATE_UNKNOWN;

	// Calculate the DC offset when we are locked
	if (m_locked) {
		m_accum += val;
		m_count++;

		if (m_count >= DCOFFSET_COUNT) {
			m_accum /= wxFloat32(DCOFFSET_COUNT);

			m_offset = m_offset * 0.9F + m_accum * 0.1F;

			m_accum = 0.0F;
			m_count = 0U;
		}
	}

	wxFloat32 out = m_filter.process(val - m_offset);

	bool bit = out > 0.0F;

	if (bit != m_prev) {
		if (m_pll < (PLLMAX / 2U))
			m_pll += m_inc;
		else
			m_pll -= m_inc;
	}

	m_prev = bit;

	m_pll += PLLINC;

	if (m_pll >= PLLMAX) {
		if (m_invert)
			state = bit ? STATE_TRUE : STATE_FALSE;
		else
			state = bit ? STATE_FALSE : STATE_TRUE;

		m_pll -= PLLMAX;
	}

	return state;
}

void CDStarGMSKDemodulator::reset()
{
	m_pll    = 0U;
	m_prev   = false;
	m_inc    = INC_UNLOCK;
	m_locked = false;
}

void CDStarGMSKDemodulator::setInvert(bool set)
{
	m_invert = set;
}

void CDStarGMSKDemodulator::lock(bool on)
{
	// Debugging only XXX
	wxLogMessage(wxT("Current DC offset: %f"), m_offset);

	m_locked = on;

	m_inc = on ? INC_LOCK : INC_UNLOCK;
}