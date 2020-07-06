#!/usr/bin/perl -w

# Copyright (c) 2015, Technische Universität Kaiserslautern
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: 
#    Matthias Jung
#    Eder F. Zulian

use warnings;
use strict;

# Assuming this address mapping:
#   <addressmapping>
#        <channel from="27" to="28" />
#        <row   from="14" to="26" />
#        <column from="7" to="13" />
#        <bank  from="4" to="6" />
#        <bytes from="0" to="3" />
#   </addressmapping>

# This is how it should look like later:
# 31:     write   0x0     ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff

my $numberOfRows = 8192;
my $numberOfColumnsPerRow = 128;
my $bytesPerColumn = 16;
my $burstLength = 4; # burst length of 4 columns --> 4 columns written or read per access
my $dataLength = $bytesPerColumn * $burstLength;

my $rowOffset = 0x4000;
my $colOffset = 0x80;

# Generate Data Pattern:
my $dataPatternByte = "ff";

my $dataPattern = "";
for(my $i = 0; $i < $dataLength; $i++)
{
    $dataPattern .= $dataPatternByte;
}

my $clkCounter = 0;
my $addr = 0;

# Generate Trace file (writes):
for(my $row = 0; $row < ($numberOfRows * $rowOffset); $row = $row + $rowOffset)
{
    for(my $col = 0; $col < ($numberOfColumnsPerRow * $colOffset); $col = $col + ($colOffset * $burstLength))
    {
        my $addrHex = sprintf("0x%x", $addr);
        print "$clkCounter:\twrite\t$addrHex\t$dataPattern\n";
        $clkCounter++;
        $addr += $colOffset * $burstLength;
    }
}

$clkCounter = 350000000;
$addr = 0;

# Generate Trace file (reads):
for(my $row = 0; $row < ($numberOfRows * $rowOffset); $row = $row + $rowOffset)
{
    for(my $col = 0; $col < ($numberOfColumnsPerRow * $colOffset); $col = $col + ($colOffset * $burstLength))
    {
        my $addrHex = sprintf("0x%x", $addr);
        print "$clkCounter:\tread\t$addrHex\t$dataPattern\n";
        $clkCounter++;
        $addr += $colOffset * $burstLength;
    }
}
