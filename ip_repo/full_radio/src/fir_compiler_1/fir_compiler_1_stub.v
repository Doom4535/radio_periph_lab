// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2023.2 (lin64) Build 4029153 Fri Oct 13 20:13:54 MDT 2023
// Date        : Sat Aug  3 20:55:03 2024
// Host        : vivado running 64-bit Ubuntu 22.04.4 LTS
// Command     : write_verilog -force -mode synth_stub
//               /home/aaron/Documents/EN.525.742_SystemOnAChipFPGADesignLaboratory/Modules/Week09/Lab6/radio_periph_lab/ip_repo/full_radio/src/fir_compiler_1/fir_compiler_1_stub.v
// Design      : fir_compiler_1
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z020clg400-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* x_core_info = "fir_compiler_v7_2_20,Vivado 2023.2" *)
module fir_compiler_1(aresetn, aclk, s_axis_data_tvalid, 
  s_axis_data_tready, s_axis_data_tdata, m_axis_data_tvalid, m_axis_data_tdata)
/* synthesis syn_black_box black_box_pad_pin="aresetn,s_axis_data_tvalid,s_axis_data_tready,s_axis_data_tdata[63:0],m_axis_data_tvalid,m_axis_data_tdata[31:0]" */
/* synthesis syn_force_seq_prim="aclk" */;
  input aresetn;
  input aclk /* synthesis syn_isclock = 1 */;
  input s_axis_data_tvalid;
  output s_axis_data_tready;
  input [63:0]s_axis_data_tdata;
  output m_axis_data_tvalid;
  output [31:0]m_axis_data_tdata;
endmodule
