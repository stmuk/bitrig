/*	$OpenBSD: ixgbe_82599.c,v 1.12 2015/04/30 14:17:26 jsg Exp $	*/

/******************************************************************************

  Copyright (c) 2001-2013, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/* FreeBSD: src/sys/dev/ixgbe/ixgbe_82599.c 251964 Jun 18 21:28:19 2013 UTC */

#include <dev/pci/ixgbe.h>
#include <dev/pci/ixgbe_type.h>

int32_t ixgbe_get_link_capabilities_82599(struct ixgbe_hw *hw,
					  ixgbe_link_speed *speed,
					  bool *autoneg);
enum ixgbe_media_type ixgbe_get_media_type_82599(struct ixgbe_hw *hw);
void ixgbe_disable_tx_laser_multispeed_fiber(struct ixgbe_hw *hw);
void ixgbe_enable_tx_laser_multispeed_fiber(struct ixgbe_hw *hw);
void ixgbe_flap_tx_laser_multispeed_fiber(struct ixgbe_hw *hw);
void ixgbe_set_fiber_fixed_speed(struct ixgbe_hw *hw,
				 ixgbe_link_speed speed);
int32_t ixgbe_setup_mac_link_multispeed_fiber(struct ixgbe_hw *hw,
					      ixgbe_link_speed speed,
					      bool autoneg_wait_to_complete);
int32_t ixgbe_setup_mac_link_smartspeed(struct ixgbe_hw *hw,
				     ixgbe_link_speed speed,
				     bool autoneg_wait_to_complete);
int32_t ixgbe_start_mac_link_82599(struct ixgbe_hw *hw,
				   bool autoneg_wait_to_complete);
void ixgbe_stop_mac_link_on_d3_82599(struct ixgbe_hw *hw);
int32_t ixgbe_setup_mac_link_82599(struct ixgbe_hw *hw,
				     ixgbe_link_speed speed,
				     bool autoneg_wait_to_complete);
int32_t ixgbe_setup_copper_link_82599(struct ixgbe_hw *hw,
				      ixgbe_link_speed speed,
				      bool autoneg_wait_to_complete);
int32_t ixgbe_setup_sfp_modules_82599(struct ixgbe_hw *hw);
void ixgbe_init_mac_link_ops_82599(struct ixgbe_hw *hw);
int32_t ixgbe_reset_hw_82599(struct ixgbe_hw *hw);
int32_t ixgbe_read_analog_reg8_82599(struct ixgbe_hw *hw, uint32_t reg,
				     uint8_t *val);
int32_t ixgbe_write_analog_reg8_82599(struct ixgbe_hw *hw, uint32_t reg,
				      uint8_t val);
int32_t ixgbe_start_hw_82599(struct ixgbe_hw *hw);
int32_t ixgbe_identify_phy_82599(struct ixgbe_hw *hw);
int32_t ixgbe_init_phy_ops_82599(struct ixgbe_hw *hw);
uint32_t ixgbe_get_supported_physical_layer_82599(struct ixgbe_hw *hw);
int32_t ixgbe_enable_rx_dma_82599(struct ixgbe_hw *hw, uint32_t regval);
int32_t ixgbe_verify_fw_version_82599(struct ixgbe_hw *hw);
bool ixgbe_verify_lesm_fw_enabled_82599(struct ixgbe_hw *hw);
int32_t ixgbe_read_eeprom_82599(struct ixgbe_hw *hw,
				uint16_t offset, uint16_t *data);
int32_t ixgbe_reset_pipeline_82599(struct ixgbe_hw *hw);
int32_t ixgbe_blink_led_start_82599(struct ixgbe_hw *hw, uint32_t index);
int32_t ixgbe_blink_led_stop_82599(struct ixgbe_hw *hw, uint32_t index);

static bool ixgbe_mng_enabled(struct ixgbe_hw *hw)
{
	uint32_t fwsm, manc, factps;

	fwsm = IXGBE_READ_REG(hw, IXGBE_FWSM);
	if ((fwsm & IXGBE_FWSM_MODE_MASK) != IXGBE_FWSM_FW_MODE_PT)
		return FALSE;

	manc = IXGBE_READ_REG(hw, IXGBE_MANC);
	if (!(manc & IXGBE_MANC_RCV_TCO_EN))
		return FALSE;

	factps = IXGBE_READ_REG(hw, IXGBE_FACTPS);
	if (factps & IXGBE_FACTPS_MNGCG)
		return FALSE;

	return TRUE;
}

void ixgbe_init_mac_link_ops_82599(struct ixgbe_hw *hw)
{
	struct ixgbe_mac_info *mac = &hw->mac;

	DEBUGFUNC("ixgbe_init_mac_link_ops_82599");

	/*
	 * enable the laser control functions for SFP+ fiber
	 * and MNG not enabled
	 */
	if ((mac->ops.get_media_type(hw) == ixgbe_media_type_fiber) &&
	    !hw->mng_fw_enabled) {
		mac->ops.disable_tx_laser =
				       &ixgbe_disable_tx_laser_multispeed_fiber;
		mac->ops.enable_tx_laser =
					&ixgbe_enable_tx_laser_multispeed_fiber;
		mac->ops.flap_tx_laser = &ixgbe_flap_tx_laser_multispeed_fiber;

	} else {
		mac->ops.disable_tx_laser = NULL;
		mac->ops.enable_tx_laser = NULL;
		mac->ops.flap_tx_laser = NULL;
	}

	if (hw->phy.multispeed_fiber) {
		/* Set up dual speed SFP+ support */
		mac->ops.setup_link = &ixgbe_setup_mac_link_multispeed_fiber;
	} else {
		if ((ixgbe_get_media_type(hw) == ixgbe_media_type_backplane) &&
		     (hw->phy.smart_speed == ixgbe_smart_speed_auto ||
		      hw->phy.smart_speed == ixgbe_smart_speed_on) &&
		      !ixgbe_verify_lesm_fw_enabled_82599(hw)) {
			mac->ops.setup_link = &ixgbe_setup_mac_link_smartspeed;
		} else {
			mac->ops.setup_link = &ixgbe_setup_mac_link_82599;
		}
	}
}

/**
 *  ixgbe_init_phy_ops_82599 - PHY/SFP specific init
 *  @hw: pointer to hardware structure
 *
 *  Initialize any function pointers that were not able to be
 *  set during init_shared_code because the PHY/SFP type was
 *  not known.  Perform the SFP init if necessary.
 *
 **/
int32_t ixgbe_init_phy_ops_82599(struct ixgbe_hw *hw)
{
	struct ixgbe_mac_info *mac = &hw->mac;
	struct ixgbe_phy_info *phy = &hw->phy;
	int32_t ret_val = IXGBE_SUCCESS;

	DEBUGFUNC("ixgbe_init_phy_ops_82599");

	/* Identify the PHY or SFP module */
	ret_val = phy->ops.identify(hw);
	if (ret_val == IXGBE_ERR_SFP_NOT_SUPPORTED)
		goto init_phy_ops_out;

	/* Setup function pointers based on detected SFP module and speeds */
	ixgbe_init_mac_link_ops_82599(hw);
	if (hw->phy.sfp_type != ixgbe_sfp_type_unknown)
		hw->phy.ops.reset = NULL;

	/* If copper media, overwrite with copper function pointers */
	if (mac->ops.get_media_type(hw) == ixgbe_media_type_copper) {
		mac->ops.setup_link = &ixgbe_setup_copper_link_82599;
		mac->ops.get_link_capabilities =
				  &ixgbe_get_copper_link_capabilities_generic;
	}

	/* Set necessary function pointers based on phy type */
	switch (hw->phy.type) {
	case ixgbe_phy_tn:
		phy->ops.setup_link = &ixgbe_setup_phy_link_tnx;
		phy->ops.check_link = &ixgbe_check_phy_link_tnx;
		phy->ops.get_firmware_version =
			     &ixgbe_get_phy_firmware_version_tnx;
		break;
	default:
		break;
	}
init_phy_ops_out:
	return ret_val;
}

int32_t ixgbe_setup_sfp_modules_82599(struct ixgbe_hw *hw)
{
	int32_t ret_val = IXGBE_SUCCESS;
	uint16_t list_offset, data_offset, data_value;
	bool got_lock = FALSE;

	DEBUGFUNC("ixgbe_setup_sfp_modules_82599");

	if (hw->phy.sfp_type != ixgbe_sfp_type_unknown) {
		ixgbe_init_mac_link_ops_82599(hw);

		hw->phy.ops.reset = NULL;

		ret_val = ixgbe_get_sfp_init_sequence_offsets(hw, &list_offset,
							      &data_offset);
		if (ret_val != IXGBE_SUCCESS)
			goto setup_sfp_out;

		/* PHY config will finish before releasing the semaphore */
		ret_val = hw->mac.ops.acquire_swfw_sync(hw,
							IXGBE_GSSR_MAC_CSR_SM);
		if (ret_val != IXGBE_SUCCESS) {
			ret_val = IXGBE_ERR_SWFW_SYNC;
			goto setup_sfp_out;
		}

		if (hw->eeprom.ops.read(hw, ++data_offset, &data_value))
			goto setup_sfp_err;
		while (data_value != 0xffff) {
			IXGBE_WRITE_REG(hw, IXGBE_CORECTL, data_value);
			IXGBE_WRITE_FLUSH(hw);
			if (hw->eeprom.ops.read(hw, ++data_offset, &data_value))
				goto setup_sfp_err;
		}

		/* Release the semaphore */
		hw->mac.ops.release_swfw_sync(hw, IXGBE_GSSR_MAC_CSR_SM);
		/* Delay obtaining semaphore again to allow FW access */
		msec_delay(hw->eeprom.semaphore_delay);

		/* Need SW/FW semaphore around AUTOC writes if LESM on,
		 * likewise reset_pipeline requires lock as it also writes
		 * AUTOC.
		 */
		if (ixgbe_verify_lesm_fw_enabled_82599(hw)) {
			ret_val = hw->mac.ops.acquire_swfw_sync(hw,
							IXGBE_GSSR_MAC_CSR_SM);
			if (ret_val != IXGBE_SUCCESS) {
				ret_val = IXGBE_ERR_SWFW_SYNC;
				goto setup_sfp_out;
			}

			got_lock = TRUE;
		}

		/* Restart DSP and set SFI mode */
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC, ((hw->mac.orig_autoc) |
				IXGBE_AUTOC_LMS_10G_SERIAL));
		hw->mac.cached_autoc = IXGBE_READ_REG(hw, IXGBE_AUTOC);
		ret_val = ixgbe_reset_pipeline_82599(hw);

		if (got_lock) {
			hw->mac.ops.release_swfw_sync(hw,
						      IXGBE_GSSR_MAC_CSR_SM);
			got_lock = FALSE;
		}

		if (ret_val) {
			DEBUGOUT("sfp module setup not complete\n");
			ret_val = IXGBE_ERR_SFP_SETUP_NOT_COMPLETE;
			goto setup_sfp_out;
		}

	}

setup_sfp_out:
	return ret_val;

setup_sfp_err:
	/* Release the semaphore */
	hw->mac.ops.release_swfw_sync(hw, IXGBE_GSSR_MAC_CSR_SM);
	/* Delay obtaining semaphore again to allow FW access */
	msec_delay(hw->eeprom.semaphore_delay);
	ERROR_REPORT2(IXGBE_ERROR_INVALID_STATE,
		      "eeprom read at offset %d failed", data_offset);
	return IXGBE_ERR_PHY;
}

/**
 *  ixgbe_init_ops_82599 - Inits func ptrs and MAC type
 *  @hw: pointer to hardware structure
 *
 *  Initialize the function pointers and assign the MAC type for 82599.
 *  Does not touch the hardware.
 **/

int32_t ixgbe_init_ops_82599(struct ixgbe_hw *hw)
{
	struct ixgbe_mac_info *mac = &hw->mac;
	struct ixgbe_phy_info *phy = &hw->phy;
	struct ixgbe_eeprom_info *eeprom = &hw->eeprom;
	int32_t ret_val;

	DEBUGFUNC("ixgbe_init_ops_82599");

	ixgbe_init_phy_ops_generic(hw);
	ret_val = ixgbe_init_ops_generic(hw);

	/* PHY */
	phy->ops.identify = &ixgbe_identify_phy_82599;
	phy->ops.init = &ixgbe_init_phy_ops_82599;

	/* MAC */
	mac->ops.reset_hw = &ixgbe_reset_hw_82599;
	mac->ops.enable_relaxed_ordering = &ixgbe_enable_relaxed_ordering_gen2;
	mac->ops.get_media_type = &ixgbe_get_media_type_82599;
	mac->ops.get_supported_physical_layer =
				    &ixgbe_get_supported_physical_layer_82599;
	mac->ops.disable_sec_rx_path = &ixgbe_disable_sec_rx_path_generic;
	mac->ops.enable_sec_rx_path = &ixgbe_enable_sec_rx_path_generic;
	mac->ops.enable_rx_dma = &ixgbe_enable_rx_dma_82599;
	mac->ops.read_analog_reg8 = &ixgbe_read_analog_reg8_82599;
	mac->ops.write_analog_reg8 = &ixgbe_write_analog_reg8_82599;
	mac->ops.start_hw = &ixgbe_start_hw_82599;

	/* RAR, Multicast, VLAN */
	mac->ops.set_vmdq = &ixgbe_set_vmdq_generic;
	mac->ops.clear_vmdq = &ixgbe_clear_vmdq_generic;
	mac->ops.insert_mac_addr = &ixgbe_insert_mac_addr_generic;
	mac->rar_highwater = 1;
	mac->ops.set_vfta = &ixgbe_set_vfta_generic;
	mac->ops.set_vlvf = &ixgbe_set_vlvf_generic;
	mac->ops.clear_vfta = &ixgbe_clear_vfta_generic;
	mac->ops.init_uta_tables = &ixgbe_init_uta_tables_generic;
	mac->ops.setup_sfp = &ixgbe_setup_sfp_modules_82599;

	mac->ops.verify_lesm_fw_enabled = &ixgbe_verify_lesm_fw_enabled_82599;
	mac->ops.reset_pipeline = &ixgbe_reset_pipeline_82599;
	mac->ops.stop_mac_link_on_d3 = &ixgbe_stop_mac_link_on_d3_82599;

	/* Link */
	mac->ops.get_link_capabilities = &ixgbe_get_link_capabilities_82599;
	mac->ops.check_link = &ixgbe_check_mac_link_generic;
	ixgbe_init_mac_link_ops_82599(hw);

	mac->mcft_size		= 128;
	mac->vft_size		= 128;
	mac->num_rar_entries	= 128;
	mac->rx_pb_size		= 512;
	mac->max_tx_queues	= 128;
	mac->max_rx_queues	= 128;
	mac->max_msix_vectors	= 0 /*ixgbe_get_pcie_msix_count_generic(hw)*/;

	hw->mbx.ops.init_params = ixgbe_init_mbx_params_pf;

	/* EEPROM */
	eeprom->ops.read = &ixgbe_read_eeprom_82599;

	/* Cache if MNG FW is up */
	hw->mng_fw_enabled = ixgbe_mng_enabled(hw);

	return ret_val;
}

/**
 *  ixgbe_get_link_capabilities_82599 - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: TRUE when autoneg or autotry is enabled
 *
 *  Determines the link capabilities by reading the AUTOC register.
 **/
int32_t ixgbe_get_link_capabilities_82599(struct ixgbe_hw *hw,
				      ixgbe_link_speed *speed,
				      bool *autoneg)
{
	int32_t status = IXGBE_SUCCESS;
	uint32_t autoc = 0;

	DEBUGFUNC("ixgbe_get_link_capabilities_82599");

	/* Check if 1G SFP module. */
	if (hw->phy.sfp_type == ixgbe_sfp_type_1g_cu_core0 ||
	    hw->phy.sfp_type == ixgbe_sfp_type_1g_cu_core1 ||
	    hw->phy.sfp_type == ixgbe_sfp_type_1g_lx_core0 ||
	    hw->phy.sfp_type == ixgbe_sfp_type_1g_lx_core1 ||
	    hw->phy.sfp_type == ixgbe_sfp_type_1g_sx_core0 ||
	    hw->phy.sfp_type == ixgbe_sfp_type_1g_sx_core1) {
		*speed = IXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = TRUE;
		goto out;
	}

	/*
	 * Determine link capabilities based on the stored value of AUTOC,
	 * which represents EEPROM defaults.  If AUTOC value has not
	 * been stored, use the current register values.
	 */
	if (hw->mac.orig_link_settings_stored)
		autoc = hw->mac.orig_autoc;
	else
		autoc = IXGBE_READ_REG(hw, IXGBE_AUTOC);

	switch (autoc & IXGBE_AUTOC_LMS_MASK) {
	case IXGBE_AUTOC_LMS_1G_LINK_NO_AN:
		*speed = IXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = FALSE;
		break;

	case IXGBE_AUTOC_LMS_10G_LINK_NO_AN:
		*speed = IXGBE_LINK_SPEED_10GB_FULL;
		*autoneg = FALSE;
		break;

	case IXGBE_AUTOC_LMS_1G_AN:
		*speed = IXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = TRUE;
		break;

	case IXGBE_AUTOC_LMS_10G_SERIAL:
		*speed = IXGBE_LINK_SPEED_10GB_FULL;
		*autoneg = FALSE;
		break;

	case IXGBE_AUTOC_LMS_KX4_KX_KR:
	case IXGBE_AUTOC_LMS_KX4_KX_KR_1G_AN:
		*speed = IXGBE_LINK_SPEED_UNKNOWN;
		if (autoc & IXGBE_AUTOC_KR_SUPP)
			*speed |= IXGBE_LINK_SPEED_10GB_FULL;
		if (autoc & IXGBE_AUTOC_KX4_SUPP)
			*speed |= IXGBE_LINK_SPEED_10GB_FULL;
		if (autoc & IXGBE_AUTOC_KX_SUPP)
			*speed |= IXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = TRUE;
		break;

	case IXGBE_AUTOC_LMS_KX4_KX_KR_SGMII:
		*speed = IXGBE_LINK_SPEED_100_FULL;
		if (autoc & IXGBE_AUTOC_KR_SUPP)
			*speed |= IXGBE_LINK_SPEED_10GB_FULL;
		if (autoc & IXGBE_AUTOC_KX4_SUPP)
			*speed |= IXGBE_LINK_SPEED_10GB_FULL;
		if (autoc & IXGBE_AUTOC_KX_SUPP)
			*speed |= IXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = TRUE;
		break;

	case IXGBE_AUTOC_LMS_SGMII_1G_100M:
		*speed = IXGBE_LINK_SPEED_1GB_FULL | IXGBE_LINK_SPEED_100_FULL;
		*autoneg = FALSE;
		break;

	default:
		status = IXGBE_ERR_LINK_SETUP;
		goto out;
		break;
	}

	if (hw->phy.multispeed_fiber) {
		*speed |= IXGBE_LINK_SPEED_10GB_FULL |
			  IXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = TRUE;
	}

out:
	return status;
}

/**
 *  ixgbe_get_media_type_82599 - Get media type
 *  @hw: pointer to hardware structure
 *
 *  Returns the media type (fiber, copper, backplane)
 **/
enum ixgbe_media_type ixgbe_get_media_type_82599(struct ixgbe_hw *hw)
{
	enum ixgbe_media_type media_type;

	DEBUGFUNC("ixgbe_get_media_type_82599");

	/* Detect if there is a copper PHY attached. */
	switch (hw->phy.type) {
	case ixgbe_phy_cu_unknown:
	case ixgbe_phy_tn:
		media_type = ixgbe_media_type_copper;
		goto out;
	default:
		break;
	}

	switch (hw->device_id) {
	case IXGBE_DEV_ID_82599_KX4:
	case IXGBE_DEV_ID_82599_KX4_MEZZ:
	case IXGBE_DEV_ID_82599_COMBO_BACKPLANE:
	case IXGBE_DEV_ID_82599_KR:
	case IXGBE_DEV_ID_82599_BACKPLANE_FCOE:
	case IXGBE_DEV_ID_82599_XAUI_LOM:
		/* Default device ID is mezzanine card KX/KX4 */
		media_type = ixgbe_media_type_backplane;
		break;
	case IXGBE_DEV_ID_82599_SFP:
	case IXGBE_DEV_ID_82599_SFP_FCOE:
	case IXGBE_DEV_ID_82599_SFP_EM:
	case IXGBE_DEV_ID_82599_SFP_SF2:
	case IXGBE_DEV_ID_82599_SFP_SF_QP:
	case IXGBE_DEV_ID_82599EN_SFP:
		media_type = ixgbe_media_type_fiber;
		break;
	case IXGBE_DEV_ID_82599_CX4:
		media_type = ixgbe_media_type_cx4;
		break;
	case IXGBE_DEV_ID_82599_T3_LOM:
		media_type = ixgbe_media_type_copper;
		break;
	case IXGBE_DEV_ID_82599_BYPASS:
		media_type = ixgbe_media_type_fiber_fixed;
		hw->phy.multispeed_fiber = TRUE;
		break;
	default:
		media_type = ixgbe_media_type_unknown;
		break;
	}
out:
	return media_type;
}

/**
 *  ixgbe_stop_mac_link_on_d3_82599 - Disables link on D3
 *  @hw: pointer to hardware structure
 *
 *  Disables link during D3 power down sequence.
 *
 **/
void ixgbe_stop_mac_link_on_d3_82599(struct ixgbe_hw *hw)
{
	uint32_t autoc2_reg;
	uint16_t ee_ctrl_2 = 0;

	DEBUGFUNC("ixgbe_stop_mac_link_on_d3_82599");
	ixgbe_read_eeprom_82599(hw, IXGBE_EEPROM_CTRL_2, &ee_ctrl_2);

	if (!hw->mng_fw_enabled && ee_ctrl_2 & IXGBE_EEPROM_CCD_BIT) {
		autoc2_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC2);
		autoc2_reg |= IXGBE_AUTOC2_LINK_DISABLE_ON_D3_MASK;
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC2, autoc2_reg);
	}
}

/**
 *  ixgbe_start_mac_link_82599 - Setup MAC link settings
 *  @hw: pointer to hardware structure
 *  @autoneg_wait_to_complete: TRUE when waiting for completion is needed
 *
 *  Configures link settings based on values in the ixgbe_hw struct.
 *  Restarts the link.  Performs autonegotiation if needed.
 **/
int32_t ixgbe_start_mac_link_82599(struct ixgbe_hw *hw,
				   bool autoneg_wait_to_complete)
{
	uint32_t autoc_reg;
	uint32_t links_reg;
	uint32_t i;
	int32_t status = IXGBE_SUCCESS;
	bool got_lock = FALSE;

	DEBUGFUNC("ixgbe_start_mac_link_82599");


	/*  reset_pipeline requires us to hold this lock as it writes to
	 *  AUTOC.
	 */
	if (ixgbe_verify_lesm_fw_enabled_82599(hw)) {
		status = hw->mac.ops.acquire_swfw_sync(hw,
						       IXGBE_GSSR_MAC_CSR_SM);
		if (status != IXGBE_SUCCESS)
			goto out;

		got_lock = TRUE;
	}

	/* Restart link */
	ixgbe_reset_pipeline_82599(hw);

	if (got_lock)
		hw->mac.ops.release_swfw_sync(hw, IXGBE_GSSR_MAC_CSR_SM);

	/* Only poll for autoneg to complete if specified to do so */
	if (autoneg_wait_to_complete) {
		autoc_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC);
		if ((autoc_reg & IXGBE_AUTOC_LMS_MASK) ==
		     IXGBE_AUTOC_LMS_KX4_KX_KR ||
		    (autoc_reg & IXGBE_AUTOC_LMS_MASK) ==
		     IXGBE_AUTOC_LMS_KX4_KX_KR_1G_AN ||
		    (autoc_reg & IXGBE_AUTOC_LMS_MASK) ==
		     IXGBE_AUTOC_LMS_KX4_KX_KR_SGMII) {
			links_reg = 0; /* Just in case Autoneg time = 0 */
			for (i = 0; i < IXGBE_AUTO_NEG_TIME; i++) {
				links_reg = IXGBE_READ_REG(hw, IXGBE_LINKS);
				if (links_reg & IXGBE_LINKS_KX_AN_COMP)
					break;
				msec_delay(100);
			}
			if (!(links_reg & IXGBE_LINKS_KX_AN_COMP)) {
				status = IXGBE_ERR_AUTONEG_NOT_COMPLETE;
				DEBUGOUT("Autoneg did not complete.\n");
			}
		}
	}

	/* Add delay to filter out noises during initial link setup */
	msec_delay(50);

out:
	return status;
}

/**
 *  ixgbe_disable_tx_laser_multispeed_fiber - Disable Tx laser
 *  @hw: pointer to hardware structure
 *
 *  The base drivers may require better control over SFP+ module
 *  PHY states.  This includes selectively shutting down the Tx
 *  laser on the PHY, effectively halting physical link.
 **/
void ixgbe_disable_tx_laser_multispeed_fiber(struct ixgbe_hw *hw)
{
	uint32_t esdp_reg = IXGBE_READ_REG(hw, IXGBE_ESDP);

	/* Disable tx laser; allow 100us to go dark per spec */
	esdp_reg |= IXGBE_ESDP_SDP3;
	IXGBE_WRITE_REG(hw, IXGBE_ESDP, esdp_reg);
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(100);
}

/**
 *  ixgbe_enable_tx_laser_multispeed_fiber - Enable Tx laser
 *  @hw: pointer to hardware structure
 *
 *  The base drivers may require better control over SFP+ module
 *  PHY states.  This includes selectively turning on the Tx
 *  laser on the PHY, effectively starting physical link.
 **/
void ixgbe_enable_tx_laser_multispeed_fiber(struct ixgbe_hw *hw)
{
	uint32_t esdp_reg = IXGBE_READ_REG(hw, IXGBE_ESDP);

	/* Enable tx laser; allow 100ms to light up */
	esdp_reg &= ~IXGBE_ESDP_SDP3;
	IXGBE_WRITE_REG(hw, IXGBE_ESDP, esdp_reg);
	IXGBE_WRITE_FLUSH(hw);
	msec_delay(100);
}

/**
 *  ixgbe_flap_tx_laser_multispeed_fiber - Flap Tx laser
 *  @hw: pointer to hardware structure
 *
 *  When the driver changes the link speeds that it can support,
 *  it sets autotry_restart to TRUE to indicate that we need to
 *  initiate a new autotry session with the link partner.  To do
 *  so, we set the speed then disable and re-enable the tx laser, to
 *  alert the link partner that it also needs to restart autotry on its
 *  end.  This is consistent with TRUE clause 37 autoneg, which also
 *  involves a loss of signal.
 **/
void ixgbe_flap_tx_laser_multispeed_fiber(struct ixgbe_hw *hw)
{
	DEBUGFUNC("ixgbe_flap_tx_laser_multispeed_fiber");

	if (hw->mac.autotry_restart) {
		ixgbe_disable_tx_laser_multispeed_fiber(hw);
		ixgbe_enable_tx_laser_multispeed_fiber(hw);
		hw->mac.autotry_restart = FALSE;
	}
}

/**
 *  ixgbe_set_fiber_fixed_speed - Set module link speed for fixed fiber
 *  @hw: pointer to hardware structure
 *  @speed: link speed to set
 *
 *  We set the module speed differently for fixed fiber.  For other
 *  multi-speed devices we don't have an error value so here if we
 *  detect an error we just log it and exit.
 */
void ixgbe_set_fiber_fixed_speed(struct ixgbe_hw *hw,
				 ixgbe_link_speed speed)
{
	int32_t status;
	uint8_t rs, eeprom_data;

	switch (speed) {
	case IXGBE_LINK_SPEED_10GB_FULL:
		/* one bit mask same as setting on */
		rs = IXGBE_SFF_SOFT_RS_SELECT_10G;
		break;
	case IXGBE_LINK_SPEED_1GB_FULL:
		rs = IXGBE_SFF_SOFT_RS_SELECT_1G;
		break;
	default:
		DEBUGOUT("Invalid fixed module speed\n");
		return;
	}

	/* Set RS0 */
	status = hw->phy.ops.read_i2c_byte(hw, IXGBE_SFF_SFF_8472_OSCB,
					   IXGBE_I2C_EEPROM_DEV_ADDR2,
					   &eeprom_data);
	if (status) {
		DEBUGOUT("Failed to read Rx Rate Select RS0\n");
		goto out;
	}

	eeprom_data = (eeprom_data & ~IXGBE_SFF_SOFT_RS_SELECT_MASK) & rs;

	status = hw->phy.ops.write_i2c_byte(hw, IXGBE_SFF_SFF_8472_OSCB,
					    IXGBE_I2C_EEPROM_DEV_ADDR2,
					    eeprom_data);
	if (status) {
		DEBUGOUT("Failed to write Rx Rate Select RS0\n");
		goto out;
	}

	/* Set RS1 */
	status = hw->phy.ops.read_i2c_byte(hw, IXGBE_SFF_SFF_8472_ESCB,
					   IXGBE_I2C_EEPROM_DEV_ADDR2,
					   &eeprom_data);
	if (status) {
		DEBUGOUT("Failed to read Rx Rate Select RS1\n");
		goto out;
	}

	eeprom_data = (eeprom_data & ~IXGBE_SFF_SOFT_RS_SELECT_MASK) & rs;

	status = hw->phy.ops.write_i2c_byte(hw, IXGBE_SFF_SFF_8472_ESCB,
					    IXGBE_I2C_EEPROM_DEV_ADDR2,
					    eeprom_data);
	if (status) {
		DEBUGOUT("Failed to write Rx Rate Select RS1\n");
		goto out;
	}
out:
	return;
}

/**
 *  ixgbe_setup_mac_link_multispeed_fiber - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: TRUE when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
int32_t ixgbe_setup_mac_link_multispeed_fiber(struct ixgbe_hw *hw,
				     ixgbe_link_speed speed,
				     bool autoneg_wait_to_complete)
{
	int32_t status = IXGBE_SUCCESS;
	ixgbe_link_speed link_speed = IXGBE_LINK_SPEED_UNKNOWN;
	ixgbe_link_speed highest_link_speed = IXGBE_LINK_SPEED_UNKNOWN;
	uint32_t speedcnt = 0;
	uint32_t esdp_reg = IXGBE_READ_REG(hw, IXGBE_ESDP);
	uint32_t i = 0;
	bool autoneg, link_up = FALSE;

	DEBUGFUNC("ixgbe_setup_mac_link_multispeed_fiber");

	/* Mask off requested but non-supported speeds */
	status = ixgbe_get_link_capabilities_82599(hw, &link_speed, &autoneg);
	if (status != IXGBE_SUCCESS)
		return status;

	speed &= link_speed;

	/*
	 * Try each speed one by one, highest priority first.  We do this in
	 * software because 10gb fiber doesn't support speed autonegotiation.
	 */
	if (speed & IXGBE_LINK_SPEED_10GB_FULL) {
		speedcnt++;
		highest_link_speed = IXGBE_LINK_SPEED_10GB_FULL;

		/* If we already have link at this speed, just jump out */
		status = ixgbe_check_link(hw, &link_speed, &link_up, FALSE);
		if (status != IXGBE_SUCCESS)
			return status;

		if ((link_speed == IXGBE_LINK_SPEED_10GB_FULL) && link_up)
			goto out;

		/* Set the module link speed */
		if (hw->phy.media_type == ixgbe_media_type_fiber_fixed) {
			ixgbe_set_fiber_fixed_speed(hw,
						    IXGBE_LINK_SPEED_10GB_FULL);
		} else {
			esdp_reg |= (IXGBE_ESDP_SDP5_DIR | IXGBE_ESDP_SDP5);
			IXGBE_WRITE_REG(hw, IXGBE_ESDP, esdp_reg);
			IXGBE_WRITE_FLUSH(hw);
		}

		/* Allow module to change analog characteristics (1G->10G) */
		msec_delay(40);

		status = ixgbe_setup_mac_link_82599(hw,
						    IXGBE_LINK_SPEED_10GB_FULL,
						    autoneg_wait_to_complete);
		if (status != IXGBE_SUCCESS)
			return status;

		/* Flap the tx laser if it has not already been done */
		ixgbe_flap_tx_laser(hw);

		/*
		 * Wait for the controller to acquire link.  Per IEEE 802.3ap,
		 * Section 73.10.2, we may have to wait up to 500ms if KR is
		 * attempted.  82599 uses the same timing for 10g SFI.
		 */
		for (i = 0; i < 5; i++) {
			/* Wait for the link partner to also set speed */
			msec_delay(100);

			/* If we have link, just jump out */
			status = ixgbe_check_link(hw, &link_speed,
						  &link_up, FALSE);
			if (status != IXGBE_SUCCESS)
				return status;

			if (link_up)
				goto out;
		}
	}

	if (speed & IXGBE_LINK_SPEED_1GB_FULL) {
		speedcnt++;
		if (highest_link_speed == IXGBE_LINK_SPEED_UNKNOWN)
			highest_link_speed = IXGBE_LINK_SPEED_1GB_FULL;

		/* If we already have link at this speed, just jump out */
		status = ixgbe_check_link(hw, &link_speed, &link_up, FALSE);
		if (status != IXGBE_SUCCESS)
			return status;

		if ((link_speed == IXGBE_LINK_SPEED_1GB_FULL) && link_up)
			goto out;

		/* Set the module link speed */
		if (hw->phy.media_type == ixgbe_media_type_fiber_fixed) {
			ixgbe_set_fiber_fixed_speed(hw,
						    IXGBE_LINK_SPEED_1GB_FULL);
		} else {
			esdp_reg &= ~IXGBE_ESDP_SDP5;
			esdp_reg |= IXGBE_ESDP_SDP5_DIR;
			IXGBE_WRITE_REG(hw, IXGBE_ESDP, esdp_reg);
			IXGBE_WRITE_FLUSH(hw);
		}

		/* Allow module to change analog characteristics (10G->1G) */
		msec_delay(40);

		status = ixgbe_setup_mac_link_82599(hw,
						    IXGBE_LINK_SPEED_1GB_FULL,
						    autoneg_wait_to_complete);
		if (status != IXGBE_SUCCESS)
			return status;

		/* Flap the tx laser if it has not already been done */
		ixgbe_flap_tx_laser(hw);

		/* Wait for the link partner to also set speed */
		msec_delay(100);

		/* If we have link, just jump out */
		status = ixgbe_check_link(hw, &link_speed, &link_up, FALSE);
		if (status != IXGBE_SUCCESS)
			return status;

		if (link_up)
			goto out;
	}

	/*
	 * We didn't get link.  Configure back to the highest speed we tried,
	 * (if there was more than one).  We call ourselves back with just the
	 * single highest speed that the user requested.
	 */
	if (speedcnt > 1)
		status = ixgbe_setup_mac_link_multispeed_fiber(hw,
			highest_link_speed, autoneg_wait_to_complete);

out:
	/* Set autoneg_advertised value based on input link speed */
	hw->phy.autoneg_advertised = 0;

	if (speed & IXGBE_LINK_SPEED_10GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_10GB_FULL;

	if (speed & IXGBE_LINK_SPEED_1GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_1GB_FULL;

	return status;
}

/**
 *  ixgbe_setup_mac_link_smartspeed - Set MAC link speed using SmartSpeed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: TRUE when waiting for completion is needed
 *
 *  Implements the Intel SmartSpeed algorithm.
 **/
int32_t ixgbe_setup_mac_link_smartspeed(struct ixgbe_hw *hw,
				     ixgbe_link_speed speed,
				     bool autoneg_wait_to_complete)
{
	int32_t status = IXGBE_SUCCESS;
	ixgbe_link_speed link_speed = IXGBE_LINK_SPEED_UNKNOWN;
	int32_t i, j;
	bool link_up = FALSE;
	uint32_t autoc_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC);

	DEBUGFUNC("ixgbe_setup_mac_link_smartspeed");

	 /* Set autoneg_advertised value based on input link speed */
	hw->phy.autoneg_advertised = 0;

	if (speed & IXGBE_LINK_SPEED_10GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_10GB_FULL;

	if (speed & IXGBE_LINK_SPEED_1GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_1GB_FULL;

	if (speed & IXGBE_LINK_SPEED_100_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_100_FULL;

	/*
	 * Implement Intel SmartSpeed algorithm.  SmartSpeed will reduce the
	 * autoneg advertisement if link is unable to be established at the
	 * highest negotiated rate.  This can sometimes happen due to integrity
	 * issues with the physical media connection.
	 */

	/* First, try to get link with full advertisement */
	hw->phy.smart_speed_active = FALSE;
	for (j = 0; j < IXGBE_SMARTSPEED_MAX_RETRIES; j++) {
		status = ixgbe_setup_mac_link_82599(hw, speed,
						    autoneg_wait_to_complete);
		if (status != IXGBE_SUCCESS)
			goto out;

		/*
		 * Wait for the controller to acquire link.  Per IEEE 802.3ap,
		 * Section 73.10.2, we may have to wait up to 500ms if KR is
		 * attempted, or 200ms if KX/KX4/BX/BX4 is attempted, per
		 * Table 9 in the AN MAS.
		 */
		for (i = 0; i < 5; i++) {
			msec_delay(100);

			/* If we have link, just jump out */
			status = ixgbe_check_link(hw, &link_speed, &link_up,
						  FALSE);
			if (status != IXGBE_SUCCESS)
				goto out;

			if (link_up)
				goto out;
		}
	}

	/*
	 * We didn't get link.  If we advertised KR plus one of KX4/KX
	 * (or BX4/BX), then disable KR and try again.
	 */
	if (((autoc_reg & IXGBE_AUTOC_KR_SUPP) == 0) ||
	    ((autoc_reg & IXGBE_AUTOC_KX4_KX_SUPP_MASK) == 0))
		goto out;

	/* Turn SmartSpeed on to disable KR support */
	hw->phy.smart_speed_active = TRUE;
	status = ixgbe_setup_mac_link_82599(hw, speed,
					    autoneg_wait_to_complete);
	if (status != IXGBE_SUCCESS)
		goto out;

	/*
	 * Wait for the controller to acquire link.  600ms will allow for
	 * the AN link_fail_inhibit_timer as well for multiple cycles of
	 * parallel detect, both 10g and 1g. This allows for the maximum
	 * connect attempts as defined in the AN MAS table 73-7.
	 */
	for (i = 0; i < 6; i++) {
		msec_delay(100);

		/* If we have link, just jump out */
		status = ixgbe_check_link(hw, &link_speed, &link_up, FALSE);
		if (status != IXGBE_SUCCESS)
			goto out;

		if (link_up)
			goto out;
	}

	/* We didn't get link.  Turn SmartSpeed back off. */
	hw->phy.smart_speed_active = FALSE;
	status = ixgbe_setup_mac_link_82599(hw, speed,
					    autoneg_wait_to_complete);

out:
	if (link_up && (link_speed == IXGBE_LINK_SPEED_1GB_FULL))
		DEBUGOUT("Smartspeed has downgraded the link speed "
		"from the maximum advertised\n");
	return status;
}

/**
 *  ixgbe_setup_mac_link_82599 - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: TRUE when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
int32_t ixgbe_setup_mac_link_82599(struct ixgbe_hw *hw,
				   ixgbe_link_speed speed,
				   bool autoneg_wait_to_complete)
{
	bool autoneg = FALSE;
	int32_t status = IXGBE_SUCCESS;
	uint32_t autoc, pma_pmd_1g, link_mode, start_autoc;
	uint32_t autoc2 = IXGBE_READ_REG(hw, IXGBE_AUTOC2);
	uint32_t orig_autoc = 0;
	uint32_t pma_pmd_10g_serial = autoc2 & IXGBE_AUTOC2_10G_SERIAL_PMA_PMD_MASK;
	uint32_t links_reg;
	uint32_t i;
	ixgbe_link_speed link_capabilities = IXGBE_LINK_SPEED_UNKNOWN;
	bool got_lock = FALSE;

	DEBUGFUNC("ixgbe_setup_mac_link_82599");

	/* Check to see if speed passed in is supported. */
	status = ixgbe_get_link_capabilities_82599(hw, &link_capabilities, &autoneg);
	if (status)
		goto out;

	speed &= link_capabilities;

	if (speed == IXGBE_LINK_SPEED_UNKNOWN) {
		status = IXGBE_ERR_LINK_SETUP;
		goto out;
	}

	/* Use stored value (EEPROM defaults) of AUTOC to find KR/KX4 support*/
	if (hw->mac.orig_link_settings_stored)
		autoc = hw->mac.orig_autoc;
	else
		autoc = IXGBE_READ_REG(hw, IXGBE_AUTOC);

	orig_autoc = autoc;
	start_autoc = hw->mac.cached_autoc;
	link_mode = autoc & IXGBE_AUTOC_LMS_MASK;
	pma_pmd_1g = autoc & IXGBE_AUTOC_1G_PMA_PMD_MASK;

	if (link_mode == IXGBE_AUTOC_LMS_KX4_KX_KR ||
	    link_mode == IXGBE_AUTOC_LMS_KX4_KX_KR_1G_AN ||
	    link_mode == IXGBE_AUTOC_LMS_KX4_KX_KR_SGMII) {
		/* Set KX4/KX/KR support according to speed requested */
		autoc &= ~(IXGBE_AUTOC_KX4_KX_SUPP_MASK | IXGBE_AUTOC_KR_SUPP);
		if (speed & IXGBE_LINK_SPEED_10GB_FULL) {
			if (orig_autoc & IXGBE_AUTOC_KX4_SUPP)
				autoc |= IXGBE_AUTOC_KX4_SUPP;
			if ((orig_autoc & IXGBE_AUTOC_KR_SUPP) &&
			    (hw->phy.smart_speed_active == FALSE))
				autoc |= IXGBE_AUTOC_KR_SUPP;
		}
		if (speed & IXGBE_LINK_SPEED_1GB_FULL)
			autoc |= IXGBE_AUTOC_KX_SUPP;
	} else if ((pma_pmd_1g == IXGBE_AUTOC_1G_SFI) &&
		   (link_mode == IXGBE_AUTOC_LMS_1G_LINK_NO_AN ||
		    link_mode == IXGBE_AUTOC_LMS_1G_AN)) {
		/* Switch from 1G SFI to 10G SFI if requested */
		if ((speed == IXGBE_LINK_SPEED_10GB_FULL) &&
		    (pma_pmd_10g_serial == IXGBE_AUTOC2_10G_SFI)) {
			autoc &= ~IXGBE_AUTOC_LMS_MASK;
			autoc |= IXGBE_AUTOC_LMS_10G_SERIAL;
		}
	} else if ((pma_pmd_10g_serial == IXGBE_AUTOC2_10G_SFI) &&
		   (link_mode == IXGBE_AUTOC_LMS_10G_SERIAL)) {
		/* Switch from 10G SFI to 1G SFI if requested */
		if ((speed == IXGBE_LINK_SPEED_1GB_FULL) &&
		    (pma_pmd_1g == IXGBE_AUTOC_1G_SFI)) {
			autoc &= ~IXGBE_AUTOC_LMS_MASK;
			if (autoneg)
				autoc |= IXGBE_AUTOC_LMS_1G_AN;
			else
				autoc |= IXGBE_AUTOC_LMS_1G_LINK_NO_AN;
		}
	}

	if (autoc != start_autoc) {
		/* Need SW/FW semaphore around AUTOC writes if LESM is on,
		 * likewise reset_pipeline requires us to hold this lock as
		 * it also writes to AUTOC.
		 */
		if (ixgbe_verify_lesm_fw_enabled_82599(hw)) {
			status = hw->mac.ops.acquire_swfw_sync(hw,
							IXGBE_GSSR_MAC_CSR_SM);
			if (status != IXGBE_SUCCESS) {
				status = IXGBE_ERR_SWFW_SYNC;
				goto out;
			}

			got_lock = TRUE;
		}

		/* Restart link */
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC, autoc);
		hw->mac.cached_autoc = autoc;
		ixgbe_reset_pipeline_82599(hw);

		if (got_lock) {
			hw->mac.ops.release_swfw_sync(hw,
						      IXGBE_GSSR_MAC_CSR_SM);
			got_lock = FALSE;
		}

		/* Only poll for autoneg to complete if specified to do so */
		if (autoneg_wait_to_complete) {
			if (link_mode == IXGBE_AUTOC_LMS_KX4_KX_KR ||
			    link_mode == IXGBE_AUTOC_LMS_KX4_KX_KR_1G_AN ||
			    link_mode == IXGBE_AUTOC_LMS_KX4_KX_KR_SGMII) {
				links_reg = 0; /*Just in case Autoneg time=0*/
				for (i = 0; i < IXGBE_AUTO_NEG_TIME; i++) {
					links_reg =
					       IXGBE_READ_REG(hw, IXGBE_LINKS);
					if (links_reg & IXGBE_LINKS_KX_AN_COMP)
						break;
					msec_delay(100);
				}
				if (!(links_reg & IXGBE_LINKS_KX_AN_COMP)) {
					status =
						IXGBE_ERR_AUTONEG_NOT_COMPLETE;
					DEBUGOUT("Autoneg did not complete.\n");
				}
			}
		}

		/* Add delay to filter out noises during initial link setup */
		msec_delay(50);
	}

out:
	return status;
}

/**
 *  ixgbe_setup_copper_link_82599 - Set the PHY autoneg advertised field
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: TRUE if waiting is needed to complete
 *
 *  Restarts link on PHY and MAC based on settings passed in.
 **/
int32_t ixgbe_setup_copper_link_82599(struct ixgbe_hw *hw,
				      ixgbe_link_speed speed,
				      bool autoneg_wait_to_complete)
{
	int32_t status;

	DEBUGFUNC("ixgbe_setup_copper_link_82599");

	/* Setup the PHY according to input speed */
	status = hw->phy.ops.setup_link_speed(hw, speed,
					      autoneg_wait_to_complete);
	/* Set up MAC */
	ixgbe_start_mac_link_82599(hw, autoneg_wait_to_complete);

	return status;
}

/**
 *  ixgbe_reset_hw_82599 - Perform hardware reset
 *  @hw: pointer to hardware structure
 *
 *  Resets the hardware by resetting the transmit and receive units, masks
 *  and clears all interrupts, perform a PHY reset, and perform a link (MAC)
 *  reset.
 **/
int32_t ixgbe_reset_hw_82599(struct ixgbe_hw *hw)
{
	ixgbe_link_speed link_speed;
	int32_t status;
	uint32_t ctrl, i, autoc2;
	uint32_t curr_lms;
	bool link_up = FALSE;

	DEBUGFUNC("ixgbe_reset_hw_82599");

	/* Call adapter stop to disable tx/rx and clear interrupts */
	status = hw->mac.ops.stop_adapter(hw);
	if (status != IXGBE_SUCCESS)
		goto reset_hw_out;

	/* flush pending Tx transactions */
	ixgbe_clear_tx_pending(hw);

	/* PHY ops must be identified and initialized prior to reset */

	/* Identify PHY and related function pointers */
	status = hw->phy.ops.init(hw);

	if (status == IXGBE_ERR_SFP_NOT_SUPPORTED)
		goto reset_hw_out;

	/* Setup SFP module if there is one present. */
	if (hw->phy.sfp_setup_needed) {
		status = hw->mac.ops.setup_sfp(hw);
		hw->phy.sfp_setup_needed = FALSE;
	}

	if (status == IXGBE_ERR_SFP_NOT_SUPPORTED)
		goto reset_hw_out;

	/* Reset PHY */
	if (hw->phy.reset_disable == FALSE && hw->phy.ops.reset != NULL)
		hw->phy.ops.reset(hw);

	/* remember AUTOC from before we reset */
	if (hw->mac.cached_autoc)
		curr_lms = hw->mac.cached_autoc & IXGBE_AUTOC_LMS_MASK;
	else
		curr_lms = IXGBE_READ_REG(hw, IXGBE_AUTOC) &
					  IXGBE_AUTOC_LMS_MASK;

mac_reset_top:
	/*
	 * Issue global reset to the MAC.  Needs to be SW reset if link is up.
	 * If link reset is used when link is up, it might reset the PHY when
	 * mng is using it.  If link is down or the flag to force full link
	 * reset is set, then perform link reset.
	 */
	ctrl = IXGBE_CTRL_LNK_RST;
	if (!hw->force_full_reset) {
		hw->mac.ops.check_link(hw, &link_speed, &link_up, FALSE);
		if (link_up)
			ctrl = IXGBE_CTRL_RST;
	}

	ctrl |= IXGBE_READ_REG(hw, IXGBE_CTRL);
	IXGBE_WRITE_REG(hw, IXGBE_CTRL, ctrl);
	IXGBE_WRITE_FLUSH(hw);

	/* Poll for reset bit to self-clear indicating reset is complete */
	for (i = 0; i < 10; i++) {
		usec_delay(1);
		ctrl = IXGBE_READ_REG(hw, IXGBE_CTRL);
		if (!(ctrl & IXGBE_CTRL_RST_MASK))
			break;
	}

	if (ctrl & IXGBE_CTRL_RST_MASK) {
		status = IXGBE_ERR_RESET_FAILED;
		DEBUGOUT("Reset polling failed to complete.\n");
	}

	msec_delay(50);

	/*
	 * Double resets are required for recovery from certain error
	 * conditions.  Between resets, it is necessary to stall to allow time
	 * for any pending HW events to complete.
	 */
	if (hw->mac.flags & IXGBE_FLAGS_DOUBLE_RESET_REQUIRED) {
		hw->mac.flags &= ~IXGBE_FLAGS_DOUBLE_RESET_REQUIRED;
		goto mac_reset_top;
	}

	/*
	 * Store the original AUTOC/AUTOC2 values if they have not been
	 * stored off yet.  Otherwise restore the stored original
	 * values since the reset operation sets back to defaults.
	 */
	hw->mac.cached_autoc = IXGBE_READ_REG(hw, IXGBE_AUTOC);
	autoc2 = IXGBE_READ_REG(hw, IXGBE_AUTOC2);

	/* Enable link if disabled in NVM */
	if (autoc2 & IXGBE_AUTOC2_LINK_DISABLE_MASK) {
		autoc2 &= ~IXGBE_AUTOC2_LINK_DISABLE_MASK;
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC2, autoc2);
		IXGBE_WRITE_FLUSH(hw);
	}

	if (hw->mac.orig_link_settings_stored == FALSE) {
		hw->mac.orig_autoc = hw->mac.cached_autoc;
		hw->mac.orig_autoc2 = autoc2;
		hw->mac.orig_link_settings_stored = TRUE;
	} else {

		/* If MNG FW is running on a multi-speed device that
		 * doesn't autoneg with out driver support we need to
		 * leave LMS in the state it was before we MAC reset.
		 * Likewise if we support WoL we don't want change the
		 * LMS state.
		 */
		if (hw->phy.multispeed_fiber && hw->mng_fw_enabled)
			hw->mac.orig_autoc =
				(hw->mac.orig_autoc & ~IXGBE_AUTOC_LMS_MASK) |
				curr_lms;

		if (hw->mac.cached_autoc != hw->mac.orig_autoc) {
			/* Need SW/FW semaphore around AUTOC writes if LESM is
			 * on, likewise reset_pipeline requires us to hold
			 * this lock as it also writes to AUTOC.
			 */
			bool got_lock = FALSE;
			if (ixgbe_verify_lesm_fw_enabled_82599(hw)) {
				status = hw->mac.ops.acquire_swfw_sync(hw,
							IXGBE_GSSR_MAC_CSR_SM);
				if (status != IXGBE_SUCCESS) {
					status = IXGBE_ERR_SWFW_SYNC;
					goto reset_hw_out;
				}

				got_lock = TRUE;
			}

			IXGBE_WRITE_REG(hw, IXGBE_AUTOC, hw->mac.orig_autoc);
			hw->mac.cached_autoc = hw->mac.orig_autoc;
			ixgbe_reset_pipeline_82599(hw);

			if (got_lock)
				hw->mac.ops.release_swfw_sync(hw,
						      IXGBE_GSSR_MAC_CSR_SM);
		}

		if ((autoc2 & IXGBE_AUTOC2_UPPER_MASK) !=
		    (hw->mac.orig_autoc2 & IXGBE_AUTOC2_UPPER_MASK)) {
			autoc2 &= ~IXGBE_AUTOC2_UPPER_MASK;
			autoc2 |= (hw->mac.orig_autoc2 &
				   IXGBE_AUTOC2_UPPER_MASK);
			IXGBE_WRITE_REG(hw, IXGBE_AUTOC2, autoc2);
		}
	}

	/* Store the permanent mac address */
	hw->mac.ops.get_mac_addr(hw, hw->mac.perm_addr);

	/*
	 * Store MAC address from RAR0, clear receive address registers, and
	 * clear the multicast table.  Also reset num_rar_entries to 128,
	 * since we modify this value when programming the SAN MAC address.
	 */
	hw->mac.num_rar_entries = 128;
	hw->mac.ops.init_rx_addrs(hw);

reset_hw_out:
	return status;
}

/**
 *  ixgbe_read_analog_reg8_82599 - Reads 8 bit Omer analog register
 *  @hw: pointer to hardware structure
 *  @reg: analog register to read
 *  @val: read value
 *
 *  Performs read operation to Omer analog register specified.
 **/
int32_t ixgbe_read_analog_reg8_82599(struct ixgbe_hw *hw, uint32_t reg,
				     uint8_t *val)
{
	uint32_t  core_ctl;

	DEBUGFUNC("ixgbe_read_analog_reg8_82599");

	IXGBE_WRITE_REG(hw, IXGBE_CORECTL, IXGBE_CORECTL_WRITE_CMD |
			(reg << 8));
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(10);
	core_ctl = IXGBE_READ_REG(hw, IXGBE_CORECTL);
	*val = (uint8_t)core_ctl;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_write_analog_reg8_82599 - Writes 8 bit Omer analog register
 *  @hw: pointer to hardware structure
 *  @reg: atlas register to write
 *  @val: value to write
 *
 *  Performs write operation to Omer analog register specified.
 **/
int32_t ixgbe_write_analog_reg8_82599(struct ixgbe_hw *hw, uint32_t reg,
				      uint8_t val)
{
	uint32_t  core_ctl;

	DEBUGFUNC("ixgbe_write_analog_reg8_82599");

	core_ctl = (reg << 8) | val;
	IXGBE_WRITE_REG(hw, IXGBE_CORECTL, core_ctl);
	IXGBE_WRITE_FLUSH(hw);
	usec_delay(10);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_start_hw_82599 - Prepare hardware for Tx/Rx
 *  @hw: pointer to hardware structure
 *
 *  Starts the hardware using the generic start_hw function
 *  and the generation start_hw function.
 *  Then performs revision-specific operations, if any.
 **/
int32_t ixgbe_start_hw_82599(struct ixgbe_hw *hw)
{
	int32_t ret_val = IXGBE_SUCCESS;

	DEBUGFUNC("ixgbe_start_hw_82599");

	ret_val = ixgbe_start_hw_generic(hw);
	if (ret_val != IXGBE_SUCCESS)
		goto out;

	ret_val = ixgbe_start_hw_gen2(hw);
	if (ret_val != IXGBE_SUCCESS)
		goto out;

	/* We need to run link autotry after the driver loads */
	hw->mac.autotry_restart = TRUE;

	if (ret_val == IXGBE_SUCCESS)
		ret_val = ixgbe_verify_fw_version_82599(hw);
out:
	return ret_val;
}

/**
 *  ixgbe_identify_phy_82599 - Get physical layer module
 *  @hw: pointer to hardware structure
 *
 *  Determines the physical layer module found on the current adapter.
 *  If PHY already detected, maintains current PHY type in hw struct,
 *  otherwise executes the PHY detection routine.
 **/
int32_t ixgbe_identify_phy_82599(struct ixgbe_hw *hw)
{
	int32_t status = IXGBE_ERR_PHY_ADDR_INVALID;

	DEBUGFUNC("ixgbe_identify_phy_82599");

	/* Detect PHY if not unknown - returns success if already detected. */
	status = ixgbe_identify_phy_generic(hw);
	if (status != IXGBE_SUCCESS) {
		/* 82599 10GBASE-T requires an external PHY */
		if (hw->mac.ops.get_media_type(hw) == ixgbe_media_type_copper)
			goto out;
		else
			status = ixgbe_identify_module_generic(hw);
	}

	/* Set PHY type none if no PHY detected */
	if (hw->phy.type == ixgbe_phy_unknown) {
		hw->phy.type = ixgbe_phy_none;
		status = IXGBE_SUCCESS;
	}

	/* Return error if SFP module has been detected but is not supported */
	if (hw->phy.type == ixgbe_phy_sfp_unsupported)
		status = IXGBE_ERR_SFP_NOT_SUPPORTED;

out:
	return status;
}

/**
 *  ixgbe_get_supported_physical_layer_82599 - Returns physical layer type
 *  @hw: pointer to hardware structure
 *
 *  Determines physical layer capabilities of the current configuration.
 **/
uint32_t ixgbe_get_supported_physical_layer_82599(struct ixgbe_hw *hw)
{
	uint32_t physical_layer = IXGBE_PHYSICAL_LAYER_UNKNOWN;
	uint32_t autoc = IXGBE_READ_REG(hw, IXGBE_AUTOC);
	uint32_t autoc2 = IXGBE_READ_REG(hw, IXGBE_AUTOC2);
	uint32_t pma_pmd_10g_serial = autoc2 & IXGBE_AUTOC2_10G_SERIAL_PMA_PMD_MASK;
	uint32_t pma_pmd_10g_parallel = autoc & IXGBE_AUTOC_10G_PMA_PMD_MASK;
	uint32_t pma_pmd_1g = autoc & IXGBE_AUTOC_1G_PMA_PMD_MASK;
	uint16_t ext_ability = 0;
	uint8_t comp_codes_10g = 0;
	uint8_t comp_codes_1g = 0;

	DEBUGFUNC("ixgbe_get_support_physical_layer_82599");

	hw->phy.ops.identify(hw);

	switch (hw->phy.type) {
	case ixgbe_phy_tn:
	case ixgbe_phy_cu_unknown:
		hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_EXT_ABILITY,
		IXGBE_MDIO_PMA_PMD_DEV_TYPE, &ext_ability);
		if (ext_ability & IXGBE_MDIO_PHY_10GBASET_ABILITY)
			physical_layer |= IXGBE_PHYSICAL_LAYER_10GBASE_T;
		if (ext_ability & IXGBE_MDIO_PHY_1000BASET_ABILITY)
			physical_layer |= IXGBE_PHYSICAL_LAYER_1000BASE_T;
		if (ext_ability & IXGBE_MDIO_PHY_100BASETX_ABILITY)
			physical_layer |= IXGBE_PHYSICAL_LAYER_100BASE_TX;
		goto out;
	default:
		break;
	}

	switch (autoc & IXGBE_AUTOC_LMS_MASK) {
	case IXGBE_AUTOC_LMS_1G_AN:
	case IXGBE_AUTOC_LMS_1G_LINK_NO_AN:
		if (pma_pmd_1g == IXGBE_AUTOC_1G_KX_BX) {
			physical_layer = IXGBE_PHYSICAL_LAYER_1000BASE_KX |
			    IXGBE_PHYSICAL_LAYER_1000BASE_BX;
			goto out;
		} else
			/* SFI mode so read SFP module */
			goto sfp_check;
		break;
	case IXGBE_AUTOC_LMS_10G_LINK_NO_AN:
		if (pma_pmd_10g_parallel == IXGBE_AUTOC_10G_CX4)
			physical_layer = IXGBE_PHYSICAL_LAYER_10GBASE_CX4;
		else if (pma_pmd_10g_parallel == IXGBE_AUTOC_10G_KX4)
			physical_layer = IXGBE_PHYSICAL_LAYER_10GBASE_KX4;
		else if (pma_pmd_10g_parallel == IXGBE_AUTOC_10G_XAUI)
			physical_layer = IXGBE_PHYSICAL_LAYER_10GBASE_XAUI;
		goto out;
		break;
	case IXGBE_AUTOC_LMS_10G_SERIAL:
		if (pma_pmd_10g_serial == IXGBE_AUTOC2_10G_KR) {
			physical_layer = IXGBE_PHYSICAL_LAYER_10GBASE_KR;
			goto out;
		} else if (pma_pmd_10g_serial == IXGBE_AUTOC2_10G_SFI)
			goto sfp_check;
		break;
	case IXGBE_AUTOC_LMS_KX4_KX_KR:
	case IXGBE_AUTOC_LMS_KX4_KX_KR_1G_AN:
		if (autoc & IXGBE_AUTOC_KX_SUPP)
			physical_layer |= IXGBE_PHYSICAL_LAYER_1000BASE_KX;
		if (autoc & IXGBE_AUTOC_KX4_SUPP)
			physical_layer |= IXGBE_PHYSICAL_LAYER_10GBASE_KX4;
		if (autoc & IXGBE_AUTOC_KR_SUPP)
			physical_layer |= IXGBE_PHYSICAL_LAYER_10GBASE_KR;
		goto out;
		break;
	default:
		goto out;
		break;
	}

sfp_check:
	/* SFP check must be done last since DA modules are sometimes used to
	 * test KR mode -  we need to id KR mode correctly before SFP module.
	 * Call identify_sfp because the pluggable module may have changed */
	hw->phy.ops.identify_sfp(hw);
	if (hw->phy.sfp_type == ixgbe_sfp_type_not_present)
		goto out;

	switch (hw->phy.type) {
	case ixgbe_phy_sfp_passive_tyco:
	case ixgbe_phy_sfp_passive_unknown:
		physical_layer = IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU;
		break;
	case ixgbe_phy_sfp_ftl_active:
	case ixgbe_phy_sfp_active_unknown:
		physical_layer = IXGBE_PHYSICAL_LAYER_SFP_ACTIVE_DA;
		break;
	case ixgbe_phy_sfp_avago:
	case ixgbe_phy_sfp_ftl:
	case ixgbe_phy_sfp_intel:
	case ixgbe_phy_sfp_unknown:
		hw->phy.ops.read_i2c_eeprom(hw,
		      IXGBE_SFF_1GBE_COMP_CODES, &comp_codes_1g);
		hw->phy.ops.read_i2c_eeprom(hw,
		      IXGBE_SFF_10GBE_COMP_CODES, &comp_codes_10g);
		if (comp_codes_10g & IXGBE_SFF_10GBASESR_CAPABLE)
			physical_layer = IXGBE_PHYSICAL_LAYER_10GBASE_SR;
		else if (comp_codes_10g & IXGBE_SFF_10GBASELR_CAPABLE)
			physical_layer = IXGBE_PHYSICAL_LAYER_10GBASE_LR;
		else if (comp_codes_10g &
		    (IXGBE_SFF_DA_PASSIVE_CABLE | IXGBE_SFF_DA_BAD_HP_CABLE))
			physical_layer = IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU;
		else if (comp_codes_10g & IXGBE_SFF_DA_ACTIVE_CABLE)
			physical_layer = IXGBE_PHYSICAL_LAYER_SFP_ACTIVE_DA;
		else if (comp_codes_1g & IXGBE_SFF_1GBASET_CAPABLE)
			physical_layer = IXGBE_PHYSICAL_LAYER_1000BASE_T;
		else if (comp_codes_1g & IXGBE_SFF_1GBASESX_CAPABLE)
			physical_layer = IXGBE_PHYSICAL_LAYER_1000BASE_SX;
		else if (comp_codes_1g & IXGBE_SFF_1GBASELX_CAPABLE)
			physical_layer = IXGBE_PHYSICAL_LAYER_1000BASE_LX;
		break;
	default:
		break;
	}

out:
	return physical_layer;
}

/**
 *  ixgbe_enable_rx_dma_82599 - Enable the Rx DMA unit on 82599
 *  @hw: pointer to hardware structure
 *  @regval: register value to write to RXCTRL
 *
 *  Enables the Rx DMA unit for 82599
 **/
int32_t ixgbe_enable_rx_dma_82599(struct ixgbe_hw *hw, uint32_t regval)
{

	DEBUGFUNC("ixgbe_enable_rx_dma_82599");

	/*
	 * Workaround for 82599 silicon errata when enabling the Rx datapath.
	 * If traffic is incoming before we enable the Rx unit, it could hang
	 * the Rx DMA unit.  Therefore, make sure the security engine is
	 * completely disabled prior to enabling the Rx unit.
	 */

	hw->mac.ops.disable_sec_rx_path(hw);

	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, regval);

	hw->mac.ops.enable_sec_rx_path(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_verify_fw_version_82599 - verify fw version for 82599
 *  @hw: pointer to hardware structure
 *
 *  Verifies that installed the firmware version is 0.6 or higher
 *  for SFI devices. All 82599 SFI devices should have version 0.6 or higher.
 *
 *  Returns IXGBE_ERR_EEPROM_VERSION if the FW is not present or
 *  if the FW version is not supported.
 **/
int32_t ixgbe_verify_fw_version_82599(struct ixgbe_hw *hw)
{
	int32_t status = IXGBE_ERR_EEPROM_VERSION;
	uint16_t fw_offset, fw_ptp_cfg_offset;
	uint16_t fw_version;

	DEBUGFUNC("ixgbe_verify_fw_version_82599");

	/* firmware check is only necessary for SFI devices */
	if (hw->phy.media_type != ixgbe_media_type_fiber) {
		status = IXGBE_SUCCESS;
		goto fw_version_out;
	}

	/* get the offset to the Firmware Module block */
	if (hw->eeprom.ops.read(hw, IXGBE_FW_PTR, &fw_offset)) {
		ERROR_REPORT2(IXGBE_ERROR_INVALID_STATE,
			      "eeprom read at offset %d failed", IXGBE_FW_PTR);
		return IXGBE_ERR_EEPROM_VERSION;
	}

	if ((fw_offset == 0) || (fw_offset == 0xFFFF))
		goto fw_version_out;

	/* get the offset to the Pass Through Patch Configuration block */
	if (hw->eeprom.ops.read(hw, (fw_offset +
				 IXGBE_FW_PASSTHROUGH_PATCH_CONFIG_PTR),
				 &fw_ptp_cfg_offset)) {
		ERROR_REPORT2(IXGBE_ERROR_INVALID_STATE,
			      "eeprom read at offset %d failed",
			      fw_offset +
			      IXGBE_FW_PASSTHROUGH_PATCH_CONFIG_PTR);
		return IXGBE_ERR_EEPROM_VERSION;
	}

	if ((fw_ptp_cfg_offset == 0) || (fw_ptp_cfg_offset == 0xFFFF))
		goto fw_version_out;

	/* get the firmware version */
	if (hw->eeprom.ops.read(hw, (fw_ptp_cfg_offset +
			    IXGBE_FW_PATCH_VERSION_4), &fw_version)) {
		ERROR_REPORT2(IXGBE_ERROR_INVALID_STATE,
			      "eeprom read at offset %d failed",
			      fw_ptp_cfg_offset + IXGBE_FW_PATCH_VERSION_4);
		return IXGBE_ERR_EEPROM_VERSION;
	}

	if (fw_version > 0x5)
		status = IXGBE_SUCCESS;

fw_version_out:
	return status;
}

/**
 *  ixgbe_verify_lesm_fw_enabled_82599 - Checks LESM FW module state.
 *  @hw: pointer to hardware structure
 *
 *  Returns TRUE if the LESM FW module is present and enabled. Otherwise
 *  returns FALSE. Smart Speed must be disabled if LESM FW module is enabled.
 **/
bool ixgbe_verify_lesm_fw_enabled_82599(struct ixgbe_hw *hw)
{
	bool lesm_enabled = FALSE;
	uint16_t fw_offset, fw_lesm_param_offset, fw_lesm_state;
	int32_t status;

	DEBUGFUNC("ixgbe_verify_lesm_fw_enabled_82599");

	/* get the offset to the Firmware Module block */
	status = hw->eeprom.ops.read(hw, IXGBE_FW_PTR, &fw_offset);

	if ((status != IXGBE_SUCCESS) ||
	    (fw_offset == 0) || (fw_offset == 0xFFFF))
		goto out;

	/* get the offset to the LESM Parameters block */
	status = hw->eeprom.ops.read(hw, (fw_offset +
				     IXGBE_FW_LESM_PARAMETERS_PTR),
				     &fw_lesm_param_offset);

	if ((status != IXGBE_SUCCESS) ||
	    (fw_lesm_param_offset == 0) || (fw_lesm_param_offset == 0xFFFF))
		goto out;

	/* get the lesm state word */
	status = hw->eeprom.ops.read(hw, (fw_lesm_param_offset +
				     IXGBE_FW_LESM_STATE_1),
				     &fw_lesm_state);

	if ((status == IXGBE_SUCCESS) &&
	    (fw_lesm_state & IXGBE_FW_LESM_STATE_ENABLED))
		lesm_enabled = TRUE;

out:
	return lesm_enabled;
}

/**
 *  ixgbe_read_eeprom_82599 - Read EEPROM word using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 16 bit word from the EEPROM
 **/
int32_t ixgbe_read_eeprom_82599(struct ixgbe_hw *hw,
				uint16_t offset, uint16_t *data)
{
	struct ixgbe_eeprom_info *eeprom = &hw->eeprom;
	int32_t ret_val = IXGBE_ERR_CONFIG;

	DEBUGFUNC("ixgbe_read_eeprom_82599");

	/*
	 * If EEPROM is detected and can be addressed using 14 bits,
	 * use EERD otherwise use bit bang
	 */
	if ((eeprom->type == ixgbe_eeprom_spi) &&
	    (offset <= IXGBE_EERD_MAX_ADDR))
		ret_val = ixgbe_read_eerd_generic(hw, offset, data);
	else
		ret_val = ixgbe_read_eeprom_bit_bang_generic(hw, offset, data);

	return ret_val;
}

/**
 * ixgbe_reset_pipeline_82599 - perform pipeline reset
 *
 *  @hw: pointer to hardware structure
 *
 * Reset pipeline by asserting Restart_AN together with LMS change to ensure
 * full pipeline reset
 **/
int32_t ixgbe_reset_pipeline_82599(struct ixgbe_hw *hw)
{
	int32_t ret_val;
	uint32_t anlp1_reg = 0;
	uint32_t i, autoc_reg, autoc2_reg;

	/* Enable link if disabled in NVM */
	autoc2_reg = IXGBE_READ_REG(hw, IXGBE_AUTOC2);
	if (autoc2_reg & IXGBE_AUTOC2_LINK_DISABLE_MASK) {
		autoc2_reg &= ~IXGBE_AUTOC2_LINK_DISABLE_MASK;
		IXGBE_WRITE_REG(hw, IXGBE_AUTOC2, autoc2_reg);
		IXGBE_WRITE_FLUSH(hw);
	}

	autoc_reg = hw->mac.cached_autoc;
	autoc_reg |= IXGBE_AUTOC_AN_RESTART;
	/* Write AUTOC register with toggled LMS[2] bit and Restart_AN */
	IXGBE_WRITE_REG(hw, IXGBE_AUTOC, autoc_reg ^ IXGBE_AUTOC_LMS_1G_AN);
	/* Wait for AN to leave state 0 */
	for (i = 0; i < 10; i++) {
		msec_delay(4);
		anlp1_reg = IXGBE_READ_REG(hw, IXGBE_ANLP1);
		if (anlp1_reg & IXGBE_ANLP1_AN_STATE_MASK)
			break;
	}

	if (!(anlp1_reg & IXGBE_ANLP1_AN_STATE_MASK)) {
		DEBUGOUT("auto negotiation not completed\n");
		ret_val = IXGBE_ERR_RESET_FAILED;
		goto reset_pipeline_out;
	}

	ret_val = IXGBE_SUCCESS;

reset_pipeline_out:
	/* Write AUTOC register with original LMS field and Restart_AN */
	IXGBE_WRITE_REG(hw, IXGBE_AUTOC, autoc_reg);
	IXGBE_WRITE_FLUSH(hw);

	return ret_val;
}
