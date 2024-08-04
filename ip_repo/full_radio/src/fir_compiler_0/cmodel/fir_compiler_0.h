
//------------------------------------------------------------------------------
// (c) Copyright 2014 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
//------------------------------------------------------------------------------ 
//
// C Model configuration for the "fir_compiler_0" instance.
//
//------------------------------------------------------------------------------
//
// coefficients: -67,-33,-40,-49,-59,-69,-81,-94,-107,-122,-137,-153,-170,-187,-205,-223,-241,-258,-275,-292,-307,-320,-332,-341,-348,-351,-351,-346,-336,-321,-300,-272,-237,-194,-142,-81,-10,71,164,269,386,517,661,819,992,1180,1384,1603,1839,2091,2360,2645,2947,3266,3601,3953,4320,4703,5101,5513,5939,6378,6829,7291,7763,8245,8734,9230,9731,10236,10744,11252,11760,12266,12767,13263,13752,14233,14702,15160,15603,16031,16443,16835,17208,17559,17888,18192,18472,18725,18950,19147,19316,19454,19563,19641,19687,19703,19687,19641,19563,19454,19316,19147,18950,18725,18472,18192,17888,17559,17208,16835,16443,16031,15603,15160,14702,14233,13752,13263,12767,12266,11760,11252,10744,10236,9731,9230,8734,8245,7763,7291,6829,6378,5939,5513,5101,4703,4320,3953,3601,3266,2947,2645,2360,2091,1839,1603,1384,1180,992,819,661,517,386,269,164,71,-10,-81,-142,-194,-237,-272,-300,-321,-336,-346,-351,-351,-348,-341,-332,-320,-307,-292,-275,-258,-241,-223,-205,-187,-170,-153,-137,-122,-107,-94,-81,-69,-59,-49,-40,-33,-67
// chanpats: 173
// name: fir_compiler_0
// filter_type: 2
// rate_change: 0
// interp_rate: 1
// decim_rate: 41
// zero_pack_factor: 1
// coeff_padding: 25
// num_coeffs: 195
// coeff_sets: 1
// reloadable: 0
// is_halfband: 0
// quantization: 2
// coeff_width: 16
// coeff_fract_width: 0
// chan_seq: 0
// num_channels: 1
// num_paths: 2
// data_width: 16
// data_fract_width: 0
// output_rounding_mode: 4
// output_width: 32
// output_fract_width: 0
// config_method: 0

const double fir_compiler_0_coefficients[195] = {-67,-33,-40,-49,-59,-69,-81,-94,-107,-122,-137,-153,-170,-187,-205,-223,-241,-258,-275,-292,-307,-320,-332,-341,-348,-351,-351,-346,-336,-321,-300,-272,-237,-194,-142,-81,-10,71,164,269,386,517,661,819,992,1180,1384,1603,1839,2091,2360,2645,2947,3266,3601,3953,4320,4703,5101,5513,5939,6378,6829,7291,7763,8245,8734,9230,9731,10236,10744,11252,11760,12266,12767,13263,13752,14233,14702,15160,15603,16031,16443,16835,17208,17559,17888,18192,18472,18725,18950,19147,19316,19454,19563,19641,19687,19703,19687,19641,19563,19454,19316,19147,18950,18725,18472,18192,17888,17559,17208,16835,16443,16031,15603,15160,14702,14233,13752,13263,12767,12266,11760,11252,10744,10236,9731,9230,8734,8245,7763,7291,6829,6378,5939,5513,5101,4703,4320,3953,3601,3266,2947,2645,2360,2091,1839,1603,1384,1180,992,819,661,517,386,269,164,71,-10,-81,-142,-194,-237,-272,-300,-321,-336,-346,-351,-351,-348,-341,-332,-320,-307,-292,-275,-258,-241,-223,-205,-187,-170,-153,-137,-122,-107,-94,-81,-69,-59,-49,-40,-33,-67};

const xip_fir_v7_2_pattern fir_compiler_0_chanpats[1] = {P_BASIC};

static xip_fir_v7_2_config gen_fir_compiler_0_config() {
  xip_fir_v7_2_config config;
  config.name                = "fir_compiler_0";
  config.filter_type         = 2;
  config.rate_change         = XIP_FIR_INTEGER_RATE;
  config.interp_rate         = 1;
  config.decim_rate          = 41;
  config.zero_pack_factor    = 1;
  config.coeff               = &fir_compiler_0_coefficients[0];
  config.coeff_padding       = 25;
  config.num_coeffs          = 195;
  config.coeff_sets          = 1;
  config.reloadable          = 0;
  config.is_halfband         = 0;
  config.quantization        = XIP_FIR_MAXIMIZE_DYNAMIC_RANGE;
  config.coeff_width         = 16;
  config.coeff_fract_width   = 0;
  config.chan_seq            = XIP_FIR_BASIC_CHAN_SEQ;
  config.num_channels        = 1;
  config.init_pattern        = fir_compiler_0_chanpats[0];
  config.num_paths           = 2;
  config.data_width          = 16;
  config.data_fract_width    = 0;
  config.output_rounding_mode= XIP_FIR_CONVERGENT_EVEN;
  config.output_width        = 32;
  config.output_fract_width  = 0,
  config.config_method       = XIP_FIR_CONFIG_SINGLE;
  return config;
}

const xip_fir_v7_2_config fir_compiler_0_config = gen_fir_compiler_0_config();

