/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/arm/cortex_m/cmsis.h>
#include <cortex_m/tz.h>

static void configure_nonsecure_vtor_offset(u32_t vtor_ns)
{
	SCB_NS->VTOR = vtor_ns;
}

static void configure_nonsecure_msp(u32_t msp_ns)
{
	__TZ_set_MSP_NS(msp_ns);
}

static void configure_nonsecure_psp(u32_t psp_ns)
{
	__TZ_set_PSP_NS(psp_ns);
}

static void configure_nonsecure_control(u32_t spsel_ns, u32_t npriv_ns)
{
	u32_t control_ns = __TZ_get_CONTROL_NS();

	/* Only nPRIV and SPSEL bits are banked between security states. */
	control_ns &= ~(CONTROL_SPSEL_Msk | CONTROL_nPRIV_Msk);

	if (spsel_ns) {
		control_ns |= CONTROL_SPSEL_Msk;
	}
	if (npriv_ns) {
		control_ns |= CONTROL_nPRIV_Msk;
	}

	__TZ_set_CONTROL_NS(control_ns);
}

void tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf)
{
	configure_nonsecure_vtor_offset(p_ns_conf->vtor_ns);
	configure_nonsecure_msp(p_ns_conf->msp_ns);
	configure_nonsecure_psp(p_ns_conf->psp_ns);
	/* Select which stack-pointer to use (MSP or PSP) and
	 * the privilege level for thread mode.
	 */
	configure_nonsecure_control(p_ns_conf->control_ns.spsel,
		p_ns_conf->control_ns.npriv);
}