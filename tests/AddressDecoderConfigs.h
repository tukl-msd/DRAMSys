/*
 * Copyright (c) 2022, RPTU Kaiserslautern-Landau
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Derek Christ
 */

#pragma once

#include <string_view>

inline constexpr std::string_view addressMappingJsonString = R"(
    {
        "addressmapping": {
            "BYTE_BIT": [
                0
            ],
            "COLUMN_BIT": [
                1,
                2,
                3,
                4,
                9,
                10,
                11,
                12,
                13,
                14
            ],
            "BANKGROUP_BIT": [
                5,
                6
            ],
            "BANK_BIT": [
                7,
                8
            ],
            "ROW_BIT": [
                15,
                16,
                17,
                18,
                19,
                20,
                21,
                22,
                23,
                24,
                25,
                26,
                27,
                28,
                29,
                30
            ]
        }
    }
    )";

inline constexpr std::string_view validAddressMappingJsonString = R"(
        {
            "addressmapping": {
                "BYTE_BIT": [
                    0,
                    1,
                    2
                ],
                "COLUMN_BIT": [
                    3,
                    4,
                    5,
                    10,
                    11,
                    12,
                    13,
                    14,
                    15,
                    16
                ],
                "BANKGROUP_BIT": [
                    6,
                    9
                ],
                "BANK_BIT": [
                    7,
                    8
                ],
                "ROW_BIT": [
                    17,
                    18,
                    19,
                    20,
                    21,
                    22,
                    23,
                    24,
                    25,
                    26,
                    27,
                    28,
                    29,
                    30
                ]
            }
        }
        )";

inline constexpr std::string_view validMemSpecJsonString = R"(
{
"memspec": {
    "memarchitecturespec": {
        "burstLength": 8,
        "dataRate": 2,
        "nbrOfBankGroups": 4,
        "nbrOfBanks": 16,
        "nbrOfColumns": 1024,
        "nbrOfRanks": 1,
        "nbrOfRows": 16384,
        "width": 8,
        "nbrOfDevices": 8,
        "nbrOfChannels": 1,
        "RefMode": 1
    },
    "memoryId": "MICRON_4Gb_DDR4-2400_8bit_A",
        "memoryType": "DDR4",
        "mempowerspec": {
            "vdd": 1.2,
            "idd0": 60.75e-3,
            "idd2n": 38.25e-3,
            "idd3n": 44.0e-3,
            "idd4r": 184.5e-3,
            "idd4w": 168.75e-3,
            "idd6n": 20.25e-3,
            "idd2p": 17.0e-3,
            "idd3p": 22.5e-3,
            
            "vpp": 2.5,
            "ipp0": 4.05e-3,
            "ipp2n": 0,
            "ipp3n": 0,
            "ipp4r": 0,
            "ipp4w": 0,
            "ipp6n": 2.6e-3,
            "ipp2p": 17.0e-3,
            "ipp3p": 22.5e-3,
            
            "idd5B": 118.0e-3,
            "ipp5B": 0.0,
            "idd5F2": 0.0,
            "ipp5F2": 0.0,
            "idd5F4": 0.0,
            "ipp5F4": 0.0,
            "vddq": 0.0,

            "iBeta_vdd": 60.75e-3,
            "iBeta_vpp": 4.05e-3
        },
        "memtimingspec": {
            "AL": 0,
            "CCD_L": 6,
            "CCD_S": 4,
            "CKE": 6,
            "CKESR": 7,
            "CL": 16,
            "DQSCK": 2,
            "FAW": 26,
            "RAS": 39,
            "RC": 55,
            "RCD": 16,
            "REFM": 1,
            "REFI": 4680,
            "RFC1": 313,
            "RFC2": 0,
            "RFC4": 0,
            "RL": 16,
            "RPRE": 1,
            "RP": 16,
            "RRD_L": 6,
            "RRD_S": 4,
            "RTP": 12,
            "WL": 16,
            "WPRE": 1,
            "WR": 18,
            "WTR_L": 9,
            "WTR_S": 3,
            "XP": 8,
            "XPDLL": 325,
            "XS": 324,
            "XSDLL": 512,
            "ACTPDEN": 2,
            "PRPDEN": 2,
            "REFPDEN": 2,
            "RTRS": 1,
            "tCK": 833e-12
        },
        "bankwisespec": {
            "factRho": 1.0
        },
        "memimpedancespec": {
            "ck_termination": true,
            "ck_R_eq": 1e6,
            "ck_dyn_E": 1e-12,

            "ca_termination": true,
            "ca_R_eq": 1e6,
            "ca_dyn_E": 1e-12,

            "rdq_termination": true,
            "rdq_R_eq": 1e6,
            "rdq_dyn_E": 1e-12,
            "wdq_termination": true,
            "wdq_R_eq": 1e6,
            "wdq_dyn_E": 1e-12,

            "wdqs_termination": true,
            "wdqs_R_eq": 1e6,
            "wdqs_dyn_E": 1e-12,
            "rdqs_termination": true,
            "rdqs_R_eq": 1e6,
            "rdqs_dyn_E": 1e-12,
            
            "wdbi_termination": true,
            "wdbi_R_eq": 1e6,
            "wdbi_dyn_E": 1e-12,
            "rdbi_termination": true,
            "rdbi_R_eq": 1e6,
            "rdbi_dyn_E": 1e-12
        },
        "prepostamble": {
            "read_zeroes": 0.0,
            "write_zeroes": 0.0,
            "read_ones": 0.0,
            "write_ones": 0.0,
            "read_zeroes_to_ones": 0,
            "write_zeroes_to_ones": 0,
            "write_ones_to_zeroes": 0,
            "read_ones_to_zeroes": 0,
            "readMinTccd": 0,
            "writeMinTccd": 0
        }
    }
}
)";

inline constexpr std::string_view validNP2MemSpecJsonString = R"(
{
"memspec": {
    "memarchitecturespec": {
        "burstLength": 7,
        "dataRate": 2,
        "nbrOfBankGroups": 4,
        "nbrOfBanks": 16,
        "nbrOfColumns": 1024,
        "nbrOfRanks": 1,
        "nbrOfRows": 16384,
        "width": 8,
        "nbrOfDevices": 7,
        "nbrOfChannels": 1,
        "RefMode": 1
    },
    "memoryId": "MICRON_4Gb_DDR4-2400_8bit_A",
        "memoryType": "DDR4",
        "mempowerspec": {
            "vdd": 1.2,
            "idd0": 60.75e-3,
            "idd2n": 38.25e-3,
            "idd3n": 44.0e-3,
            "idd4r": 184.5e-3,
            "idd4w": 168.75e-3,
            "idd6n": 20.25e-3,
            "idd2p": 17.0e-3,
            "idd3p": 22.5e-3,
            
            "vpp": 2.5,
            "ipp0": 4.05e-3,
            "ipp2n": 0,
            "ipp3n": 0,
            "ipp4r": 0,
            "ipp4w": 0,
            "ipp6n": 2.6e-3,
            "ipp2p": 17.0e-3,
            "ipp3p": 22.5e-3,
            
            "idd5B": 118.0e-3,
            "ipp5B": 0.0,
            "idd5F2": 0.0,
            "ipp5F2": 0.0,
            "idd5F4": 0.0,
            "ipp5F4": 0.0,
            "vddq": 0.0,

            "iBeta_vdd": 60.75e-3,
            "iBeta_vpp": 4.05e-3
        },
        "memtimingspec": {
            "AL": 0,
            "CCD_L": 6,
            "CCD_S": 4,
            "CKE": 6,
            "CKESR": 7,
            "CL": 16,
            "DQSCK": 2,
            "FAW": 26,
            "RAS": 39,
            "RC": 55,
            "RCD": 16,
            "REFM": 1,
            "REFI": 4680,
            "RFC1": 313,
            "RFC2": 0,
            "RFC4": 0,
            "RL": 16,
            "RPRE": 1,
            "RP": 16,
            "RRD_L": 6,
            "RRD_S": 4,
            "RTP": 12,
            "WL": 16,
            "WPRE": 1,
            "WR": 18,
            "WTR_L": 9,
            "WTR_S": 3,
            "XP": 8,
            "XPDLL": 325,
            "XS": 324,
            "XSDLL": 512,
            "ACTPDEN": 2,
            "PRPDEN": 2,
            "REFPDEN": 2,
            "RTRS": 1,
            "tCK": 833e-12
        },
        "bankwisespec": {
            "factRho": 1.0
        },
        "memimpedancespec": {
            "ck_termination": true,
            "ck_R_eq": 1e6,
            "ck_dyn_E": 1e-12,

            "ca_termination": true,
            "ca_R_eq": 1e6,
            "ca_dyn_E": 1e-12,

            "rdq_termination": true,
            "rdq_R_eq": 1e6,
            "rdq_dyn_E": 1e-12,
            "wdq_termination": true,
            "wdq_R_eq": 1e6,
            "wdq_dyn_E": 1e-12,

            "wdqs_termination": true,
            "wdqs_R_eq": 1e6,
            "wdqs_dyn_E": 1e-12,
            "rdqs_termination": true,
            "rdqs_R_eq": 1e6,
            "rdqs_dyn_E": 1e-12,
            
            "wdbi_termination": true,
            "wdbi_R_eq": 1e6,
            "wdbi_dyn_E": 1e-12,
            "rdbi_termination": true,
            "rdbi_R_eq": 1e6,
            "rdbi_dyn_E": 1e-12
        },
        "prepostamble": {
            "read_zeroes": 0.0,
            "write_zeroes": 0.0,
            "read_ones": 0.0,
            "write_ones": 0.0,
            "read_zeroes_to_ones": 0,
            "write_zeroes_to_ones": 0,
            "write_ones_to_zeroes": 0,
            "read_ones_to_zeroes": 0,
            "readMinTccd": 0,
            "writeMinTccd": 0
        }
    }
}
)";

inline constexpr std::string_view invalidMaxAddressMemSpecJsonString = R"(
{
"memspec": {    
        "memarchitecturespec": {
        "burstLength": 8,
        "dataRate": 2,
        "nbrOfBankGroups": 4,
        "nbrOfBanks": 16,
        "nbrOfColumns": 1024,
        "nbrOfRanks": 1,
        "nbrOfRows": 16385,
        "width": 8,
        "nbrOfDevices": 8,
        "nbrOfChannels": 1,
        "RefMode": 1
    },
    "memoryId": "MICRON_4Gb_DDR4-2400_8bit_A",
        "memoryType": "DDR4",
        "mempowerspec": {
            "vdd": 1.2,
            "idd0": 60.75e-3,
            "idd2n": 38.25e-3,
            "idd3n": 44.0e-3,
            "idd4r": 184.5e-3,
            "idd4w": 168.75e-3,
            "idd6n": 20.25e-3,
            "idd2p": 17.0e-3,
            "idd3p": 22.5e-3,
            
            "vpp": 2.5,
            "ipp0": 4.05e-3,
            "ipp2n": 0,
            "ipp3n": 0,
            "ipp4r": 0,
            "ipp4w": 0,
            "ipp6n": 2.6e-3,
            "ipp2p": 17.0e-3,
            "ipp3p": 22.5e-3,
            
            "idd5B": 118.0e-3,
            "ipp5B": 0.0,
            "idd5F2": 0.0,
            "ipp5F2": 0.0,
            "idd5F4": 0.0,
            "ipp5F4": 0.0,
            "vddq": 0.0,

            "iBeta_vdd": 60.75e-3,
            "iBeta_vpp": 4.05e-3
        },
        "memtimingspec": {
            "AL": 0,
            "CCD_L": 6,
            "CCD_S": 4,
            "CKE": 6,
            "CKESR": 7,
            "CL": 16,
            "DQSCK": 2,
            "FAW": 26,
            "RAS": 39,
            "RC": 55,
            "RCD": 16,
            "REFM": 1,
            "REFI": 4680,
            "RFC1": 313,
            "RFC2": 0,
            "RFC4": 0,
            "RL": 16,
            "RPRE": 1,
            "RP": 16,
            "RRD_L": 6,
            "RRD_S": 4,
            "RTP": 12,
            "WL": 16,
            "WPRE": 1,
            "WR": 18,
            "WTR_L": 9,
            "WTR_S": 3,
            "XP": 8,
            "XPDLL": 325,
            "XS": 324,
            "XSDLL": 512,
            "ACTPDEN": 2,
            "PRPDEN": 2,
            "REFPDEN": 2,
            "RTRS": 1,
            "tCK": 833e-12
        },
        "bankwisespec": {
            "factRho": 1.0
        },
        "memimpedancespec": {
            "ck_termination": true,
            "ck_R_eq": 1e6,
            "ck_dyn_E": 1e-12,

            "ca_termination": true,
            "ca_R_eq": 1e6,
            "ca_dyn_E": 1e-12,

            "rdq_termination": true,
            "rdq_R_eq": 1e6,
            "rdq_dyn_E": 1e-12,
            "wdq_termination": true,
            "wdq_R_eq": 1e6,
            "wdq_dyn_E": 1e-12,

            "wdqs_termination": true,
            "wdqs_R_eq": 1e6,
            "wdqs_dyn_E": 1e-12,
            "rdqs_termination": true,
            "rdqs_R_eq": 1e6,
            "rdqs_dyn_E": 1e-12,

            "wdbi_termination": true,
            "wdbi_R_eq": 1e6,
            "wdbi_dyn_E": 1e-12,
            "rdbi_termination": true,
            "rdbi_R_eq": 1e6,
            "rdbi_dyn_E": 1e-12
        },
        "prepostamble": {
            "read_zeroes": 0.0,
            "write_zeroes": 0.0,
            "read_ones": 0.0,
            "write_ones": 0.0,
            "read_zeroes_to_ones": 0,
            "write_zeroes_to_ones": 0,
            "write_ones_to_zeroes": 0,
            "read_ones_to_zeroes": 0,
            "readMinTccd": 0,
            "writeMinTccd": 0
        }
    }
}
)";

inline constexpr std::string_view addressMappingWithDuplicatesJsonString = R"(
        {
            "addressmapping": {
                "BYTE_BIT": [
                    0,
                    1,
                    2
                ],
                "COLUMN_BIT": [
                    3,
                    4,
                    5,
                    3, 
                    10,
                    11
                ],
                "BANKGROUP_BIT": [
                    6,
                    9
                ],
                "BANK_BIT": [
                    7,
                    8
                ],
                "ROW_BIT": [
                    17,
                    18,
                    19,
                    20,
                    21,
                    22,
                    23,
                    24,
                    25,
                    26,
                    27,
                    28,
                    29,
                    30
                ]
            }
        }
        )";

inline constexpr std::string_view nonContinuousByteBitsAddressMappingJsonString = R"(
            {
                "addressmapping": {
                    "BYTE_BIT": [
                        0,
                        1, 
                        3  
                    ],
                    "COLUMN_BIT": [
                        2,
                        4,
                        5,
                        10,
                        11,
                        12
                    ],
                    "BANKGROUP_BIT": [
                        6,
                        9
                    ],
                    "BANK_BIT": [
                        7,
                        8
                    ],
                    "ROW_BIT": [
                        17,
                        18,
                        19,
                        20,
                        21,
                        22,
                        23,
                        24,
                        25,
                        26,
                        27,
                        28,
                        29,
                        30
                    ]
                }
            }
            )";

inline constexpr std::string_view invalidChannelMemSpecJsonString = R"(
{
"memspec": {
    "memarchitecturespec": {
        "burstLength": 8,
        "dataRate": 2,
        "nbrOfBankGroups": 4,
        "nbrOfBanks": 8,
        "nbrOfColumns": 1024,
        "nbrOfRanks": 1,
        "nbrOfRows": 16384,
        "width": 8,
        "nbrOfDevices": 8,
        "nbrOfChannels": 2,
        "RefMode": 1
    },
    "memoryId": "MICRON_4Gb_DDR4-2400_8bit_A",
        "memoryType": "DDR4",
        "mempowerspec": {
            "vdd": 1.2,
            "idd0": 60.75e-3,
            "idd2n": 38.25e-3,
            "idd3n": 44.0e-3,
            "idd4r": 184.5e-3,
            "idd4w": 168.75e-3,
            "idd6n": 20.25e-3,
            "idd2p": 17.0e-3,
            "idd3p": 22.5e-3,
            
            "vpp": 2.5,
            "ipp0": 4.05e-3,
            "ipp2n": 0,
            "ipp3n": 0,
            "ipp4r": 0,
            "ipp4w": 0,
            "ipp6n": 2.6e-3,
            "ipp2p": 17.0e-3,
            "ipp3p": 22.5e-3,
            
            "idd5B": 118.0e-3,
            "ipp5B": 0.0,
            "idd5F2": 0.0,
            "ipp5F2": 0.0,
            "idd5F4": 0.0,
            "ipp5F4": 0.0,
            "vddq": 0.0,

            "iBeta_vdd": 60.75e-3,
            "iBeta_vpp": 4.05e-3
        },
        "memtimingspec": {
            "AL": 0,
            "CCD_L": 6,
            "CCD_S": 4,
            "CKE": 6,
            "CKESR": 7,
            "CL": 16,
            "DQSCK": 2,
            "FAW": 26,
            "RAS": 39,
            "RC": 55,
            "RCD": 16,
            "REFM": 1,
            "REFI": 4680,
            "RFC1": 313,
            "RFC2": 0,
            "RFC4": 0,
            "RL": 16,
            "RPRE": 1,
            "RP": 16,
            "RRD_L": 6,
            "RRD_S": 4,
            "RTP": 12,
            "WL": 16,
            "WPRE": 1,
            "WR": 18,
            "WTR_L": 9,
            "WTR_S": 3,
            "XP": 8,
            "XPDLL": 325,
            "XS": 324,
            "XSDLL": 512,
            "ACTPDEN": 2,
            "PRPDEN": 2,
            "REFPDEN": 2,
            "RTRS": 1,
            "tCK": 833e-12
        },
        "bankwisespec": {
            "factRho": 1.0
        },
        "memimpedancespec": {
            "ck_termination": true,
            "ck_R_eq": 1e6,
            "ck_dyn_E": 1e-12,

            "ca_termination": true,
            "ca_R_eq": 1e6,
            "ca_dyn_E": 1e-12,

            "rdq_termination": true,
            "rdq_R_eq": 1e6,
            "rdq_dyn_E": 1e-12,
            "wdq_termination": true,
            "wdq_R_eq": 1e6,
            "wdq_dyn_E": 1e-12,

            "wdqs_termination": true,
            "wdqs_R_eq": 1e6,
            "wdqs_dyn_E": 1e-12,
            "rdqs_termination": true,
            "rdqs_R_eq": 1e6,
            "rdqs_dyn_E": 1e-12,

            "wdbi_termination": true,
            "wdbi_R_eq": 1e6,
            "wdbi_dyn_E": 1e-12,
            "rdbi_termination": true,
            "rdbi_R_eq": 1e6,
            "rdbi_dyn_E": 1e-12
        },
        "prepostamble": {
            "read_zeroes": 0.0,
            "write_zeroes": 0.0,
            "read_ones": 0.0,
            "write_ones": 0.0,
            "read_zeroes_to_ones": 0,
            "write_zeroes_to_ones": 0,
            "write_ones_to_zeroes": 0,
            "read_ones_to_zeroes": 0,
            "readMinTccd": 0,
            "writeMinTccd": 0
        }
    }
}
)";

inline constexpr std::string_view invalidBankGroupMemSpecJsonString = R"(
    {
    "memspec": {
        "memarchitecturespec": {
            "burstLength": 8,
            "dataRate": 2,
            "nbrOfBankGroups": 8,
            "nbrOfBanks": 8,
            "nbrOfColumns": 1024,
            "nbrOfRanks": 1,
            "nbrOfRows": 16384,
            "width": 8,
            "nbrOfDevices": 8,
            "nbrOfChannels": 1,
            "RefMode": 1
        },
        "memoryId": "MICRON_4Gb_DDR4-2400_8bit_A",
        "memoryType": "DDR4",
        "mempowerspec": {
            "vdd": 1.2,
            "idd0": 60.75e-3,
            "idd2n": 38.25e-3,
            "idd3n": 44.0e-3,
            "idd4r": 184.5e-3,
            "idd4w": 168.75e-3,
            "idd6n": 20.25e-3,
            "idd2p": 17.0e-3,
            "idd3p": 22.5e-3,
            
            "vpp": 2.5,
            "ipp0": 4.05e-3,
            "ipp2n": 0,
            "ipp3n": 0,
            "ipp4r": 0,
            "ipp4w": 0,
            "ipp6n": 2.6e-3,
            "ipp2p": 17.0e-3,
            "ipp3p": 22.5e-3,
            
            "idd5B": 118.0e-3,
            "ipp5B": 0.0,
            "idd5F2": 0.0,
            "ipp5F2": 0.0,
            "idd5F4": 0.0,
            "ipp5F4": 0.0,
            "vddq": 0.0,

            "iBeta_vdd": 60.75e-3,
            "iBeta_vpp": 4.05e-3
        },
        "memtimingspec": {
            "AL": 0,
            "CCD_L": 6,
            "CCD_S": 4,
            "CKE": 6,
            "CKESR": 7,
            "CL": 16,
            "DQSCK": 2,
            "FAW": 26,
            "RAS": 39,
            "RC": 55,
            "RCD": 16,
            "REFM": 1,
            "REFI": 4680,
            "RFC1": 313,
            "RFC2": 0,
            "RFC4": 0,
            "RL": 16,
            "RPRE": 1,
            "RP": 16,
            "RRD_L": 6,
            "RRD_S": 4,
            "RTP": 12,
            "WL": 16,
            "WPRE": 1,
            "WR": 18,
            "WTR_L": 9,
            "WTR_S": 3,
            "XP": 8,
            "XPDLL": 325,
            "XS": 324,
            "XSDLL": 512,
            "ACTPDEN": 2,
            "PRPDEN": 2,
            "REFPDEN": 2,
            "RTRS": 1,
            "tCK": 833e-12
        },
        "bankwisespec": {
            "factRho": 1.0
        },
        "memimpedancespec": {
            "ck_termination": true,
            "ck_R_eq": 1e6,
            "ck_dyn_E": 1e-12,

            "ca_termination": true,
            "ca_R_eq": 1e6,
            "ca_dyn_E": 1e-12,

            "rdq_termination": true,
            "rdq_R_eq": 1e6,
            "rdq_dyn_E": 1e-12,
            "wdq_termination": true,
            "wdq_R_eq": 1e6,
            "wdq_dyn_E": 1e-12,

            "wdqs_termination": true,
            "wdqs_R_eq": 1e6,
            "wdqs_dyn_E": 1e-12,
            "rdqs_termination": true,
            "rdqs_R_eq": 1e6,
            "rdqs_dyn_E": 1e-12,

            "wdbi_termination": true,
            "wdbi_R_eq": 1e6,
            "wdbi_dyn_E": 1e-12,
            "rdbi_termination": true,
            "rdbi_R_eq": 1e6,
            "rdbi_dyn_E": 1e-12
        },
        "prepostamble": {
            "read_zeroes": 0.0,
            "write_zeroes": 0.0,
            "read_ones": 0.0,
            "write_ones": 0.0,
            "read_zeroes_to_ones": 0,
            "write_zeroes_to_ones": 0,
            "write_ones_to_zeroes": 0,
            "read_ones_to_zeroes": 0,
            "readMinTccd": 0,
            "writeMinTccd": 0
        }
    }
    }
    )";

    inline constexpr std::string_view invalidRanksMemSpecJsonString = R"(
        {
        "memspec": {
            "memarchitecturespec": {
                "burstLength": 8,
                "dataRate": 2,
                "nbrOfBankGroups": 2,
                "nbrOfBanks": 8,
                "nbrOfColumns": 1024,
                "nbrOfRanks": 2,
                "nbrOfRows": 16384,
                "width": 8,
                "nbrOfDevices": 8,
                "nbrOfChannels": 1,
                "RefMode": 1
            },
            "memoryId": "MICRON_4Gb_DDR4-2400_8bit_A",
        "memoryType": "DDR4",
        "mempowerspec": {
            "vdd": 1.2,
            "idd0": 60.75e-3,
            "idd2n": 38.25e-3,
            "idd3n": 44.0e-3,
            "idd4r": 184.5e-3,
            "idd4w": 168.75e-3,
            "idd6n": 20.25e-3,
            "idd2p": 17.0e-3,
            "idd3p": 22.5e-3,
            
            "vpp": 2.5,
            "ipp0": 4.05e-3,
            "ipp2n": 0,
            "ipp3n": 0,
            "ipp4r": 0,
            "ipp4w": 0,
            "ipp6n": 2.6e-3,
            "ipp2p": 17.0e-3,
            "ipp3p": 22.5e-3,
            
            "idd5B": 118.0e-3,
            "ipp5B": 0.0,
            "idd5F2": 0.0,
            "ipp5F2": 0.0,
            "idd5F4": 0.0,
            "ipp5F4": 0.0,
            "vddq": 0.0,

            "iBeta_vdd": 60.75e-3,
            "iBeta_vpp": 4.05e-3
        },
        "memtimingspec": {
            "AL": 0,
            "CCD_L": 6,
            "CCD_S": 4,
            "CKE": 6,
            "CKESR": 7,
            "CL": 16,
            "DQSCK": 2,
            "FAW": 26,
            "RAS": 39,
            "RC": 55,
            "RCD": 16,
            "REFM": 1,
            "REFI": 4680,
            "RFC1": 313,
            "RFC2": 0,
            "RFC4": 0,
            "RL": 16,
            "RPRE": 1,
            "RP": 16,
            "RRD_L": 6,
            "RRD_S": 4,
            "RTP": 12,
            "WL": 16,
            "WPRE": 1,
            "WR": 18,
            "WTR_L": 9,
            "WTR_S": 3,
            "XP": 8,
            "XPDLL": 325,
            "XS": 324,
            "XSDLL": 512,
            "ACTPDEN": 2,
            "PRPDEN": 2,
            "REFPDEN": 2,
            "RTRS": 1,
            "tCK": 833e-12
        },
        "bankwisespec": {
            "factRho": 1.0
        },
        "memimpedancespec": {
            "ck_termination": true,
            "ck_R_eq": 1e6,
            "ck_dyn_E": 1e-12,

            "ca_termination": true,
            "ca_R_eq": 1e6,
            "ca_dyn_E": 1e-12,

            "rdq_termination": true,
            "rdq_R_eq": 1e6,
            "rdq_dyn_E": 1e-12,
            "wdq_termination": true,
            "wdq_R_eq": 1e6,
            "wdq_dyn_E": 1e-12,

            "wdqs_termination": true,
            "wdqs_R_eq": 1e6,
            "wdqs_dyn_E": 1e-12,
            "rdqs_termination": true,
            "rdqs_R_eq": 1e6,
            "rdqs_dyn_E": 1e-12,
            
            "wdbi_termination": true,
            "wdbi_R_eq": 1e6,
            "wdbi_dyn_E": 1e-12,
            "rdbi_termination": true,
            "rdbi_R_eq": 1e6,
            "rdbi_dyn_E": 1e-12
        },
        "prepostamble": {
            "read_zeroes": 0.0,
            "write_zeroes": 0.0,
            "read_ones": 0.0,
            "write_ones": 0.0,
            "read_zeroes_to_ones": 0,
            "write_zeroes_to_ones": 0,
            "write_ones_to_zeroes": 0,
            "read_ones_to_zeroes": 0,
            "readMinTccd": 0,
            "writeMinTccd": 0
        }
    }
        }
        )";